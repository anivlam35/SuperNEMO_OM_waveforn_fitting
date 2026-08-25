#!/bin/bash
#SBATCH --job-name=sncalib
#SBATCH --mem=8G
#SBATCH --licenses=sps
#SBATCH --time=72:00:00
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --output=slurm-sncalib-%j.out
#SBATCH --error=slurm-sncalib-%j.err

set -euo pipefail

INFILE="merged_calib_2025X-BOTH.root"
OUTFILE="calib_constants-BOTH.csv"
BIN="/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/Calibration_Constants_Extraction-BOTH/build/SNCalib/sncalib"
CONF="/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/Calibration_Constants_Extraction-BOTH/build/SNCalib/params.conf"

PROFILE_SCRIPT="${THRONG_DIR:-}/config/supernemo_profile.bash"
STACK_NAME="falaise@2026-02-09"

[ -f "${INFILE}" ]  || { echo "ERROR: input file not found: ${INFILE}"; exit 1; }
[ -f "${BIN}" ]     || { echo "ERROR: sncalib binary not found: ${BIN}"; exit 2; }
[ -x "${BIN}" ]     || { echo "ERROR: sncalib is not executable: ${BIN}"; exit 3; }
[ -f "${CONF}" ]    || { echo "ERROR: params.conf not found: ${CONF}"; exit 4; }
[ -n "${THRONG_DIR:-}" ] || { echo "ERROR: THRONG_DIR is not set"; exit 5; }
[ -f "${PROFILE_SCRIPT}" ] || { echo "ERROR: profile script not found: ${PROFILE_SCRIPT}"; exit 6; }

set +e
set +u
source "${PROFILE_SCRIPT}"
PROFILE_RC=$?
snswmgr_load_stack "${STACK_NAME}"
STACK_RC=$?
set -e
set -u

echo "Profile setup exit code: ${PROFILE_RC}"
echo "Stack load exit code   : ${STACK_RC}"

[ "${PROFILE_RC}" -eq 0 ] || { echo "ERROR: sourcing profile failed"; exit 107; }
[ "${STACK_RC}"   -eq 0 ] || { echo "ERROR: snswmgr_load_stack failed"; exit 108; }

echo "Input : ${INFILE}"
echo "Output: ${OUTFILE}"
echo "Running sncalib..."

"${BIN}" \
  -i "${INFILE}" \
  -o "${OUTFILE}" \
  -p "${CONF}" \
  -s -V

echo "Done: ${OUTFILE}"
