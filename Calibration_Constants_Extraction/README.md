# Calibration_Constants_Extraction

Software and helper scripts for extracting SuperNEMO calorimeter calibration constants from Bi-207 calibration data.

This repository contains the **first stage of the calibration workflow** used in the thesis: the extraction of the linear calibration constants `a` and `b` for each optical module (OM). The produced CSV file is then used in the next stage by `Charge2EnergyModule` to convert measured PMT charge into calibrated electron energy.

Technically, this repository is a compact, separated adaptation of the calibration-constant extraction part of the original `CalibrationTools` package. It was kept as an independent repository to make this stage easier to build, run, test, and repeat separately from the full calibration software chain.

The extraction chain converts reconstructed calibration events into ROOT trees grouped by OM, fits the Bi-207 conversion-electron peaks, and produces a CSV file with the calibration parameters for each OM.

The repository is intended to be used inside the SuperNEMO/Falaise software environment, in particular on CC-IN2P3.

---

## Origin of the software

This repository is based on the original **CalibrationTools** package developed by F. Koňařík for the SuperNEMO calorimeter energy calibration:

- <https://github.com/konarfil/CalibrationTools>

The original package provides the complete calibration software framework. The version stored here is a modified and adapted copy focused specifically on the **calibration-constant extraction stage**. It was separated from the full package for practical use in the thesis workflow, where the extraction of constants had to be run repeatedly for different correction modes and datasets before applying the constants in the following energy-calibration step.

The original calibration method and the core software concept belong to F. Koňařík. The modifications in this repository were introduced to make the extraction stage convenient to use in the local processing chain, including mode-specific builds, configuration changes, CC-IN2P3 batch scripts, and integration with the surrounding calibration-data processing workflow.

In other words, this repository should be understood as a thesis-specific adaptation of the constant-extraction part of `CalibrationTools`, not as an independent reimplementation of the original calibration package.


## Purpose

The SuperNEMO calorimeter energy calibration assumes a linear relation between the charge measured by a photomultiplier tube and the deposited electron energy,

```text
E = a * Q + b
```

where `a` and `b` are OM-dependent calibration constants.

This repository provides a separated implementation of the constant-extraction step:

- a Falaise module, `CalibrationCutsModule`, that selects Bi-207 calibration tracks and stores the information required for calibration;
- an energy-correction calculator used during the calibration fit;
- the `SNCalib` executable, which extracts calibration constants from the ROOT file produced by `CalibrationCutsModule`;
- scripts for running the extraction on many reconstructed `.brio` files and merging the resulting ROOT files.

It does **not** represent the full calibration chain by itself. Its output is the calibration-constant CSV file, which is then passed to the energy-calibration stage.

The output CSV file is later used by the energy-calibration repository:

- <https://github.com/YaKozina/Energy_Calibration>

This repository is also used as part of the full calibration testing pipeline:

- <https://github.com/YaKozina/Calibration_testing>

---

## Repository structure

```text
Calibration_Constants_Extraction/
├── CMakeLists.txt
├── install.sh
├── README.md
├── CalibrationCutsModule/
│   ├── CMakeLists.txt
│   ├── calibration_cuts.conf.in
│   ├── source_positions.txt.in
│   ├── variant.profile.in
│   ├── include/
│   │   └── calibration_cuts_module.h
│   └── src/
│       └── calibration_cuts_module.cc
├── EnergyCorrectionCalculator/
│   ├── CMakeLists.txt
│   ├── include/
│   │   └── energy_correction_calculator.h
│   └── src/
│       └── energy_correction_calculator.cc
├── SNCalib/
│   ├── CMakeLists.txt
│   ├── params.conf.in
│   ├── Fits/
│   │   └── .gitkeep
│   ├── include/
│   │   ├── calib_info.h
│   │   └── calibration_parameter_finder.h
│   └── src/
│       ├── calibration_paremeter_finder.cc
│       └── sncalib.cc
└── scripts/
    ├── submit_calibrationcuts_root.sh
    ├── run_calibrationcuts_root.sh
    ├── merge_calibroots.cpp
    ├── run_csv_make.sh
    ├── send_EXTRACT_CONSTANTS.sh
    ├── reco.conf
    ├── for_CalibrationCutsModule_build/
    │   ├── calibration_cuts.conf
    │   ├── source_positions.txt
    │   └── variant.profile
    └── CalibrationCutsModule_src_CALIBMODELS_VARIATIONS/
        ├── calibration_paremeter_finder_nocalib.cc
        ├── calibration_paremeter_finder_optic.cc
        ├── calibration_paremeter_finder_bete.cc
        └── calibration_paremeter_finder_both.cc
```

