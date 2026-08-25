#!/bin/bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
source "${PROJECT_DIR}/config.sh"

mkdir -p "${PROJECT_DIR}/results/ptd"
mkdir -p "${PROJECT_DIR}/logs/cd2ptd"

CD_FILE="${PROJECT_DIR}/results/udd2cd/snemo_run-2011_udd_CD.brio"

echo "CD file:"
echo "  ${CD_FILE}"

if [ ! -f "${CD_FILE}" ]; then
    echo "ERROR: CD file not found:"
    echo "${CD_FILE}"
    exit 1
fi

echo "Submitting CD → PTD..."

sbatch \
    --job-name=CD2PTD_2011 \
    --output="${PROJECT_DIR}/logs/cd2ptd/%j.out" \
    --error="${PROJECT_DIR}/logs/cd2ptd/%j.err" \
    "${PROJECT_DIR}/scripts/run_cd2ptd.sh" \
    "${CD_FILE}"
