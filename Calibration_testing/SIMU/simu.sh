#!/bin/bash
#SBATCH --job-name=simu_reco_bi207
#SBATCH --mem=10G
#SBATCH --licenses=sps
#SBATCH --time=72:00:00
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --output=slurm-simu-%A_%a.out
#SBATCH --error=slurm-simu-%A_%a.err

set -euo pipefail

# ============================================================
# ARGUMENTS ? same pattern as run_calibrate_using_BOTH-const_array.sh
# Usage: called by sbatch with SIMU_DATA as $1
# SLURM_ARRAY_TASK_ID is used as num (folder index)
# ============================================================
[ $# -eq 1 ] || { echo "ERROR: Usage: $0 SIMU_DATA"; exit 2; }

SIMU_DATA="$1"

[ -n "${SIMU_DATA}" ] || { echo "ERROR: SIMU_DATA is empty"; exit 3; }
[ -d "${SIMU_DATA}" ] || { echo "ERROR: SIMU_DATA does not exist: ${SIMU_DATA}"; exit 3; }

[ -n "${SLURM_ARRAY_TASK_ID:-}" ] || { echo "ERROR: SLURM_ARRAY_TASK_ID is not set"; exit 6; }

num="${SLURM_ARRAY_TASK_ID}"

# ============================================================
# PATHS
# ============================================================
MY_DIR="/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/SIMU/Bi-207"

SIMU_CONF="${MY_DIR}/simu_setup.conf"
PIPE1_CONF="${MY_DIR}/1S-pipeline.conf"
PIPE2_CONF="${MY_DIR}/2D-CMRMN-CPT-GT-pipeline.conf"

OUTDIR="${SIMU_DATA}/${num}"
mkdir -p "${OUTDIR}"

SIMU_OUT="${OUTDIR}/simu_Bi_${num}.brio"
RECO1_OUT="${OUTDIR}/1S_Bi_${num}.brio"
RECO2_OUT="${OUTDIR}/2D_reco_Bi_${num}.brio"

# ============================================================
# VERIFY KEY PATHS before environment setup
# ============================================================
[ -f "${SIMU_CONF}" ]  || { echo "ERROR: simu config not found: ${SIMU_CONF}";  exit 20; }
[ -f "${PIPE1_CONF}" ] || { echo "ERROR: pipeline 1 not found: ${PIPE1_CONF}";  exit 21; }
[ -f "${PIPE2_CONF}" ] || { echo "ERROR: pipeline 2 not found: ${PIPE2_CONF}";  exit 22; }

[ -n "${THRONG_DIR:-}" ] || { echo "ERROR: THRONG_DIR is not set"; exit 106; }

PROFILE_SCRIPT="${THRONG_DIR}/config/supernemo_profile.bash"
STACK_NAME="falaise@2026-04-07"

[ -f "${PROFILE_SCRIPT}" ] || { echo "ERROR: profile script not found: ${PROFILE_SCRIPT}"; exit 105; }

# ============================================================
# LOAD ENVIRONMENT safely
# ============================================================
set +e
set +u
source "${PROFILE_SCRIPT}"
PROFILE_RC=$?
snswmgr_load_stack "${STACK_NAME}"
STACK_RC=$?
BIN_FLSIM="$(command -v flsimulate || true)"
BIN_FLREC="$(command -v flreconstruct || true)"
set -e
set -u

echo "Profile setup exit code: ${PROFILE_RC}"
echo "Stack load exit code   : ${STACK_RC}"
echo "DEBUG: THRONG_DIR      = ${THRONG_DIR}"
echo "DEBUG: STACK_NAME      = ${STACK_NAME}"
echo "DEBUG: flsimulate      = ${BIN_FLSIM}"
echo "DEBUG: flreconstruct   = ${BIN_FLREC}"

[ "${PROFILE_RC}" -eq 0 ]  || { echo "ERROR: sourcing profile failed";          exit 107; }
[ "${STACK_RC}"   -eq 0 ]  || { echo "ERROR: snswmgr_load_stack failed";        exit 108; }
[ -n "${BIN_FLSIM}" ]      || { echo "ERROR: flsimulate not found in PATH";     exit 102; }
[ -x "${BIN_FLSIM}" ]      || { echo "ERROR: flsimulate is not executable";     exit 103; }
[ -n "${BIN_FLREC}" ]      || { echo "ERROR: flreconstruct not found in PATH";  exit 102; }
[ -x "${BIN_FLREC}" ]      || { echo "ERROR: flreconstruct is not executable";  exit 103; }

echo "========================================"
echo "num     : ${num}"
echo "OUTDIR  : ${OUTDIR}"
echo "SIMU_OUT: ${SIMU_OUT}"
echo "RECO1   : ${RECO1_OUT}"
echo "RECO2   : ${RECO2_OUT}"
echo "========================================"

# Skip if final output already exists
if [ -f "${RECO2_OUT}" ]; then
  echo "INFO: Final output already exists, skipping: ${RECO2_OUT}"
  exit 0
fi

# ============================================================
# STEP 1: flsimulate
# ============================================================
echo "--- STEP 1: flsimulate ---"

"${BIN_FLSIM}" \
    -c "${SIMU_CONF}" \
    -o "${SIMU_OUT}"

[ -f "${SIMU_OUT}" ] || { echo "ERROR: flsimulate did not produce output: ${SIMU_OUT}"; exit 30; }
echo "flsimulate done: ${SIMU_OUT}"

# ============================================================
# STEP 2: flreconstruct ? pipeline 1 (1S)
# ============================================================
echo "--- STEP 2: flreconstruct step 1 (1S-pipeline) ---"

"${BIN_FLREC}" \
    -i "${SIMU_OUT}" \
    -p "${PIPE1_CONF}" \
    -o "${RECO1_OUT}"

[ -f "${RECO1_OUT}" ] || { echo "ERROR: reco step 1 did not produce output: ${RECO1_OUT}"; exit 31; }
echo "Reco step 1 done: ${RECO1_OUT}"

# ============================================================
# STEP 3: flreconstruct ? pipeline 2 (Cimrman + CPT)
# ============================================================
echo "--- STEP 3: flreconstruct step 2 (2D-CMRMN-CPT-GT-pipeline) ---"

"${BIN_FLREC}" \
    -i "${RECO1_OUT}" \
    -p "${PIPE2_CONF}" \
    -o "${RECO2_OUT}"

[ -f "${RECO2_OUT}" ] || { echo "ERROR: reco step 2 did not produce output: ${RECO2_OUT}"; exit 32; }
echo "Reco step 2 done: ${RECO2_OUT}"

# ============================================================
# CLEANUP ? remove intermediates, keep only final brio
# same logic as original: rm -f simu_Bi_N.brio after success
# ============================================================
echo "--- Cleanup ---"
rm -f "${SIMU_OUT}"
echo "Removed: ${SIMU_OUT}"
rm -f "${RECO1_OUT}"
echo "Removed: ${RECO1_OUT}"

echo "========================================"
echo "Final output: ${RECO2_OUT}"
echo "Done."
echo "========================================"
