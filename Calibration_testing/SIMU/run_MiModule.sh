#!/bin/bash
#SBATCH --job-name=miroot
#SBATCH --mem=2G
#SBATCH --licenses=sps
#SBATCH --time=10:00:00
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --output=slurm-miroot-%A_%a.out
#SBATCH --error=slurm-miroot-%A_%a.err

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

case "$(basename "${INFILE}")" in
  2D_reco_Bi_*.brio) ;;
  *)
    echo "ERROR: Input file does not match expected pattern: ${INFILE}"
    exit 5
    ;;
esac

RUNID=$(basename "$(dirname "${INFILE}")")   # e.g. "42"
BASENAME=$(basename "${INFILE}" .brio)       # e.g. 2D_reco_Bi_42

OUTDIR="${DST_BASE}/${RUNID}"
mkdir -p "${OUTDIR}"

OUTPUT="${OUTDIR}/${BASENAME}.root"

PROFILE_SCRIPT="${THRONG_DIR:-}/config/supernemo_profile.bash"
STACK_NAME="falaise@2026-04-07"
CONF="/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/SIMU/p_MiModule_v00.conf"

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
echo "Outdir: ${OUTDIR}"
echo "Output: ${OUTPUT}"

if [ -f "${OUTPUT}" ]; then
  echo "INFO: Output already exists, skipping: ${OUTPUT}"
  exit 0
fi

WORKDIR=$(mktemp -d "/tmp/miroot_${RUNID}_${SLURM_JOB_ID}_${SLURM_ARRAY_TASK_ID}_XXXXXX")
cleanup() { rm -rf "${WORKDIR}"; }
trap cleanup EXIT
cd "${WORKDIR}"

echo "Running MiModule..."
set +e
"${BIN}" -i "${INFILE}" -p "${CONF}"
RC=$?
set -e

echo "flreconstruct exit code: ${RC}"

if [ "${RC}" -ne 0 ]; then
  echo "ERROR: flreconstruct failed with exit code ${RC}"
  exit "${RC}"
fi

[ -f "Default.root" ] || { echo "ERROR: Default.root not created in ${WORKDIR}"; exit 30; }

mv "Default.root" "${OUTPUT}"

[ -f "${OUTPUT}" ] || { echo "ERROR: Final output not created: ${OUTPUT}"; exit 31; }

echo "Done: ${OUTPUT}"
