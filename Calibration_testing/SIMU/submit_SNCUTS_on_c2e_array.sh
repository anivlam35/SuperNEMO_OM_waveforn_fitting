#!/bin/bash
set -euo pipefail

SRC_BASE="/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/SIMU/brios/SNCUTS/NEW_s_positions"
DST_BASE="/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/SIMU/brios/SNCUTS/NEW_s_positions/e_600-800"

mkdir -p "${DST_BASE}"
LIST="${DST_BASE}/brio_list.txt"

# Numbered folders: brios/1/, brios/2/, ... each contains 2D_reco_Bi_N.brio
find "${SRC_BASE}" -mindepth 2 -maxdepth 2 -type f \
  -name "2D_reco_Bi_*.brio" \
  | sort > "${LIST}"

N=$(wc -l < "${LIST}")
echo "Found ${N} brio files."
[ "${N}" -gt 0 ] || { echo "No 2D_reco_Bi brio files found."; exit 1; }

WORKER="$(cd "$(dirname "$0")" && pwd)/run_SNCUTS_on_c2e_array.sh"

[ -f "${WORKER}" ] || { echo "ERROR: worker script not found: ${WORKER}"; exit 4; }
[ -x "${WORKER}" ] || { echo "ERROR: worker script is not executable: ${WORKER}"; exit 5; }

sbatch --array=1-"${N}"%400 "${WORKER}" "${LIST}" "${SRC_BASE}" "${DST_BASE}"
#sbatch --array=1-1 "${WORKER}" "${LIST}" "${SRC_BASE}" "${DST_BASE}"
