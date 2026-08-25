#!/bin/bash
set -euo pipefail

SRC_BASE="/sps/nemo/scratch/mpetro/Projects/real_data_analysis_cimrman_filip/data_fl5110_alpha/phase_2"
DST_BASE="/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/BOTH/calibrated_brio"

mkdir -p "${DST_BASE}"
LIST="${DST_BASE}/brio_list.txt"

find "${SRC_BASE}" -type f \
  -path "${SRC_BASE}/week_2025_X[1-7]/bi207/*/*_PTD.brio" \
  | sort > "${LIST}"

N=$(wc -l < "${LIST}")
echo "Found ${N} PTD brio files."
[ "${N}" -gt 0 ] || { echo "No PTD brio files found."; exit 1; }

WORKER="$(cd "$(dirname "$0")" && pwd)/run_calibrate_using_BOTH-const_array.sh"

sbatch --array=1-"${N}"%400 "${WORKER}" "${LIST}" "${SRC_BASE}" "${DST_BASE}"
#sbatch --array=1-1 "${WORKER}" "${LIST}" "${SRC_BASE}" "${DST_BASE}"
