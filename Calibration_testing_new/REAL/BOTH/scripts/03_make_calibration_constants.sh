#!/bin/bash

#SBATCH --job-name=both_sncalib
#SBATCH --mem=8G
#SBATCH --licenses=sps
#SBATCH --time=72:00:00
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --output=logs/sncalib-%j.out
#SBATCH --error=logs/sncalib-%j.err

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

source "${PROJECT_DIR}/config.sh"

INPUT_ROOT="${CALIBRATIONCUTS_DIR}/snemo_run-2011_PTD_calibrationcuts.root"

# ВАЖНО:
# сюда поставь фактический sncalib binary/config из твоей установки.
SNCalib_BIN="/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/Calibration_Constants_Extraction-BOTH/build/SNCalib/sncalib"
SNCalib_CONF="/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/Calibration_Constants_Extraction-BOTH/build/SNCalib/params.conf"

[ -f "${INPUT_ROOT}" ] || {
    echo "ERROR: ${INPUT_ROOT} not found"
    exit 1
}

[ -x "${SNCalib_BIN}" ] || {
    echo "ERROR: sncalib binary not found:"
    echo "${SNCalib_BIN}"
    exit 2
}

[ -f "${SNCalib_CONF}" ] || {
    echo "ERROR: params.conf not found:"
    echo "${SNCalib_CONF}"
    exit 3
}

load_falaise

"${SNCalib_BIN}" \
    -i "${INPUT_ROOT}" \
    -o "${CALIB_CONSTANTS}" \
    -p "${SNCalib_CONF}" \
    -s -V

[ -s "${CALIB_CONSTANTS}" ] || {
    echo "ERROR: calibration constants were not created"
    exit 30
}

echo "Created:"
echo "${CALIB_CONSTANTS}"
