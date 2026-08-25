#!/bin/bash
# submit_simu.sh
#
# Submits a SLURM array job for Bi-207 flsimulate + flreconstruct.
# Same pattern as submit_calibrate_using_BOTH-const_array.sh:
#   - this script decides HOW MANY jobs to run
#   - passes SIMU_DATA to the worker as argument
#   - SLURM_ARRAY_TASK_ID is used as num (folder index) inside worker
#
# Usage:
#   bash submit_simu.sh FIRST LAST
#
# Example ? simulate events 1..100, each in its own folder:
#   bash submit_simu.sh 1 100

set -euo pipefail

# ============================================================
# ARGUMENTS
# ============================================================
[ $# -eq 2 ] || { echo "ERROR: Usage: $0 FIRST LAST"; exit 2; }

FIRST="$1"
LAST="$2"

[ "${FIRST}" -ge 1 ]         || { echo "ERROR: FIRST must be >= 1"; exit 3; }
[ "${LAST}" -ge "${FIRST}" ] || { echo "ERROR: LAST must be >= FIRST"; exit 3; }

# ============================================================
# PATHS
# ============================================================
SIMU_DATA="/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/SIMU/brios"
mkdir -p "${SIMU_DATA}"

WORKER="$(cd "$(dirname "$0")" && pwd)/simu.sh"

[ -f "${WORKER}" ] || { echo "ERROR: worker script not found: ${WORKER}"; exit 4; }
[ -x "${WORKER}" ] || { echo "ERROR: worker script is not executable: ${WORKER}"; exit 5; }

N=$(( LAST - FIRST + 1 ))

echo "========================================"
echo "Submitting Bi-207 simulation array"
echo "FIRST    : ${FIRST}"
echo "LAST     : ${LAST}"
echo "N jobs   : ${N}"
echo "SIMU_DATA: ${SIMU_DATA}"
echo "WORKER   : ${WORKER}"
echo "========================================"

# ============================================================
# SUBMIT ? same pattern as submit_calibrate:
#   sbatch --array=... WORKER ARG1 ARG2 ...
# Max 50 simultaneous jobs ? adjust %50 as needed
# ============================================================
sbatch --array="${FIRST}-${LAST}%400" "${WORKER}" "${SIMU_DATA}"

echo "Submitted array job: indices ${FIRST} to ${LAST} (max 50 simultaneous)."
echo "Output brios: ${SIMU_DATA}/<num>/2D_reco_Bi_<num>.brio"
