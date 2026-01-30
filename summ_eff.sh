#!/bin/bash

# Define the main data folder and summary log file
DATA_FOLDER=/sps/nemo/scratch/ktrofimi/Attempt/data_folder
SUMMARY_LOG=$DATA_FOLDER/final_summary_log.txt

# Clear the summary log file if it exists, or create it if it doesn't
> $SUMMARY_LOG

# Find the last user folder (ignore final_summary_log.txt)
LAST_USER_FOLDER=$(ls -d $DATA_FOLDER/*/ 2>/dev/null | sort -V | tail -n 1)

# Check if the LAST_USER_FOLDER is indeed a directory
if [ ! -d "$LAST_USER_FOLDER" ]; then
    echo "No user folders found in $DATA_FOLDER."
    exit 1
fi

# Find the last file folder within the last user folder
LAST_FILE_FOLDER=$(ls -d $LAST_USER_FOLDER*/ 2>/dev/null | sort -V | tail -n 1)

# Check if the LAST_FILE_FOLDER is indeed a directory
if [ ! -d "$LAST_FILE_FOLDER" ]; then
    echo "No file folders found in $LAST_USER_FOLDER."
    exit 1
fi

# Define the path to the OUT.log file
OUT_LOG_FILE=$(ls $LAST_FILE_FOLDER/OUT_*.log 2>/dev/null | sort -V | tail -n 1)

# Check if the OUT.log file exists and append its content to the summary log
if [ -f "$OUT_LOG_FILE" ]; then
    echo "Efficiencies from $LAST_FILE_FOLDER:" >> $SUMMARY_LOG
    grep "eps" $OUT_LOG_FILE >> $SUMMARY_LOG
    echo "Summary log created at $SUMMARY_LOG"
else
    echo "OUT.log file not found in $LAST_FILE_FOLDER"
fi
