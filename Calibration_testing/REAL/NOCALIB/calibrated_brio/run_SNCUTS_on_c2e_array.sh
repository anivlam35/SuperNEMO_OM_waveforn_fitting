#!/bin/bash
#SBATCH --job-name=sncuts
#SBATCH --mem=3G
#SBATCH --licenses=sps
#SBATCH --time=10:00:00
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --output=slurm-sncuts-%A_%a.out
#SBATCH --error=slurm-sncuts-%A_%a.err

set -euo pipefail

[ $# -eq 3 ] || { echo "ERROR: Usage: $0 LISTFILE SRC_BASE DST_BASE"; exit 2; }

LISTFILE="$1"
SRC_BASE="$2"
DST_BASE="$3"

[ -n "${LISTFILE}" ] || { echo "ERROR: LISTFILE is empty"; exit 3; }
[ -n "${SRC_BASE}" ] || { echo "ERROR: SRC_BASE is empty"; exit 3; }
[ -n "${DST_BASE}" ] || { echo "ERROR: DST_BASE is empty"; exit 3; }
[ -f "${LISTFILE}" ] || { echo "ERROR: LISTFILE does not exist: ${LISTFILE}"; exit 3; }
[ -d "${SRC_BASE}" ] || { echo "ERROR: SRC_BASE does not exist: ${SRC_BASE}"; exit 3; }

[ -n "${SLURM_ARRAY_TASK_ID:-}" ] || { echo "ERROR: SLURM_ARRAY_TASK_ID is not set"; exit 6; }

IDX="${SLURM_ARRAY_TASK_ID}"
INFILE=$(sed -n "${IDX}p" "${LISTFILE}")
[ -n "${INFILE}" ] || { echo "ERROR: Empty infile for index ${IDX}"; exit 1; }

case "${INFILE}" in
  ${SRC_BASE}/*) ;;
  *)
    echo "ERROR: INFILE is not under SRC_BASE"
    echo "SRC_BASE=${SRC_BASE}"
    echo "INFILE=${INFILE}"
    exit 4
    ;;
esac

case "$(basename "${INFILE}")" in
  *_c2e.brio) ;;
  *)
    echo "ERROR: Input file is not a c2e brio: ${INFILE}"
    exit 5
    ;;
esac

RUNID=$(basename "$(dirname "${INFILE}")")
BASENAME=$(basename "${INFILE}" .brio)

REL="${INFILE#${SRC_BASE}/}"
REL_DIR=$(dirname "${REL}")
OUTDIR="${DST_BASE}/${REL_DIR}"
mkdir -p "${OUTDIR}"

INPUT_DIR_REAL=$(realpath -m "$(dirname "${INFILE}")")
OUTDIR_REAL=$(realpath -m "${OUTDIR}")
SRC_REAL=$(realpath -m "${SRC_BASE}")

if [ "${OUTDIR_REAL}" = "${INPUT_DIR_REAL}" ]; then
  echo "ERROR: OUTDIR is the same as input directory. Refusing to run."
  echo "INPUT_DIR_REAL=${INPUT_DIR_REAL}"
  echo "OUTDIR_REAL=${OUTDIR_REAL}"
  exit 11
fi

cd "${OUTDIR}"

PROFILE_SCRIPT="${THRONG_DIR:-}/config/supernemo_profile.bash"
STACK_NAME="falaise@2026-02-09"
CONF="/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/BOTH/calibrated_brio/SNCuts-pipeline.conf"

[ -f "${INFILE}" ]         || { echo "ERROR: Input file does not exist: ${INFILE}"; exit 101; }
[ -n "${THRONG_DIR:-}" ]   || { echo "ERROR: THRONG_DIR is not set"; exit 106; }
[ -f "${PROFILE_SCRIPT}" ] || { echo "ERROR: SuperNEMO profile script does not exist: ${PROFILE_SCRIPT}"; exit 105; }
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
echo "DEBUG: THRONG_DIR      = ${THRONG_DIR}"
echo "DEBUG: STACK_NAME      = ${STACK_NAME}"
echo "DEBUG: SRC_REAL        = ${SRC_REAL}"
echo "DEBUG: INPUT_DIR_REAL  = ${INPUT_DIR_REAL}"
echo "DEBUG: OUTDIR_REAL     = ${OUTDIR_REAL}"
echo "DEBUG: which flreconstruct = ${BIN}"
echo "DEBUG: CONF exists?    $( [ -f "${CONF}" ] && echo yes || echo no )"

[ "${PROFILE_RC}" -eq 0 ] || { echo "ERROR: sourcing profile failed"; exit 107; }
[ "${STACK_RC}"   -eq 0 ] || { echo "ERROR: snswmgr_load_stack failed"; exit 108; }
[ -n "${BIN}" ]            || { echo "ERROR: flreconstruct not found in PATH after stack load"; exit 102; }
[ -x "${BIN}" ]            || { echo "ERROR: flreconstruct is not executable: ${BIN}"; exit 103; }

OUTPUT="${OUTDIR}/${BASENAME}-SNCUTS.brio"

echo "Input : ${INFILE}"
echo "Outdir: ${OUTDIR_REAL}"
echo "RunID : ${RUNID}"
echo "Base  : ${BASENAME}"
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
