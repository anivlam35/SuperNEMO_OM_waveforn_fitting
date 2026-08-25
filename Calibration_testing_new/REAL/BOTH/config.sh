#!/bin/bash

# ============================================================
# REAL / BOTH — central configuration
# ============================================================

set -euo pipefail

# ------------------------------------------------------------
# Project
# ------------------------------------------------------------

PROJECT_DIR="/sps/nemo/scratch/ikovalen/OM_waveform_fitting/Calibration_testing_new/REAL/BOTH"

# ------------------------------------------------------------
# Input real data
# ------------------------------------------------------------

# Original PTD files
INPUT_PTD="/sps/nemo/scratch/ikovalen/OM_waveform_fitting/Calibration_testing_new/REAL/BOTH/results/ptd"

# ------------------------------------------------------------
# Falaise
# ------------------------------------------------------------

STACK_NAME="falaise@2026-06-19"

# ------------------------------------------------------------
# Configuration files
# ------------------------------------------------------------

CONFIG_DIR="${PROJECT_DIR}/config"

INPUT_UDD="/sps/nemo/snemo/snemo_data/reco_data/UDD/delta-tdc-1600us-v3"

UDD2CD_CONF="${PROJECT_DIR}/config/1D-pipeline.conf"
CD2PTD_CONF="${PROJECT_DIR}/config/2D-CMRMN-CPT-GT-pipeline.conf"

CHARGE2ENERGY_CONF="${CONFIG_DIR}/charge2energy-BOTH.conf"
CALIBRATIONCUTS_CONF="${CONFIG_DIR}/calibration_cuts.conf"
SNCUTS_CONF="${CONFIG_DIR}/SNCuts-pipeline.conf"
SNCUTS_600_800_CONF="${CONFIG_DIR}/SNCuts_600-800.conf"
MIMODULE_CONF="${CONFIG_DIR}/p_MiModule_v00.conf"

# ------------------------------------------------------------
# Calibration constants
# ------------------------------------------------------------

CALIB_CONSTANTS="${PROJECT_DIR}/calib_constants-BOTH.csv"
SOURCE_POSITIONS="${PROJECT_DIR}/config/source_positions_NEW.txt"

# ------------------------------------------------------------
# Lists
# ------------------------------------------------------------

LIST_DIR="${PROJECT_DIR}/lists"

PTD_LIST="${LIST_DIR}/ptd.txt"
CALIBRATIONCUTS_LIST="${LIST_DIR}/calibrationcuts.txt"
C2E_LIST="${LIST_DIR}/c2e.txt"
SNCUTS_LIST="${LIST_DIR}/sncuts.txt"

# ------------------------------------------------------------
# Results
# ------------------------------------------------------------

RESULTS_DIR="${PROJECT_DIR}/results"

CALIBRATIONCUTS_DIR="${RESULTS_DIR}/calibrationcuts"
C2E_DIR="${RESULTS_DIR}/c2e"
SNCUTS_DIR="${RESULTS_DIR}/sncuts"
ROOT_DIR="${RESULTS_DIR}/root"
SELECTED_600_800_DIR="${RESULTS_DIR}/selected_600_800"

# ------------------------------------------------------------
# Logs
# ------------------------------------------------------------

LOG_DIR="${PROJECT_DIR}/logs"

# ------------------------------------------------------------
# Analysis
# ------------------------------------------------------------

ANALYSIS_DIR="${PROJECT_DIR}/analysis"

# ------------------------------------------------------------
# Create required directories
# ------------------------------------------------------------

mkdir -p \
    "${LIST_DIR}" \
    "${RESULTS_DIR}" \
    "${CALIBRATIONCUTS_DIR}" \
    "${C2E_DIR}" \
    "${SNCUTS_DIR}" \
    "${ROOT_DIR}" \
    "${SELECTED_600_800_DIR}" \
    "${LOG_DIR}"

# ------------------------------------------------------------
# Helper
# ------------------------------------------------------------

load_falaise()
{
    set +e
    set +u
    source "${THRONG_DIR}/config/supernemo_profile.bash"
    snswmgr_load_stack "${STACK_NAME}"
    set -u
    set -e
}

# ------------------------------------------------------------
# Show configuration
# ------------------------------------------------------------

show_config()
{
    echo
    echo "========== REAL/BOTH configuration =========="
    echo "PROJECT_DIR       = ${PROJECT_DIR}"
    echo "INPUT_PTD         = ${INPUT_PTD}"
    echo "C2E_DIR           = ${C2E_DIR}"
    echo "SNCUTS_DIR        = ${SNCUTS_DIR}"
    echo "ROOT_DIR          = ${ROOT_DIR}"
    echo "CALIBRATIONCUTS   = ${CALIBRATIONCUTS_DIR}"
    echo "=============================================="
    echo
}