---

## Main components

### `CalibrationCutsModule`

`CalibrationCutsModule` is a custom Falaise `flreconstruct` module.

It reads reconstructed events containing the required data banks and produces a ROOT file called:

```text
extracted_data.root
```

The output file contains one ROOT `TTree` per optical module. Each tree stores the quantities needed by `SNCalib`:

| Branch | Meaning |
|--------|---------|
| `charge` | PMT charge measured for the associated OM hit |
| `source_vertex_pos` | reconstructed vertex position at the calibration source |
| `calo_vertex_pos` | reconstructed calorimeter impact position |
| `calo_vertex_pos_OM` | impact position expressed in the local OM coordinate system |

The module is configured through:

```text
CalibrationCutsModule/calibration_cuts.conf.in
```

and uses calibration source positions from:

```text
CalibrationCutsModule/source_positions.txt.in
```

A ready-to-use example configuration is also kept in:

```text
scripts/for_CalibrationCutsModule_build/
```

### Calibration-event selection logic

The module targets single-electron Bi-207 calibration events. The selection is based on reconstructed particle-track information and source/calo trajectory projection points.

The selected event must satisfy the calibration topology expected for conversion electrons:

- the reconstructed source-side vertex must be close to a known Bi-207 calibration source position;
- the source-side vertex is tested inside an ellipse around the nearest calibration source position;
- the reconstructed track must have exactly one associated calorimeter hit;
- the associated calorimeter vertex must be located on a main-wall or X-wall optical module.

The selected information is written to the OM-specific ROOT tree.

---

### `EnergyCorrectionCalculator`

`EnergyCorrectionCalculator` implements the correction functions needed during the calibration fit.

It is used by `SNCalib` to evaluate corrected electron energies while searching for the calibration parameters.

The correction logic includes:

- optical non-linearity correction;
- geometrical non-uniformity correction using SuperNEMO/Falaise resource files;
- Bethe-Bloch energy-loss corrections for detector materials;
- tracking-gas correction using gas pressure, gas composition, and temperature from the configuration file.

The relevant source files are:

```text
EnergyCorrectionCalculator/include/energy_correction_calculator.h
EnergyCorrectionCalculator/src/energy_correction_calculator.cc
```

---

### `SNCalib`

`SNCalib` is the executable that extracts calibration constants from the ROOT file produced by `CalibrationCutsModule`.

It reads a ROOT file containing OM-specific trees, builds the corrected energy spectrum for each OM, fits the Bi-207 conversion-electron peaks, and writes the calibration constants to a CSV file.

The executable is built in:

```text
build/SNCalib/sncalib
```

The configuration file is generated from:

```text
SNCalib/params.conf.in
```

Default parameters:

```text
gas_pressure=0.89
He_pressure=0.955
Et_pressure=0.035
Ar_pressure=0.01
T_gas=298.0
minimization_threshold=1.0
max_iterations=100
min_hits=1000
```

The gas fractions must satisfy:

```text
He_pressure + Et_pressure + Ar_pressure = 1
```

### `SNCalib` output

The output CSV file has the following format:

```text
#OM_number;a;b;chi2_A;chi2_B;loss
```

| Column | Meaning |
|--------|---------|
| `OM_number` | optical module geometry identifier |
| `a` | slope of the linear charge-to-energy calibration |
| `b` | offset of the linear charge-to-energy calibration |
| `chi2_A` | chi-square/NDF of the fit around the first Bi-207 peak |
| `chi2_B` | chi-square/NDF of the fit around the second Bi-207 peak |
| `loss` | minimization loss based on peak-position agreement |

---

## Calibration model used by `SNCalib`

For each trial pair of calibration parameters `a` and `b`, the measured charge is converted to an observed energy:

```text
E_f = a * Q + b
```

Then the correction chain is applied using the reconstructed track and calorimeter-hit geometry.

For main-wall OMs, the correction includes:

```text
optical correction
+ Mylar energy-loss correction
+ nylon energy-loss correction
+ tracker-gas energy-loss correction
```

For X-wall OMs, the correction includes:

