#!/bin/bash

#SBATCH --job-name=both_miroot
#SBATCH --mem=2G
#SBATCH --licenses=sps
#SBATCH --time=10:00:00
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --output=logs/miroot-%A_%a.out
#SBATCH --error=logs/miroot-%A_%a.err

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

source "${PROJECT_DIR}/config.sh"

IDX="${SLURM_ARRAY_TASK_ID}"

INFILE="$(sed -n "${IDX}p" "${SNCUTS_LIST}")"

[ -f "${INFILE}" ] || {
    echo "ERROR: ${INFILE} not found"
    exit 1
}

BASE="$(basename "${INFILE}" .brio)"
OUTPUT="${ROOT_DIR}/${BASE}.root"
TMP="${ROOT_DIR}/${BASE}.${SLURM_JOB_ID}_${IDX}.tmp.root"

echo "=========================================="
echo "MiModule"
echo "Input : ${INFILE}"
echo "Output: ${OUTPUT}"
echo "=========================================="

if [ -f "${OUTPUT}" ]; then
    echo "Already exists. Skipping."
    exit 0
fi

load_falaise

WORKDIR="$(mktemp -d "/tmp/miroot_${SLURM_JOB_ID}_${IDX}_XXXXXX")"

cleanup()
{
    rm -rf "${WORKDIR}"
}

trap cleanup EXIT

cd "${WORKDIR}"

echo "Workdir: ${WORKDIR}"

flreconstruct \
    -i "${INFILE}" \
    -p "${MIMODULE_CONF}"

[ -s Default.root ] || {
    echo "ERROR: MiModule did not create a valid Default.root"
    exit 30
}

mv Default.root "${TMP}"

[ -s "${TMP}" ] || {
    echo "ERROR: temporary ROOT file is empty"
    exit 31
}

mv "${TMP}" "${OUTPUT}"

[ -s "${OUTPUT}" ] || {
    echo "ERROR: final ROOT file is empty"
    exit 32
}

echo "Done:"
echo "${OUTPUT}"
