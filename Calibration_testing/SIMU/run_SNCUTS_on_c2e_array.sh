#!/bin/bash
#SBATCH --job-name=sncuts
#SBATCH --mem=6G
#SBATCH --licenses=sps
#SBATCH --time=24:00:00
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --output=slurm-sncuts-%A_%a.out
#SBATCH --error=slurm-sncuts-%A_%a.err

set -euo pipefail

[ $# -eq 3 ] || { echo "ERROR: Usage: $0 LISTFILE SRC_BASE DST_BASE"; exit 2; }

LISTFILE="$1"
SRC_BASE="$2"
DST_BASE="$3"

[ -f "${LISTFILE}" ] || { echo "ERROR: LISTFILE does not exist: ${LISTFILE}"; exit 3; }
[ -d "${SRC_BASE}" ] || { echo "ERROR: SRC_BASE does not exist: ${SRC_BASE}"; exit 3; }

[ -n "${SLURM_ARRAY_TASK_ID:-}" ] || { echo "ERROR: SLURM_ARRAY_TASK_ID is not set"; exit 6; }

IDX="${SLURM_ARRAY_TASK_ID}"
INFILE=$(sed -n "${IDX}p" "${LISTFILE}")
[ -n "${INFILE}" ] || { echo "ERROR: Empty infile for index ${IDX}"; exit 1; }

# process ONLY 2D_reco_Bi_*.brio files
case "$(basename "${INFILE}")" in
  2D_reco_Bi_*.brio) ;;
  *)
    echo "ERROR: Input file does not match expected pattern 2D_reco_Bi_*.brio: ${INFILE}"
    exit 5
    ;;
esac

RUNID=$(basename "$(dirname "${INFILE}")")  # e.g. "42"
BASENAME=$(basename "${INFILE}" .brio)      # e.g. 2D_reco_Bi_42

# Output goes to DST_BASE/<num>/
OUTDIR="${DST_BASE}/${RUNID}"
mkdir -p "${OUTDIR}"

OUTPUT="${OUTDIR}/${BASENAME}-SNCUTS.brio"

# --- Verify key paths before environment setup ---
PROFILE_SCRIPT="${THRONG_DIR:-}/config/supernemo_profile.bash"
STACK_NAME="falaise@2026-04-07"
#CONF="/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/SIMU/Bi-207/SNCuts-pipeline.conf"
CONF="/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/SIMU/SNCuts_600-800.conf"

[ -f "${INFILE}" ]         || { echo "ERROR: Input file does not exist: ${INFILE}"; exit 101; }
[ -n "${THRONG_DIR:-}" ]   || { echo "ERROR: THRONG_DIR is not set"; exit 106; }
[ -f "${PROFILE_SCRIPT}" ] || { echo "ERROR: Profile script does not exist: ${PROFILE_SCRIPT}"; exit 105; }
[ -f "${CONF}" ]           || { echo "ERROR: Config does not exist: ${CONF}"; exit 104; }

# --- Load environment safely ---
set +e
set +u
source "${PROFILE_SCRIPT}"
PROFILE_RC=$?
snswmgr_load_stack "${STACK_NAME}"
STACK_RC=$?
BIN="$(command -v flreconstruct || true)"
set -e
set -u

echo "Profile setup exit code: ${PROFILE_RC}"
echo "Stack load exit code   : ${STACK_RC}"
echo "DEBUG: which flreconstruct = ${BIN}"
echo "DEBUG: CONF exists?    $( [ -f "${CONF}" ] && echo yes || echo no )"

[ "${PROFILE_RC}" -eq 0 ] || { echo "ERROR: sourcing profile failed"; exit 107; }
[ "${STACK_RC}"   -eq 0 ] || { echo "ERROR: snswmgr_load_stack failed"; exit 108; }
[ -n "${BIN}" ]            || { echo "ERROR: flreconstruct not found in PATH"; exit 102; }
[ -x "${BIN}" ]            || { echo "ERROR: flreconstruct is not executable"; exit 103; }

echo "========================================"
echo "Input : ${INFILE}"
echo "Output: ${OUTPUT}"
echo "========================================"

# Skip if output already exists
if [ -f "${OUTPUT}" ]; then
  echo "INFO: Output already exists, skipping: ${OUTPUT}"
  exit 0
fi

"${BIN}" \
  -i "${INFILE}" \
  -p "${CONF}" \
  -o "${OUTPUT}"

[ -f "${OUTPUT}" ] || { echo "ERROR: Output was not created: ${OUTPUT}"; exit 30; }

echo "Done: ${OUTPUT}"
