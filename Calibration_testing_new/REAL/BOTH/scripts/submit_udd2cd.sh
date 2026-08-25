#!/bin/bash

PROJECT_DIR="/sps/nemo/scratch/ikovalen/OM_waveform_fitting/Calibration_testing_new/REAL/BOTH"

source "${PROJECT_DIR}/config.sh"

INPUT_FILE="/sps/nemo/snemo/snemo_data/reco_data/UDD/delta-tdc-1600us-v3/snemo_run-2011_udd.brio"

if [ ! -f "${INPUT_FILE}" ]; then
    echo "ERROR: File not found:"
    echo "${INPUT_FILE}"
    exit 1
fi

mkdir -p "${PROJECT_DIR}/logs/udd2cd"
mkdir -p "${PROJECT_DIR}/results/udd2cd"

echo "Submitting:"
echo "${INPUT_FILE}"

sbatch \
    --job-name=UDD2CD_2011 \
    --output="${PROJECT_DIR}/logs/udd2cd/%j.out" \
    --error="${PROJECT_DIR}/logs/udd2cd/%j.err" \
    --export=ALL,UDD_INPUT="${INPUT_FILE}" \
    "${PROJECT_DIR}/scripts/run_udd2cd.sh"