```text
optical correction
+ Mylar energy-loss correction
+ tracker-gas energy-loss correction
```

The corrected spectrum is then fitted around the Bi-207 conversion-electron peaks. The calibration parameters are found by minimizing the distance between the fitted peak positions and the expected Bi-207 peak energies.

---

## Correction-model variants

The folder

```text
scripts/CalibrationCutsModule_src_CALIBMODELS_VARIATIONS/
```

contains alternative versions of the calibration-parameter finder source file:

| File | Intended correction mode |
|------|--------------------------|
| `calibration_paremeter_finder_nocalib.cc` | no correction mode |
| `calibration_paremeter_finder_optic.cc` | optical correction only |
| `calibration_paremeter_finder_bete.cc` | Bethe-Bloch energy-loss correction only |
| `calibration_paremeter_finder_both.cc` | optical + Bethe-Bloch corrections |

These files were used to build separate calibration-extraction versions for different correction modes. To use one of them, replace the active `SNCalib/src/calibration_paremeter_finder.cc` with the desired variant before building.

---

## Build instructions

### 1. Load the SuperNEMO/Falaise environment

On CC-IN2P3, load the SuperNEMO software stack before building. For example:

```bash
source "${THRONG_DIR}/config/supernemo_profile.bash"
snswmgr_load_stack falaise@2026-02-09
```

Use the stack version appropriate for your data and local setup.

### 2. Build the repository

The repository includes a simple build script:

```bash
bash install.sh
```

This runs:

```bash
mkdir build
cd build
cmake ..
make
```

After a successful build, the main executable should be available as:

```text
build/SNCalib/sncalib
```

and the generated configuration files should appear inside the corresponding build subdirectories.

---

## Quick reference: full calibration-constant extraction chain

The typical workflow is:

```text
PTD brio files
  -> CalibrationCutsModule
  -> per-run ROOT files
  -> merged ROOT file
  -> SNCalib
  -> calibration constants CSV
```

---

## Step 1: run `CalibrationCutsModule`

`CalibrationCutsModule` is run with `flreconstruct` on PTD `.brio` files.

Example command for a single input file:

```bash
flreconstruct \
  -i /path/to/input_PTD.brio \
  -p /path/to/build/CalibrationCutsModule/calibration_cuts.conf
```

The module creates:

```text
extracted_data.root
```

For batch processing, use:

```bash
bash scripts/submit_calibrationcuts_root.sh
```

This script:

1. searches for PTD `.brio` files in the configured input directory;
2. creates a list of files to process;
3. submits a SLURM array job;
4. runs `scripts/run_calibrationcuts_root.sh` for each input file;
5. stores one ROOT file per processed run.

Important variables to edit before running:

```bash
SRC_BASE="/path/to/input/PTD/brio/files"
DST_BASE="/path/to/output/root/files"
CONF="/path/to/build/CalibrationCutsModule/calibration_cuts.conf"
STACK_NAME="falaise@2026-02-09"
```

`run_calibrationcuts_root.sh` expects:

```bash
run_calibrationcuts_root.sh LISTFILE SRC_BASE DST_BASE
```

where:

| Argument | Meaning |
|----------|---------|
| `LISTFILE` | text file containing one input PTD `.brio` file per line |
| `SRC_BASE` | base directory containing the input files |
| `DST_BASE` | output directory for ROOT files |

The output files are named as:

```text
<input_basename>_calibrationcuts.root
```

---

## Step 2: merge per-run ROOT files

After `CalibrationCutsModule` has been run on all input files, merge the per-run ROOT files into a single ROOT file:

```bash
root -l -q 'scripts/merge_calibroots.cpp'
```

Edit paths inside the macro before running.

The merged file is used as the input to `SNCalib`.

---

## Step 3: extract calibration constants with `SNCalib`

Run `SNCalib` on the merged ROOT file:

```bash
build/SNCalib/sncalib \
  -i merged_calib.root \
  -o calib_constants.csv \
  -p build/SNCalib/params.conf \
  -s -V
```

Options:

| Option | Meaning |
|--------|---------|
| `-i`, `--input` | input ROOT file with calibration data |
| `-p`, `--config` | configuration file with calibration parameters |
| `-o`, `--output` | output CSV file with calibration constants |
| `-n`, `--number` | calibrate only one chosen OM |
| `-s`, `--spectra` | save fitted spectra as PNG files |
| `-V`, `--verbose` | print processed OM numbers |

