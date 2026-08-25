#!/bin/bash
set -euo pipefail

SRC_BASE="/sps/nemo/scratch/ikovalen/OM_waveform_fitting/Calibration_testing/build"
DST_BASE="/sps/nemo/scratch/ikovalen/OM_waveform_fitting/Calibration_testing/calibrated_brio"

mkdir -p "${DST_BASE}"

LIST="${DST_BASE}/brio_list.txt"

find "${SRC_BASE}" -type f -name "*_PTD.brio" | sort > "${LIST}"

N=$(wc -l < "${LIST}")
echo "Found ${N} PTD brio files."

[ "${N}" -gt 0 ] || {
    echo "No PTD brio files found."
    exit 1
}

WORKER="$(cd "$(dirname "$0")" && pwd)/run_calibrate_using_BOTH-const_array.sh"

sbatch --array=1-"${N}"%400 "${WORKER}" "${LIST}" "${SRC_BASE}" "${DST_BASE}"
