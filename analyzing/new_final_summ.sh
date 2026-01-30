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

# Initialize variables to store cumulative efficiency data
total_entries=0
total_passed1=0
total_passed2=0
total_passed3=0
total_passed4=0
folders_with_data=0

# Function to parse OUT.log files and extract entries and passed counts
parse_output_log() {
    local logfile=$1
    local output_log="$logfile"

    # Debug: Print the content of the logfile
    echo "Parsing $logfile:"
    cat "$output_log"
    echo "--------------------------------------------------"

    # Extract entries and passed counts from the output log
    local entries=$(grep "Total entries:" "$output_log" | awk '{print $4}')
    local passed1=$(grep "Passed1:" "$output_log" | awk '{print $2}')
    local passed2=$(grep "Passed2:" "$output_log" | awk '{print $2}')
    local passed3=$(grep "Passed3:" "$output_log" | awk '{print $2}')
    local passed4=$(grep "Passed4:" "$output_log" | awk '{print $2}')

    # Debug: Print the extracted values
    echo "Extracted values - entries: $entries, passed1: $passed1, passed2: $passed2, passed3: $passed3, passed4: $passed4"

    if [ -n "$entries" ] && [ -n "$passed1" ] && [ -n "$passed2" ] && [ -n "$passed3" ] && [ -n "$passed4" ]; then
        total_entries=$((total_entries + entries))
        total_passed1=$((total_passed1 + passed1))
        total_passed2=$((total_passed2 + passed2))
        total_passed3=$((total_passed3 + passed3))
        total_passed4=$((total_passed4 + passed4))
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

# Calculate overall efficiencies and their errors if at least one log file was parsed
if [ $folders_with_data -gt 0 ]; then
    overall_eps1=$(echo "scale=8; (100.0 * $total_passed1) / $total_entries" | bc)
    overall_eps2=$(echo "scale=8; (100.0 * $total_passed2) / $total_entries" | bc)
    overall_eps3=$(echo "scale=8; (100.0 * $total_passed3) / $total_entries" | bc)
    overall_eps4=$(echo "scale=8; (100.0 * $total_passed4) / $total_entries" | bc)

    error_eps1=$(echo "scale=8; sqrt($overall_eps1 * (100.0 - $overall_eps1) / $total_entries)" | bc)
    error_eps2=$(echo "scale=8; sqrt($overall_eps2 * (100.0 - $overall_eps2) / $total_entries)" | bc)
    error_eps3=$(echo "scale=8; sqrt($overall_eps3 * (100.0 - $overall_eps3) / $total_entries)" | bc)
    error_eps4=$(echo "scale=8; sqrt($overall_eps4 * (100.0 - $overall_eps4) / $total_entries)" | bc)

    # Write the overall efficiencies and their errors to the summary log
    echo "Overall Efficiencies:" >> $SUMMARY_LOG
    echo "eps1 = $overall_eps1% ± $error_eps1%" >> $SUMMARY_LOG
    echo "eps2 = $overall_eps2% ± $error_eps2%" >> $SUMMARY_LOG
    echo "eps3 = $overall_eps3% ± $error_eps3%" >> $SUMMARY_LOG
    echo "eps4 = $overall_eps4% ± $error_eps4%" >> $SUMMARY_LOG
else
    echo "No valid data found in any of the output log files."
fi

# Output the summary log
cat $SUMMARY_LOG
