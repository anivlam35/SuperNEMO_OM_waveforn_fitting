#!/bin/bash

# Define the main data folder and summary log file
DATA_FOLDER="/sps/nemo/scratch/ktrofimi/Attempt/data_folder"
SUMMARY_LOG="$DATA_FOLDER/final_summary_log.txt"

# Clear the summary log file if it exists, or create it if it doesn't
> $SUMMARY_LOG

# Prompt user for the name of the simulation folder
echo "Enter the name of the simulation folder:"
read USER_FOLDNAME

# Check if the simulation folder exists
if [ ! -d "$DATA_FOLDER/$USER_FOLDNAME" ]; then
    echo "Simulation folder $DATA_FOLDER/$USER_FOLDNAME does not exist."
    exit 1
fi

# Prompt user for the number of files
echo "Enter the number of files:"
read FILES

# Initialize variables to store cumulative efficiency and error data
total_entries=0
total_passed1=0
total_passed2=0
total_passed3=0
total_passed4=0
total_err1=0
total_err2=0
total_err3=0
total_err4=0
folders_with_data=0

# Function to parse OUT.log files and extract efficiencies and errors
parse_output_log() {
    local logfile=$1
    local output_log="$logfile"

    # Extract efficiencies and errors from the output log
    local eps1=$(grep "eps1 = " "$output_log" | awk '{print $3}' | tr -d '%')
    local eps2=$(grep "eps2 = " "$output_log" | awk '{print $3}' | tr -d '%')
    local eps3=$(grep "eps3 = " "$output_log" | awk '{print $3}' | tr -d '%')
    local eps4=$(grep "eps4 = " "$output_log" | awk '{print $3}' | tr -d '%')

    local err1=$(grep "eps1 = " "$output_log" | awk '{print $5}' | tr -d '%')
    local err2=$(grep "eps2 = " "$output_log" | awk '{print $5}' | tr -d '%')
    local err3=$(grep "eps3 = " "$output_log" | awk '{print $5}' | tr -d '%')
    local err4=$(grep "eps4 = " "$output_log" | awk '{print $5}' | tr -d '%')

    if [ -n "$eps1" ] && [ -n "$eps2" ] && [ -n "$eps3" ] && [ -n "$eps4" ] \
       && [ -n "$err1" ] && [ -n "$err2" ] && [ -n "$err3" ] && [ -n "$err4" ]; then
        total_entries=$((total_entries + 1000))  # Assuming 1000 entries per log file
        total_passed1=$(echo "scale=2; $total_passed1 + $eps1" | bc)
        total_passed2=$(echo "scale=2; $total_passed2 + $eps2" | bc)
        total_passed3=$(echo "scale=2; $total_passed3 + $eps3" | bc)
        total_passed4=$(echo "scale=2; $total_passed4 + $eps4" | bc)
        total_err1=$(echo "scale=2; $total_err1 + $err1" | bc)
        total_err2=$(echo "scale=2; $total_err2 + $err2" | bc)
        total_err3=$(echo "scale=2; $total_err3 + $err3" | bc)
        total_err4=$(echo "scale=2; $total_err4 + $err4" | bc)
        folders_with_data=$((folders_with_data + 1))
    else
        echo "Failed to parse $logfile."
    fi
}

# Loop over all the specified files and parse the output logs
for (( f=0; f < $FILES; f++ )); do
    logfile="$DATA_FOLDER/$USER_FOLDNAME/$f/OUT_$f.log"
    if [ -f "$logfile" ]; then
        parse_output_log "$logfile"
    else
        echo "Output log file $logfile does not exist."
    fi
done

# Calculate overall efficiencies if at least one log file was parsed
if [ $folders_with_data -gt 0 ]; then
    overall_eps1=$(echo "scale=7; (1000.0 * $total_passed1) / $total_entries" | bc)
    overall_eps2=$(echo "scale=7; (1000.0 * $total_passed2) / $total_entries" | bc)
    overall_eps3=$(echo "scale=7; (1000.0 * $total_passed3) / $total_entries" | bc)
    overall_eps4=$(echo "scale=7; (1000.0 * $total_passed4) / $total_entries" | bc)

    overall_err1=$(echo "scale=7; $total_err1 / $folders_with_data" | bc)
    overall_err2=$(echo "scale=7; $total_err2 / $folders_with_data" | bc)
    overall_err3=$(echo "scale=7; $total_err3 / $folders_with_data" | bc)
    overall_err4=$(echo "scale=7; $total_err4 / $folders_with_data" | bc)

    # Write the overall efficiencies and errors to the summary log
    echo "Overall Efficiencies:" >> $SUMMARY_LOG
    echo "eps1 = $overall_eps1% +- $overall_err1%" >> $SUMMARY_LOG
    echo "eps2 = $overall_eps2% +- $overall_err2%" >> $SUMMARY_LOG
    echo "eps3 = $overall_eps3% +- $overall_err3%" >> $SUMMARY_LOG
    echo "eps4 = $overall_eps4% +- $overall_err4%" >> $SUMMARY_LOG
else
    echo "No valid data found in any of the output log files."
fi

# Output the summary log
cat $SUMMARY_LOG
