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
#1) CD -> PTD
##############################################################################
/sps/nemo/scratch/chauveau/software/falaise/develop/install/bin/flreconstruct \
    -i /.../CD_1556.brio \ #full path to the file
    -p /sps/nemo/scratch/ykozina/Falaise/calibration/Calibration_Constants_Extraction/reco.conf \ #full path to the file
    -o /.../reco-PTD_1556-100.brio #full path to the file
    
##############################################################################    
#2) CalibrationCutsModule (produces root file)
#flreconstruct -i /path/to/input.brio -p /path/to/config.conf 
##############################################################################

#/sps/nemo/scratch/chauveau/software/falaise/develop/install/bin/flreconstruct \
#	-i /.../reco-PTD_1556-100.brio \ 
#	-p /.../calibration_cuts.conf

##############################################################################
#3) SNCalib (extracts calibration constants from root and saves in csv)
##############################################################################

#/sps/nemo/scratch/ykozina/Falaise/tutorial/.../build/SNCalib/sncalib \
#	-i extracted_data-1556.root \
#	-o /.../output-1556.csv \
#	-p /.../build/SNCalib/params.conf -s -V


#end
