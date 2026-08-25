#!/bin/bash
set -euo pipefail

SRC_BASE="/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/NOCALIB/calibrated_brio/SNCUTS/sncuts_NEW_positions"
DST_BASE="/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/NOCALIB/calibrated_brio/SNCUTS/sncuts_NEW_positions/ROOT"

mkdir -p "${DST_BASE}"
LIST="${DST_BASE}/sncuts_brio_list.txt"

find "${SRC_BASE}" -type f \
  -path "${SRC_BASE}/week_2025_X[1-7]/bi207/*/*-SNCUTS.brio" \
  | sort > "${LIST}"

N=$(wc -l < "${LIST}")
echo "Found ${N} SNCUTS brio files."
[ "${N}" -gt 0 ] || { echo "No SNCUTS brio files found."; exit 1; }

WORKER="$(cd "$(dirname "$0")" && pwd)/run_MiModule.sh"

sbatch --array=1-"${N}"%100 "${WORKER}" "${LIST}" "${SRC_BASE}" "${DST_BASE}"
