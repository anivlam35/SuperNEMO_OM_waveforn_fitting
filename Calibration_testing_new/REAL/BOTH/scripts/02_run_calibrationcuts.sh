#!/bin/bash

#SBATCH --job-name=both_calibcuts
#SBATCH --mem=4G
#SBATCH --licenses=sps
#SBATCH --time=01:00:00
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8
#SBATCH --output=logs/calibcuts-%A_%a.out
#SBATCH --error=logs/calibcuts-%A_%a.err

set -euo pipefail

# PROJECT_DIR передаётся из submit-скрипта через --export
if [ -z "${PROJECT_DIR:-}" ]; then
    echo "ERROR: PROJECT_DIR is not set"
    exit 1
fi

source "${PROJECT_DIR}/config.sh"

# ------------------------------------------------------------
# Input
# ------------------------------------------------------------

IDX="${SLURM_ARRAY_TASK_ID}"

INFILE="$(sed -n "${IDX}p" "${CALIBRATIONCUTS_LIST}")"

if [ ! -f "${INFILE}" ]; then
    echo "ERROR: Input does not exist:"
    echo "${INFILE}"
    exit 1
fi

# ------------------------------------------------------------
# Output
# ------------------------------------------------------------

BASE="$(basename "${INFILE}" .brio)"

OUTPUT="${CALIBRATIONCUTS_DIR}/${BASE}_calibrationcuts.root"
TMP="${CALIBRATIONCUTS_DIR}/${BASE}.${SLURM_JOB_ID}_${IDX}.tmp.root"

echo "=========================================="
echo "Calibration cuts"
echo "Job ID : ${SLURM_JOB_ID}"
echo "Array  : ${IDX}"
echo "Input  : ${INFILE}"
echo "Output : ${OUTPUT}"
echo "=========================================="

if [ -f "${OUTPUT}" ]; then
    echo "Already exists. Skipping."
    exit 0
fi

# ------------------------------------------------------------
# Falaise
# ------------------------------------------------------------


echo "flreconstruct: $(which flreconstruct)"
flreconstruct --version

# ------------------------------------------------------------
# Temporary working directory
# ------------------------------------------------------------

WORKDIR="$(mktemp -d "/tmp/calibcuts_${SLURM_JOB_ID}_${IDX}_XXXXXX")"

cleanup()
{
    rm -rf "${WORKDIR}"
}

trap cleanup EXIT

cd "${WORKDIR}"

echo "Working directory: ${WORKDIR}"

# ------------------------------------------------------------
# Run reconstruction
# ------------------------------------------------------------

flreconstruct \
    -i "${INFILE}" \
    -p "${CALIBRATIONCUTS_CONF}"

# ------------------------------------------------------------
# Check result
# ------------------------------------------------------------

if [ ! -s extracted_data.root ]; then
    echo "ERROR: extracted_data.root was not produced"
    exit 30
fi

mv extracted_data.root "${TMP}"
mv "${TMP}" "${OUTPUT}"

echo "Done: ${OUTPUT}"
