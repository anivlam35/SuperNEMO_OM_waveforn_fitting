#!/bin/sh

# SLURM options:

#SBATCH --partition=htc                  # Partition choice (most generally we work with htc, but for quick debugging you can use
										 #					 #SBATCH --partition=flash. This avoids waiting times, but is limited to 1hr)
#SBATCH --mem=16G                     	 # RAM
#SBATCH --licenses=sps                   # When working on sps, must declare license!!

#SBATCH --time=4-0                 	 # Time for the job in format “minutes:seconds” or  “hours:minutes:seconds”, “days-hours”
#SBATCH --cpus-per-task=1                # Number of CPUs

source ${THRONG_DIR}/config/supernemo_profile.bash
snswmgr_load_stack base@2024-09-04
snswmgr_load_setup falaise@5.1.2

echo "================================="
echo "run_script started:"
start=`date +%s`

echo "Working in directory: "
pwd
echo "================================="
echo "STARTING simulation!"

flsimulate -c %DATA_FOLDER/%USER_FOLDNAME/%f/simu_%ISO.conf -o %DATA_FOLDER/%USER_FOLDNAME/%f/simu_%ISO.brio
flreconstruct -i %DATA_FOLDER/%USER_FOLDNAME/%f/simu_%ISO.brio -p /sps/nemo/sw/Falaise/install_develop/share//Falaise-4.1.0/resources/snemo/demonstrator/reconstruction/official-2.0.0.conf -o %DATA_FOLDER/%USER_FOLDNAME/%f/reco_%ISO.brio
flreconstruct -i %DATA_FOLDER/%USER_FOLDNAME/%f/reco_%ISO.brio -p %MIRO_MODULE/testing_products/p_MiModule_v00.conf -o %DATA_FOLDER/%USER_FOLDNAME/%f/MiModule.root
#mv %MIRO_MODULE/testing_products/Default.root %DATA_FOLDER/%USER_FOLDNAME/%f/MiModule.root

echo "================================="
echo "FINISHED simulation, STARTING analysis!"

#root %DATA_FOLDER/%USER_FOLDNAME/%f/analyze.cpp

# rm %DATA_FOLDER/%USER_FOLDNAME/%f/simu_%ISO.brio
# rm %DATA_FOLDER/%USER_FOLDNAME/%f/reco_%ISO.brio