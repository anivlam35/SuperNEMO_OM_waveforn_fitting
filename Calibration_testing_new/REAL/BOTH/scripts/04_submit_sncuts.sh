#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

source "${PROJECT_DIR}/config.sh"

find "${C2E_DIR}" \
    -type f \
    -name "*_c2e.brio" \
    | sort > "${C2E_LIST}"

N=$(wc -l < "${C2E_LIST}")

echo "Found ${N} C2E files."

[ "${N}" -gt 0 ] || {
    echo "ERROR: No C2E files found."
    exit 1
}

sbatch \
    --array=1-"${N}"%400 \
    "${SCRIPT_DIR}/04_run_sncuts.sh"
