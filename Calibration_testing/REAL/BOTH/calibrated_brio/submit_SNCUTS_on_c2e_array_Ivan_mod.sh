#!/bin/bash
set -euo pipefail

SRC_BASE="/sps/nemo/scratch/ikovalen/OM_waveform_fitting/Calibration_testing/calibrated_brio"
DST_BASE="/sps/nemo/scratch/ikovalen/OM_waveform_fitting/Calibration_testing/calibrated_brio/SNCUTS/sncuts_NEW_positions"

mkdir -p "${DST_BASE}"

LIST="${DST_BASE}/c2e_brio_list.txt"

find "${SRC_BASE}" -maxdepth 1 -type f -name "*_c2e.brio" | sort > "${LIST}"

N=$(wc -l < "${LIST}")
echo "Found ${N} c2e brio files."

[ "${N}" -gt 0 ] || {
    echo "No c2e brio files found."
    exit 1
}

WORKER="$(cd "$(dirname "$0")" && pwd)/run_SNCUTS_on_c2e_array.sh"

sbatch --array=1-"${N}"%400 "${WORKER}" "${LIST}" "${SRC_BASE}" "${DST_BASE}"

# Для запуска только первого файла:
# sbatch --array=1-1 "${WORKER}" "${LIST}" "${SRC_BASE}" "${DST_BASE}"
