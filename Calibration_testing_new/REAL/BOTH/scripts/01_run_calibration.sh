#!/bin/bash

#SBATCH --job-name=both_calib
#SBATCH --mem=2G
#SBATCH --licenses=sps
#SBATCH --time=10:00:00
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --output=logs/calibration-%A_%a.out
#SBATCH --error=logs/calibration-%A_%a.err

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

source "${PROJECT_DIR}/config.sh"

echo "===== DEBUG ====="
echo "PWD        = $(pwd)"
echo "SCRIPT_DIR = ${SCRIPT_DIR}"
echo "PROJECT_DIR= ${PROJECT_DIR}"
echo "PTD_LIST   = ${PTD_LIST}"
echo "INPUT_PTD  = ${INPUT_PTD}"
echo "C2E_DIR    = ${C2E_DIR}"
echo "SLURM_ARRAY_TASK_ID = ${SLURM_ARRAY_TASK_ID}"
echo "================="
IDX="${SLURM_ARRAY_TASK_ID}"

INFILE="$(sed -n "${IDX}p" "${PTD_LIST}")"

[ -f "${INFILE}" ] || {
    echo "ERROR: Input does not exist:"
    echo "${INFILE}"
    exit 1
}

REL="${INFILE#${INPUT_PTD}/}"
REL_DIR="$(dirname "${REL}")"
BASE="$(basename "${INFILE}" .brio)"

OUTDIR="${C2E_DIR}/${REL_DIR}"
OUTPUT="${OUTDIR}/${BASE}_c2e.brio"

mkdir -p "${OUTDIR}"

echo "=========================================="
echo "Calibration"
echo "Input : ${INFILE}"
echo "Output: ${OUTPUT}"
echo "=========================================="

if [ -f "${OUTPUT}" ]; then
    echo "Already exists. Skipping."
    exit 0
fi

# load_falaise

flreconstruct \
    -i "${INFILE}" \
    -p "${CHARGE2ENERGY_CONF}" \
    -o "${OUTPUT}"

[ -s "${OUTPUT}" ] || {
    echo "ERROR: output was not created or is empty"
    exit 30
}

echo "Done."
