#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

source "${PROJECT_DIR}/config.sh"

if [ $# -ne 1 ]; then
    echo "Usage:"
    echo "  $0 /path/to/file-SNCUTS.brio"
    exit 2
fi

INFILE="$1"

[ -f "${INFILE}" ] || {
    echo "ERROR: file does not exist:"
    echo "${INFILE}"
    exit 1
}

sbatch \
    "${SCRIPT_DIR}/06_run_600_800.sh" \
    "${INFILE}"
