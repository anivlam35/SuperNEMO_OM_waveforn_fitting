#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

source "${PROJECT_DIR}/config.sh"

mkdir -p "${LIST_DIR}" "${C2E_DIR}"

find "${INPUT_PTD}" \
    -type f \
    -name '*_PTD.brio' \
    | sort > "${PTD_LIST}"

N=$(wc -l < "${PTD_LIST}")

echo "Found ${N} PTD files."

if [ "${N}" -eq 0 ]; then
    echo "ERROR: No PTD files found."
    exit 1
fi

WORKER="${SCRIPT_DIR}/01_run_calibration.sh"

echo "Submitting ${N} calibration jobs..."

sbatch \
    --array=1-"${N}"%400 \
    --export=ALL,PROJECT_DIR="${PROJECT_DIR}"\
    "${WORKER}" \
    "${PTD_LIST}"
