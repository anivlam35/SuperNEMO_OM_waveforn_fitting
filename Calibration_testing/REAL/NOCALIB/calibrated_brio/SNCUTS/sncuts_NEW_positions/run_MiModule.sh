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

[ -n "${LISTFILE}" ] || { echo "ERROR: LISTFILE is empty"; exit 3; }
[ -n "${SRC_BASE}" ] || { echo "ERROR: SRC_BASE is empty"; exit 3; }
[ -n "${DST_BASE}" ] || { echo "ERROR: DST_BASE is empty"; exit 3; }
[ -f "${LISTFILE}" ] || { echo "ERROR: LISTFILE does not exist: ${LISTFILE}"; exit 3; }
[ -d "${SRC_BASE}" ] || { echo "ERROR: SRC_BASE does not exist: ${SRC_BASE}"; exit 3; }

[ -n "${SLURM_ARRAY_TASK_ID:-}" ] || { echo "ERROR: SLURM_ARRAY_TASK_ID is not set"; exit 6; }

IDX="${SLURM_ARRAY_TASK_ID}"
INFILE=$(sed -n "${IDX}p" "${LISTFILE}")
[ -n "${INFILE}" ] || { echo "ERROR: Empty infile for index ${IDX}"; exit 1; }

# --- Safety 0: INFILE must be inside SRC_BASE ---
case "${INFILE}" in
  ${SRC_BASE}/*) ;;
  *)
    echo "ERROR: INFILE is not under SRC_BASE"
    echo "SRC_BASE=${SRC_BASE}"
    echo "INFILE=${INFILE}"
    exit 4
    ;;
esac

# --- Safety 0b: process ONLY *-SNCUTS.brio files ---
case "$(basename "${INFILE}")" in
  *-SNCUTS.brio) ;;
  *)
    echo "ERROR: Input file is not an SNCUTS brio: ${INFILE}"
    exit 5
    ;;
esac

RUNID=$(basename "$(dirname "${INFILE}")")
BASENAME=$(basename "${INFILE}" .brio)   # e.g. 2204_PTD_c2e-SNCUTS

# All ROOT outputs go directly into DST_BASE
OUTDIR="${DST_BASE}"
mkdir -p "${OUTDIR}"

# --- Safety 1: OUTDIR must not be the same as the input directory ---
INPUT_DIR_REAL=$(realpath -m "$(dirname "${INFILE}")")
OUTDIR_REAL=$(realpath -m "${OUTDIR}")
SRC_REAL=$(realpath -m "${SRC_BASE}")

if [ "${OUTDIR_REAL}" = "${INPUT_DIR_REAL}" ]; then
  echo "ERROR: OUTDIR is the same as input directory. Refusing to run."
  echo "INPUT_DIR_REAL=${INPUT_DIR_REAL}"
  echo "OUTDIR_REAL=${OUTDIR_REAL}"
  exit 11
fi

# --- Verify key paths before environment setup ---
PROFILE_SCRIPT="${THRONG_DIR:-}/config/supernemo_profile.bash"
STACK_NAME="falaise@2026-02-09"
CONF="/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/NOCALIB/calibrated_brio/SNCUTS/sncuts_NEW_positions/p_MiModule_v00.conf"

[ -f "${INFILE}" ] || { echo "ERROR: Input file does not exist: ${INFILE}"; exit 101; }
[ -n "${THRONG_DIR:-}" ] || { echo "ERROR: THRONG_DIR is not set"; exit 106; }
[ -f "${PROFILE_SCRIPT}" ] || { echo "ERROR: SuperNEMO profile script does not exist: ${PROFILE_SCRIPT}"; exit 105; }
[ -f "${CONF}" ] || { echo "ERROR: Config does not exist: ${CONF}"; exit 104; }

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
echo "DEBUG: THRONG_DIR      = ${THRONG_DIR}"
echo "DEBUG: STACK_NAME      = ${STACK_NAME}"
echo "DEBUG: SRC_REAL        = ${SRC_REAL}"
echo "DEBUG: INPUT_DIR_REAL  = ${INPUT_DIR_REAL}"
echo "DEBUG: OUTDIR_REAL     = ${OUTDIR_REAL}"
echo "DEBUG: which flreconstruct = ${BIN}"
echo "DEBUG: CONF exists?    $( [ -f "${CONF}" ] && echo yes || echo no )"

[ "${PROFILE_RC}" -eq 0 ] || { echo "ERROR: sourcing profile failed"; exit 107; }
[ "${STACK_RC}" -eq 0 ] || { echo "ERROR: snswmgr_load_stack failed"; exit 108; }
[ -n "${BIN}" ] || { echo "ERROR: flreconstruct not found in PATH after stack load"; exit 102; }
[ -x "${BIN}" ] || { echo "ERROR: flreconstruct is not executable: ${BIN}"; exit 103; }

OUTPUT="${OUTDIR}/${BASENAME}.root"
TMP_OUTPUT="${OUTDIR}/${BASENAME}.${SLURM_JOB_ID}_${SLURM_ARRAY_TASK_ID}.tmp.root"

echo "Input : ${INFILE}"
echo "RunID : ${RUNID}"
echo "Base  : ${BASENAME}"
echo "Outdir: ${OUTDIR_REAL}"
echo "Output: ${OUTPUT}"
echo "Temp  : ${TMP_OUTPUT}"

# Skip if already processed
if [ -f "${OUTPUT}" ]; then
  echo "INFO: Output already exists, skipping: ${OUTPUT}"
  exit 0
fi

SRC_STAT_BEFORE=$(stat -c "%s %Y" "${INFILE}")

# Each job gets its own isolated working directory so Default.root won't collide
WORKDIR=$(mktemp -d "/tmp/miroot_${RUNID}_${SLURM_JOB_ID}_${SLURM_ARRAY_TASK_ID}_XXXXXX")
echo "Workdir: ${WORKDIR}"

cleanup() {
  rm -rf "${WORKDIR}"
}
trap cleanup EXIT

cd "${WORKDIR}"

echo "Running MiModule..."
set +e
"${BIN}" \
  -i "${INFILE}" \
  -p "${CONF}"
RC=$?
set -e

echo "flreconstruct exit code: ${RC}"

if [ "${RC}" -ne 0 ]; then
  echo "ERROR: flreconstruct failed with exit code ${RC}"
  [ -f "${TMP_OUTPUT}" ] && rm -f "${TMP_OUTPUT}"
  exit "${RC}"
fi

if [ ! -f "Default.root" ]; then
  echo "ERROR: MiModule did not create Default.root in ${WORKDIR}"
  exit 30
fi

TMP_LOCAL_SIZE=$(stat -c "%s" "Default.root")
if [ "${TMP_LOCAL_SIZE}" -le 0 ]; then
  echo "ERROR: Default.root is empty in ${WORKDIR}"
  rm -f "Default.root"
  exit 31
fi

mv "Default.root" "${TMP_OUTPUT}"

SRC_STAT_AFTER=$(stat -c "%s %Y" "${INFILE}")
if [ "${SRC_STAT_BEFORE}" != "${SRC_STAT_AFTER}" ]; then
  echo "ERROR: Source brio file metadata changed! (${INFILE})"
  echo "Before: ${SRC_STAT_BEFORE}"
  echo "After : ${SRC_STAT_AFTER}"
  [ -f "${TMP_OUTPUT}" ] && rm -f "${TMP_OUTPUT}"
  exit 20
fi

if [ ! -f "${TMP_OUTPUT}" ]; then
  echo "ERROR: Expected temporary ROOT file was not created: ${TMP_OUTPUT}"
  exit 32
fi

TMP_SIZE=$(stat -c "%s" "${TMP_OUTPUT}")
if [ "${TMP_SIZE}" -le 0 ]; then
  echo "ERROR: Temporary ROOT file is empty: ${TMP_OUTPUT}"
  rm -f "${TMP_OUTPUT}"
  exit 33
fi

mv "${TMP_OUTPUT}" "${OUTPUT}"

if [ ! -f "${OUTPUT}" ]; then
  echo "ERROR: Final output ROOT file was not created: ${OUTPUT}"
  exit 34
fi

OUT_SIZE=$(stat -c "%s" "${OUTPUT}")
if [ "${OUT_SIZE}" -le 0 ]; then
  echo "ERROR: Final output ROOT file is empty: ${OUTPUT}"
  rm -f "${OUTPUT}"
  exit 35
fi

OUT_STAT=$(stat -c "%s %Y" "${OUTPUT}")
echo "Output stat: ${OUT_STAT}"
echo "Done: ${OUTPUT}"
