#!/bin/bash
#SBATCH --job-name=sncuts_single
#SBATCH --mem=3G
#SBATCH --licenses=sps
#SBATCH --time=03:00:00
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --output=slurm-sncuts-single.out
#SBATCH --error=slurm-sncuts-single.err

set -euo pipefail

INFILE="/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/SIMU/brios/SNCUTS/NEW_s_positions/80/2D_reco_Bi_80-SNCUTS.brio"
OUTPUT="/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/SIMU/brios/SNCUTS/NEW_s_positions/2D_reco_Bi_80-SNCUTS_600-800.brio"

PROFILE_SCRIPT="${THRONG_DIR:-}/config/supernemo_profile.bash"
STACK_NAME="falaise@2026-04-07"
CONF="/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/SIMU/SNCuts_600-800.conf"

[ -f "${INFILE}" ]         || { echo "ERROR: Input file does not exist: ${INFILE}"; exit 101; }
[ -n "${THRONG_DIR:-}" ]   || { echo "ERROR: THRONG_DIR is not set"; exit 106; }
[ -f "${PROFILE_SCRIPT}" ] || { echo "ERROR: Profile script does not exist: ${PROFILE_SCRIPT}"; exit 105; }
[ -f "${CONF}" ]           || { echo "ERROR: Config does not exist: ${CONF}"; exit 104; }

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

echo "Input : ${INFILE}"
echo "Output: ${OUTPUT}"

if [ -f "${OUTPUT}" ]; then
  echo "INFO: Output already exists, skipping: ${OUTPUT}"
  exit 0
fi

echo "Running SNCuts..."
set +e
"${BIN}" \
  -i "${INFILE}" \
  -p "${CONF}" \
  -o "${OUTPUT}"
RC=$?
set -e

echo "flreconstruct exit code: ${RC}"

if [ "${RC}" -ne 0 ]; then
  echo "ERROR: flreconstruct failed with exit code ${RC}"
  [ -f "${OUTPUT}" ] && rm -f "${OUTPUT}"
  exit "${RC}"
fi

[ -f "${OUTPUT}" ] || { echo "ERROR: Output was not created: ${OUTPUT}"; exit 30; }

OUT_STAT=$(stat -c "%s %Y" "${OUTPUT}")
echo "Output stat: ${OUT_STAT}"
echo "Done: ${OUTPUT}"
