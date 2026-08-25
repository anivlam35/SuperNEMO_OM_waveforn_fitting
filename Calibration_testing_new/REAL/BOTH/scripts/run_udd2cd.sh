#!/bin/bash
#SBATCH --job-name=UDD2CD
#SBATCH --partition=htc
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=16
#SBATCH --mem=16G
#SBATCH --time=24:00:00
#SBATCH --licenses=sps

set -eo pipefail

PROJECT_DIR="/sps/nemo/scratch/ikovalen/OM_waveform_fitting/Calibration_testing_new/REAL/BOTH"

source "${PROJECT_DIR}/config.sh"

UDD_FILE="${UDD_INPUT}"

if [ ! -f "${UDD_FILE}" ]; then
    echo "ERROR: UDD file not found:"
    echo "${UDD_FILE}"
    exit 1
fi

BASENAME=$(basename "${UDD_FILE}" .brio)

OUTPUT="${PROJECT_DIR}/results/udd2cd/${BASENAME/_udd/_udd_CD}.brio"

echo "======================================"
echo "UDD → CD"
echo "Input : ${UDD_FILE}"
echo "Output: ${OUTPUT}"
echo "======================================"

echo "flreconstruct: $(which flreconstruct)"
flreconstruct --version

flreconstruct \
    -i "${UDD_FILE}" \
    -p "${UDD2CD_CONF}" \
    -o "${OUTPUT}"

echo "DONE: ${OUTPUT}"
