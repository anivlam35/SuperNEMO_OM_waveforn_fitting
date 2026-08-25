#!/bin/bash

#SBATCH --job-name=both_600_800
#SBATCH --mem=3G
#SBATCH --licenses=sps
#SBATCH --time=03:00:00
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --output=logs/600-800-%j.out
#SBATCH --error=logs/600-800-%j.err

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

source "${PROJECT_DIR}/config.sh"

[ $# -eq 1 ] || {
    echo "Usage: $0 SNCUTS_FILE"
    exit 2
}

INFILE="$1"

[ -f "${INFILE}" ] || {
    echo "ERROR: input does not exist:"
    echo "${INFILE}"
    exit 1
}

BASE="$(basename "${INFILE}" .brio)"
OUTPUT="${SELECTED_600_800_DIR}/${BASE}_600-800.brio"

mkdir -p "${SELECTED_600_800_DIR}"

load_falaise

flreconstruct \
    -i "${INFILE}" \
    -p "${SNCUTS_600_800_CONF}" \
    -o "${OUTPUT}"

[ -s "${OUTPUT}" ] || {
    echo "ERROR: output missing or empty"
    exit 30
}

echo "Done:"
echo "${OUTPUT}"
