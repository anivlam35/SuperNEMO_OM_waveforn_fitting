#!/bin/bash
set -euo pipefail

SRC_BASE="/sps/nemo/scratch/ikovalen/OM_waveform_fitting/Calibration_testing/build"
DST_BASE="/sps/nemo/scratch/ikovalen/OM_waveform_fitting/Calibration_testing/build/calibrationcuts"

mkdir -p "${DST_BASE}"
LIST="${DST_BASE}/ptd_brio_list_for_calibrationcuts.txt"

find "${SRC_BASE}" -type f \
  -path "${SRC_BASE}/*_PTD.brio" \
  | sort > "${LIST}"

N=$(wc -l < "${LIST}")
echo "Found ${N} PTD brio files."
[ "${N}" -gt 0 ] || { echo "No PTD brio files found."; exit 1; }

WORKER="$(cd "$(dirname "$0")" && pwd)/run_calibrationcuts_root.sh"

sbatch --array=1-"${N}"%400 "${WORKER}" "${LIST}" "${SRC_BASE}" "${DST_BASE}"
#sbatch --array=1-1 "${WORKER}" "${LIST}" "${SRC_BASE}" "${DST_BASE}"
