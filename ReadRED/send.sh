#!/bin/bash
#SBATCH --job-name=Ivans_job
#SBATCH --output=output_%j.log
#SBATCH --error=error_%j.log
#SBATCH --time=24:00:00
#SBATCH --partition=compute
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=16
#SBATCH --mem=32G

# Activate environment if needed
# source ~/miniconda3/bin/activate myenv

# Go to the directory where you submitted the job
cd $SLURM_SUBMIT_DIR

"$@"