Either `-o` or `-n` must be specified.

### Calibrate all OMs

```bash
build/SNCalib/sncalib \
  -i merged_calib.root \
  -o calib_constants.csv \
  -p build/SNCalib/params.conf \
  -s -V
```

### Calibrate one OM only

```bash
build/SNCalib/sncalib \
  -i merged_calib.root \
  -n 90 \
  -p build/SNCalib/params.conf \
  -s -V
```

When `-s` is used, fitted spectra are saved into:

```text
SNCalib/Fits/
```

or the fitted-spectra directory defined during the build.

For a SLURM example, see:

```text
scripts/run_csv_make.sh
```

Before using it, edit:

```bash
INFILE="merged_calib_2025X-BOTH.root"
OUTFILE="calib_constants-BOTH.csv"
BIN="/path/to/build/SNCalib/sncalib"
CONF="/path/to/build/SNCalib/params.conf"
STACK_NAME="falaise@2026-02-09"
```

Then submit:

```bash
sbatch scripts/run_csv_make.sh
```

---

## Helper scripts

### `scripts/submit_calibrationcuts_root.sh`

Creates the input file list and submits the SLURM array for `CalibrationCutsModule`.

Edit before running:

```bash
SRC_BASE="/path/to/PTD/brio/files"
DST_BASE="/path/to/output/root/files"
```

### `scripts/run_calibrationcuts_root.sh`

Worker script for the SLURM array. It runs `flreconstruct` with `CalibrationCutsModule`.

Edit before running:

```bash
CONF="/path/to/build/CalibrationCutsModule/calibration_cuts.conf"
STACK_NAME="falaise@2026-02-09"
```

### `scripts/merge_calibroots.cpp`

ROOT macro for merging ROOT files produced by `CalibrationCutsModule`.

Edit input/output paths inside the macro before running.

### `scripts/run_csv_make.sh`

SLURM job script for running `SNCalib` on a merged ROOT file.

Edit input file, output file, binary path, config path, and stack name before running.

### `scripts/send_EXTRACT_CONSTANTS.sh`

Older step-by-step example script showing the original manual workflow:

```text
CD -> PTD
PTD -> CalibrationCutsModule ROOT
ROOT -> SNCalib CSV
```

It is kept mainly as a reference/template. For production-style processing, prefer the newer `submit_calibrationcuts_root.sh`, `run_calibrationcuts_root.sh`, and `run_csv_make.sh` scripts.

### `scripts/reco.conf`

Example reconstruction configuration used in the earlier manual workflow.

---

## Input and output summary

### Required input

For the main extraction chain:

```text
*_PTD.brio
```

The input files must already contain reconstructed tracking information needed by `CalibrationCutsModule`.

### Intermediate output

From `CalibrationCutsModule`:

```text
*_calibrationcuts.root
```

After merging:

```text
merged_calib_*.root
```

### Final output

From `SNCalib`:

```text
calib_constants*.csv
```

This CSV file is used later by `Charge2EnergyModule` to convert charge to calibrated energy.

---

## Notes and limitations

- Paths in the provided scripts are user- and CC-IN2P3-specific and must be edited before use.
- The scripts assume a SLURM environment.
- The code assumes the SuperNEMO/Falaise environment is loaded before compilation and execution.
- The selected calibration events are intended for Bi-207 single-electron calibration studies.
- Separate builds can be useful when extracting constants for different correction modes.
- Simulated data should not be calibrated in the same way as real data if it already passed through Falaise mock calibration.

---

## Typical downstream usage

The CSV file produced by this repository is used by `Charge2EnergyModule` in the energy-calibration stage.

Typical downstream chain:

```text
calib_constants.csv
  -> Charge2EnergyModule
  -> calibrated brio file
  -> SNCuts calibration selection
  -> spectrum-level and geometry-level analysis
```

## Related repositories

| Repository | Role |
|------------|------|
| https://github.com/konarfil/CalibrationTools | original calibration software by F. Koňařík |
| https://github.com/YaKozina/Calibration_Constants_Extraction | first stage: extraction of calibration constants |
| https://github.com/YaKozina/Energy_Calibration | second stage: application of calibration constants |
| https://github.com/YaKozina/Calibration_testing | full real/simulation pipeline and spectrum-level analysis |
