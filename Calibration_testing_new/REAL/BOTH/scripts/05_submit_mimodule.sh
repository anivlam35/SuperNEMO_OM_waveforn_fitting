#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

source "${PROJECT_DIR}/config.sh"

find "${SNCUTS_DIR}" \
    -type f \
    -name "*-SNCUTS.brio" \
    | sort > "${SNCUTS_LIST}"

N=$(wc -l < "${SNCUTS_LIST}")

echo "Found ${N} SNCuts files."

[ "${N}" -gt 0 ] || {
    echo "ERROR: No SNCuts files found."
    exit 1
}

sbatch \
    --array=1-"${N}"%400 \
    "${SCRIPT_DIR}/05_run_mimodule.sh"
