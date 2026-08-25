#!/bin/bash
set -euo pipefail

SRC_BASE="/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/BETE/calibrated_brio"
DST_BASE="/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/BETE/calibrated_brio/SNCUTS/sncuts_NEW_positions"

mkdir -p "${DST_BASE}"
LIST="${DST_BASE}/c2e_brio_list.txt"

find "${SRC_BASE}" -type f \
  -path "${SRC_BASE}/week_2025_X[1-7]/bi207/*/*_c2e.brio" \
  | sort > "${LIST}"

N=$(wc -l < "${LIST}")
echo "Found ${N} c2e brio files."
[ "${N}" -gt 0 ] || { echo "No c2e brio files found."; exit 1; }

WORKER="$(cd "$(dirname "$0")" && pwd)/run_SNCUTS_on_c2e_array.sh"

sbatch --array=1-"${N}"%400 "${WORKER}" "${LIST}" "${SRC_BASE}" "${DST_BASE}"
#sbatch --array=1-1 "${WORKER}" "${LIST}" "${SRC_BASE}" "${DST_BASE}"
