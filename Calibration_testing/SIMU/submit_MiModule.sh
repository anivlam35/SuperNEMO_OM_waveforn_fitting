#!/bin/bash
set -euo pipefail

SRC_BASE="/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/SIMU/brios/SNCUTS/NEW_s_positions/e_600-800"
DST_BASE="/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/SIMU/brios/SNCUTS/NEW_s_positions/ROOT/e_600-800"
mkdir -p "${DST_BASE}"

LIST="${DST_BASE}/brio_list.txt"
find "${SRC_BASE}" -mindepth 2 -maxdepth 2 -type f \
  -name "2D_reco_Bi_*.brio" \
  | sort > "${LIST}"

N=$(wc -l < "${LIST}")
echo "Found ${N} brio files."
[ "${N}" -gt 0 ] || { echo "No brio files found."; exit 1; }

WORKER="$(cd "$(dirname "$0")" && pwd)/run_MiModule.sh"
[ -f "${WORKER}" ] || { echo "ERROR: worker not found: ${WORKER}"; exit 4; }
[ -x "${WORKER}" ] || { echo "ERROR: worker not executable: ${WORKER}"; exit 5; }

sbatch --array=1-"${N}"%450 "${WORKER}" "${LIST}" "${SRC_BASE}" "${DST_BASE}"
#sbatch --array=1-1 "${WORKER}" "${LIST}" "${SRC_BASE}" "${DST_BASE}"
