#!/bin/bash
#SBATCH --job-name=Ivans_job
#SBATCH --output=output_%j.log
#SBATCH --error=error_%j.log
#SBATCH --time=24:00:00
#SBATCH --partition=htc
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=16
#SBATCH --mem=32G
#SBATCH --licenses sps

# Go to the directory where you submitted the job
cd $SLURM_SUBMIT_DIR

flreconstruct -i /sps/nemo/snemo/snemo_data/reco_data/UDD/delta-tdc-1600us-v3/snemo_run-2011_udd.brio -p /sps/nemo/scratch/ikovalen/OM_waveform_fitting/Calibration_testing/U_PIPELINE_MODULES/1D-pipeline.conf -o /sps/nemo/scratch/ikovalen/OM_waveform_fitting/Calibration_testing/build/snemo_run-2011_cd.brio
