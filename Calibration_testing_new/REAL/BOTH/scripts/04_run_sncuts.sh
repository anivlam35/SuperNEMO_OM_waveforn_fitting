#!/bin/bash

#SBATCH --job-name=both_sncuts
#SBATCH --mem=3G
#SBATCH --licenses=sps
#SBATCH --time=10:00:00
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --output=logs/sncuts-%A_%a.out
#SBATCH --error=logs/sncuts-%A_%a.err

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

source "${PROJECT_DIR}/config.sh"

IDX="${SLURM_ARRAY_TASK_ID}"

INFILE="$(sed -n "${IDX}p" "${C2E_LIST}")"

[ -f "${INFILE}" ] || {
    echo "ERROR: ${INFILE} not found"
    exit 1
}

REL="${INFILE#${C2E_DIR}/}"
REL_DIR="$(dirname "${REL}")"
BASE="$(basename "${INFILE}" .brio)"

OUTDIR="${SNCUTS_DIR}/${REL_DIR}"
OUTPUT="${OUTDIR}/${BASE}-SNCUTS.brio"

mkdir -p "${OUTDIR}"

echo "=========================================="
echo "SNCuts"
echo "Input : ${INFILE}"
echo "Output: ${OUTPUT}"
echo "=========================================="

if [ -f "${OUTPUT}" ]; then
    echo "Already exists. Skipping."
    exit 0
fi

load_falaise

flreconstruct \
    -i "${INFILE}" \
    -p "${SNCUTS_CONF}" \
    -o "${OUTPUT}"

[ -s "${OUTPUT}" ] || {
    echo "ERROR: SNCuts output is missing or empty"
    exit 30
}

echo "Done."
