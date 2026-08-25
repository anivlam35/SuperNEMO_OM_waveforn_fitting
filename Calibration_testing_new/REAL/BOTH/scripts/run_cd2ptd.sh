#!/bin/bash
#SBATCH --job-name=CD2PTD
#SBATCH --partition=htc
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=16
#SBATCH --mem=16G
#SBATCH --time=24:00:00
#SBATCH --licenses=sps

set -euo pipefail

PROJECT_DIR="/sps/nemo/scratch/ikovalen/OM_waveform_fitting/Calibration_testing_new/REAL/BOTH"
source "${PROJECT_DIR}/config.sh"

CD_FILE="$1"

if [ ! -f "${CD_FILE}" ]; then
    echo "ERROR: CD file not found:"
    echo "${CD_FILE}"
    exit 1
fi

BASENAME=$(basename "${CD_FILE}" .brio)

OUTPUT="${PROJECT_DIR}/results/ptd/${BASENAME/_CD/_PTD}.brio"

echo "======================================"
echo "CD → PTD"
echo "Input : ${CD_FILE}"
echo "Output: ${OUTPUT}"
echo "======================================"

flreconstruct \
    -i "${CD_FILE}" \
    -p "${CD2PTD_CONF}" \
    -o "${OUTPUT}"

echo "DONE: ${OUTPUT}"
