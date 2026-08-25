#!/bin/bash
set -euo pipefail

SRC_BASE="/sps/nemo/scratch/ikovalen/OM_waveform_fitting/Calibration_testing/calibrated_brio/SNCUTS/sncuts_NEW_positions"
DST_BASE="/sps/nemo/scratch/ikovalen/OM_waveform_fitting/Calibration_testing/calibrated_brio/SNCUTS/sncuts_NEW_positions/ROOT"

mkdir -p "${DST_BASE}"
LIST="${DST_BASE}/sncuts_brio_list.txt"

find "${SRC_BASE}" -type f -name "*-SNCUTS.brio" | sort > "${LIST}"

N=$(wc -l < "${LIST}")
echo "Found ${N} SNCUTS brio files."
[ "${N}" -gt 0 ] || { echo "No SNCUTS brio files found."; exit 1; }

WORKER="$(cd "$(dirname "$0")" && pwd)/run_MiModule.sh"

sbatch --array=1-"${N}"%400 "${WORKER}" "${LIST}" "${SRC_BASE}" "${DST_BASE}"
