#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

source "${PROJECT_DIR}/config.sh"

find "${INPUT_PTD}" \
    -type f \
    -name '*_PTD.brio' \
    | sort > "${CALIBRATIONCUTS_LIST}"

N=$(wc -l < "${CALIBRATIONCUTS_LIST}")

echo "Found ${N} PTD files."

[ "${N}" -gt 0 ] || {
    echo "ERROR: No PTD files."
    exit 1
}

sbatch \
    --array=1-"${N}"%400 \
    --export=ALL,PROJECT_DIR="${PROJECT_DIR}" \
    "${SCRIPT_DIR}/02_run_calibrationcuts.sh" \
