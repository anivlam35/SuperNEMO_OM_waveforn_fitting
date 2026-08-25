#!/bin/sh

#SBATCH --job-name=calib_constants_extract
#SBATCH --mem=3G
#SBATCH --licenses=sps
#SBATCH --time=01:00:00
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1

INDEX=$1

source /sps/nemo/scratch/chauveau/software/falaise/develop/this_falaise.sh

OUT_BASE="/sps/nemo/scratch/ykozina/Falaise/tutorial/CalibrationScript/Tutorial" 

#comment each step after it's done and uncomment the next one, keep only one step uncommented when submit the job

##############################################################################
#5) SNCuts 
##############################################################################

/sps/nemo/scratch/chauveau/software/falaise/develop/install/bin/flreconstruct \
    -i reco-PTD_1556-100-c2e.brio \
    -p /sps/nemo/scratch/ykozina/Falaise/tutorial/CalibrationScript/Tutorial/SNCuts-pipeline.conf \
    -o reco-PTD_1556-100-c2e-SNCUTS.brio


##############################################################################
#6) Mimodule (extracts root from brio, needed to make spectra)
##############################################################################

#/sps/nemo/scratch/chauveau/software/falaise/develop/install/bin/flreconstruct \
#-i reco-PTD_2700-100-c2e-SNCUTS.brio \
#-p /sps/nemo/scratch/ykozina/Falaise/tutorial/CalibrationScript/Tutorial/p_MiModule_v00.conf 


#end
