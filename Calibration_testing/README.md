# Calibration_testing

Scripts, configuration files, and ROOT macros for the full calibration and
analysis chain of the SuperNEMO calorimeter using Bi-207 calibration sources,
developed and tested on CC-IN2P3.

Applied after:

- <https://github.com/YaKozina/Calibration_Constants_Extraction>
- <https://github.com/YaKozina/Energy_Calibration>

The repository contains two independent parts:

- **REAL** - full pipeline for real experimental data: calibration-constant
  extraction and application of the constants to the same dataset for all four
  energy-correction modes. Uses Falaise 5.1.10 at least.

- **SIMU** - simulation pipeline for Bi-207 calibration events, used to study
  the physical origin of the angular energy-loss effect observed in real data.
  Simulated events are not calibrated; the simulation is used to identify which
  electrons lose energy in the copper calibration-source frame (GID 1066) and
  to confirm that these electrons populate the inter-peak 600-800 keV plateau.
  Uses Falaise 5.1.13 and further versions only.

  and the **UNIFIED PIPELINE**

<img width="705" height="1388" alt="pipelines drawio" src="https://github.com/user-attachments/assets/b32e8201-22fa-49f5-9780-7cdf68dc66c5" />


---

## Repository structure

```text
Calibration_testing/
├── U_PIPELINE_MODULES/     # Unified pipeline configuration files (all stages)
├── REAL/                   # Real data pipeline for each correction mode
│   ├── Calibration_Constants_Extraction-NOCALIB/  # CalibrationTools built for NOCALIB
│   ├── Calibration_Constants_Extraction-OPTICAL/  # CalibrationTools built for OPTICAL
│   ├── Calibration_Constants_Extraction-BETE/     # CalibrationTools built for BETE
│   ├── Calibration_Constants_Extraction-BOTH/     # CalibrationTools built for BOTH
│   ├── NOCALIB/            # Full chain: no energy corrections
│   ├── OPTICAL/            # Full chain: optical correction only
│   ├── BETE/               # Full chain: Bethe-Bloch energy-loss correction only
│   └── BOTH/               # Full chain: both corrections applied
├── Energy_Calibration/     # Charge2EnergyModule local copy, compiled separately
├── SIMU/                   # Simulation pipeline for Bi-207
└── various_scripts/        # Utility ROOT macros and shell scripts
```

---

## U_PIPELINE_MODULES - unified pipeline configuration files

Contains the `flreconstruct` configuration files for each stage of the unified
pipeline, applicable to both real and simulated data. See `pipelines.drawio.png`
for the full pipeline scheme.

| File | Pipeline stage | Input -> Output | Data type |
|---|---|---|---|
| `1D-pipeline.conf` | 1D-pipeline | UDD -> pCD -> CD, incomplete and without energy | Real only |
| `1S-pipeline.conf` | 1S-pipeline | SD -> mock calibration -> CD, full | Simu only |
| `2D-CMRMN-CPT-GT-pipeline.conf` | 2D-pipeline | CD -> TCD, TTD, PTD | Both |
| `C2E-pipeline.conf` | C2E-pipeline | PTD brio + CSV -> calibrated CD | Real only |
| `SNCuts-pipeline.conf` | SNC-pipeline | calibrated brio -> filtered brio | Both |

### C2E-pipeline_confs_corrections/

Correction-mode-specific configuration files for `Charge2EnergyModule`.
Each file differs only in which energy corrections are applied and which CSV
file with calibration constants is used.

| File | Corrections applied |
|---|---|
| `charge2energy-NOCALIB.conf` | None, raw energy only |
| `charge2energy-OPTICAL.conf` | Optical corrections only |
| `charge2energy-BETE.conf` | Bethe-Bloch energy-loss corrections only |
| `charge2energy-BOTH.conf` | Optical + energy-loss corrections, best estimate |
| `charge2energy-EXAMPLE.conf` | Template with comments |
| `charge2energy.conf` | General-purpose version |

**Key parameters common to all C2E configs:**

- `gas_pressure` - tracking gas pressure [bar], default 0.89
- `He_pressure`, `Et_pressure`, `Ar_pressure` - partial pressures of gas components
- `T_gas` - gas temperature [K], default 298.0
- `calibration_path` - path to the CSV file with the `a_j`, `b_j` constants

### SNCuts-pipeline.conf - calibration event selection cuts

| Parameter | Value | Description |
|---|---:|---|
| `useEventHasVertexCloseToCalibSource` | `true` | Vertex within an ellipse around the nearest calibration source |
| `source_cut_ellipse_Y` | `15 mm` | Ellipse semi-axis in Y |
| `source_cut_ellipse_Z` | `40 mm` | Ellipse semi-axis in Z |
| `hasNumberofKinks` | `1 0` | Reject kinked tracks: the first number is the number of straight tracks, the second is the number of tracks with one kink; the list can be expanded for more kink categories |
| `useEventTrackHasOneAssocCaloHit` | `true` | Track must have exactly one associated OM hit |
| `minSumEnergy` / `maxSumEnergy` | `0 / 3500 keV` | Energy range, example values |
| `source_pos_path` | path to `.txt` | Calibration source positions, must match reconstruction |

### 2D-CMRMN-CPT-GT-pipeline.conf - track reconstruction

Chains three modules:

1. **Cimrman** - Legendre-transform-based clustering and trajectory
   reconstruction; produces TCD and TTD banks.
2. **ChargedParticleTracker** - vertex extrapolation and particle
   identification; produces the PTD bank.
3. **Gamma clustering** - commented out, because it is not needed for
   single-electron calibration analysis.

**Note:** update `Falaise_Cimrman.directory` to point to your local
CimrmanModule build.

---

## REAL - real data pipeline

Each correction-mode folder (`NOCALIB`, `OPTICAL`, `BETE`, `BOTH`) follows the
same structure. The compiled CalibrationTools for each mode are located in the
corresponding `Calibration_Constants_Extraction-<MODE>/` subfolder under
`REAL/`.

### Quick reference - how to run

Update all hardcoded paths inside the scripts before running.

**Step 1 - CalibrationCutsModule: brio -> ROOT per run**

```bash
bash REAL/BOTH/submit_calibrationcuts_root.sh
# Worker: run_calibrationcuts_root.sh
# Input:  *_PTD.brio files
# Output: *_PTD_calibrationcuts.root, one file per run
# Module: REAL/Calibration_Constants_Extraction-BOTH/
```

**Step 2 - Merge ROOT files**

```bash
root -l -q 'REAL/BOTH/calibroot/merge_calibroots.cpp'
# Merges per-run ROOT files while preserving OM numbering
# Output: merged_calib_*.root
```

**Step 3 - SNCalib: merged ROOT -> CSV**

```bash
sbatch REAL/BOTH/run_csv_make.sh
# Input:  merged_calib_*.root
# Output: calib_constants-BOTH.csv
# Reference result already included in the repository
```

**Step 4 - Charge2EnergyModule / C2E-pipeline: PTD brio -> calibrated brio**

```bash
bash REAL/BOTH/submit_calibrate_using_BOTH-const_array.sh
# Worker: run_calibrate_using_BOTH-const_array.sh
# Config: U_PIPELINE_MODULES/C2E-pipeline_confs_corrections/charge2energy-BOTH.conf
# Input:  *_PTD.brio
# Output: *_PTD_c2e.brio
```

**Step 5 - SNCuts: calibrated brio -> filtered brio**

```bash
bash REAL/BOTH/calibrated_brio/submit_SNCUTS_on_c2e_array.sh
# Worker: run_SNCUTS_on_c2e_array.sh
# Config: REAL/BOTH/calibrated_brio/SNCuts-pipeline.conf
# Input:  *_PTD_c2e.brio
# Output: *_PTD_c2e-SNCUTS.brio

# Optional: select 600-800 keV events for the inter-peak plateau study
sbatch REAL/BOTH/calibrated_brio/send_600-800_one_ev.sh
# Config: REAL/BOTH/calibrated_brio/SNCuts_600-800.conf
```

**Step 6 - MiModule: filtered brio -> ROOT**

```bash
bash REAL/BOTH/calibrated_brio/SNCUTS/submit_MiModule.sh
# Worker: run_MiModule.sh
# Config: REAL/BOTH/calibrated_brio/SNCUTS/p_MiModule_v00.conf
# Input:  *_PTD_c2e-SNCUTS.brio
# Output: *_PTD_c2e-SNCUTS.root, one file per run
```

**Step 7 - Merge and analyse**

```bash
root -l -q 'REAL/BOTH/calibrated_brio/SNCUTS/sncuts_NEW_positions/ROOT/merge.cpp'
# Output: merged single ROOT file with one unified tree

root -l -q 'REAL/BOTH/calibrated_brio/SNCUTS/sncuts_NEW_positions/reading_root_totE.cpp'
# Total energy spectrum as .txt

root -l -q 'REAL/BOTH/calibrated_brio/SNCUTS/sncuts_NEW_positions/reading_root_totE_angle.cpp'
# Spectra split by electron emission angle

root -l -q 'REAL/BOTH/calibrated_brio/SNCUTS/sncuts_NEW_positions/analyze_spectrum_from_txt_ADV.cpp'
# Calibration energy spectrum plot

root -l -q 'REAL/BOTH/calibrated_brio/SNCUTS/sncuts_NEW_positions/draw_spectra_overlay.cpp'
# Angular bins overlaid on one canvas, BOTH only

root -l -q 'REAL/BOTH/calibrated_brio/SNCUTS/sncuts_NEW_positions/reading_root_foil_vert_YZ_DISTRIBUTION.cpp'
# Foil vertex (Y, Z) distribution, BOTH only
```

### Per-mode reference results: CSV files

| File | Mode |
|---|---|
| `REAL/NOCALIB/calib_constants-NOCORR.csv` | No corrections |
| `REAL/OPTICAL/calib_constants_OPTIC.csv` | Optical only |
| `REAL/BETE/calib_constants-BETE.csv` | Bethe-Bloch energy-loss only |
| `REAL/BOTH/calib_constants-BOTH.csv` | Both corrections |

---

## SIMU - simulation pipeline

Configuration files and scripts for simulating Bi-207 calibration events and
processing them through the full reconstruction chain:

```text
flsimulate -> 1S-pipeline -> 2D-CMRMN-CPT-GT-pipeline -> SNCuts -> MiModule
```

No energy calibration is applied to simulated data.

The simulation was used to investigate the angular dependence of the
calibration spectrum observed in real data. By tagging electrons that deposited
energy in the copper calibration-source frame (GID 1066), it was confirmed that
these electrons are responsible for the broad plateau between the 482 keV and
976 keV conversion peaks, and that their contribution increases for electrons
emitted at grazing angles relative to the source foil.

### Configuration files (`Bi-207/`)

| File | Description |
|---|---|
| `simu_setup.conf` | `flsimulate` configuration: Bi-207 decay at calibration source positions |
| `simu.profile` / `simu_orig.profile` | Variant profiles for the simulation |
| `1S-pipeline.conf` | Mock calibration: tracker + calorimeter -> full CD bank |
| `2D-CMRMN-CPT-GT-pipeline.conf` | Track reconstruction -> TCD, TTD, PTD banks |
| `SNCuts-pipeline.conf` | Calibration event selection for simulated data |
| `SNCuts_600-800.conf` | SNCuts selection for 600-800 keV events only |
| `p_MiModule_v00.conf` | MiModule configuration |
| `source_positions_NEW.txt` | Calibration source positions |
| `pipeline.conf` / `reco.conf` | Alternative pipeline configurations |

### Quick reference - how to run

**Steps 1-3 in one job: flsimulate -> 1S-pipeline -> 2D-pipeline**

```bash
bash SIMU/submit_simu_array.sh FIRST LAST
# Example: bash SIMU/submit_simu_array.sh 1 100
# Worker: simu.sh
# Output: SIMU/brios/<num>/2D_reco_Bi_<num>.brio
# Intermediate files, SD and 1S brio, are deleted automatically after success
```

**Step 4 - SNCuts**

```bash
bash SIMU/submit_SNCUTS_on_c2e_array.sh
# Config: SIMU/Bi-207/SNCuts-pipeline.conf
# Input:  SIMU/brios/<num>/2D_reco_Bi_<num>.brio
# Output: filtered brio files

# Single-event visualisation in the 600-800 keV window
sbatch SIMU/send_600-800_one_ev.sh
```

**Step 5 - MiModule**

```bash
bash SIMU/submit_MiModule.sh
# Config: SIMU/Bi-207/p_MiModule_v00.conf
# Output: ROOT files
```

**Step 6 - Merge and analyse**

```bash
root -l -q 'SIMU/brios/SNCUTS/NEW_s_positions/ROOT/merge.cpp'

root -l -q 'SIMU/brios/SNCUTS/NEW_s_positions/ROOT/reading_root_totE.cpp'
# Total energy spectrum

root -l -q 'SIMU/brios/SNCUTS/NEW_s_positions/ROOT/reading_root_totE_angle.cpp'
# Angular-binned spectra

root -l -q 'SIMU/brios/SNCUTS/NEW_s_positions/ROOT/carrier_loss_spectrum.cpp'
# Energy spectrum of electrons with non-zero deposit in the copper frame, GID 1066

root -l -q 'SIMU/brios/SNCUTS/NEW_s_positions/ROOT/overlay_with_without_carrier.cpp'
# Overlay: all electrons vs copper-frame electrons

root -l -q 'SIMU/brios/SNCUTS/NEW_s_positions/ROOT/overlay_spectrum.cpp'
# Overlay of simulated and real spectra

root -l -q 'SIMU/brios/SNCUTS/NEW_s_positions/ROOT/list_carrier_gids.cpp'
# List geometry IDs of copper-frame hits

root -l -q 'SIMU/brios/SNCUTS/NEW_s_positions/ROOT/analyze_spectrum_from_txt_ADV.cpp'
# Draw spectrum from .txt file
```

### MiModule with GID 1066 feature (`MiModule_scripts-GID_1066_feature/`)

Modified MiModule scripts that additionally extract energy deposits in the
copper calibration-source frame (geometry identifier GID 1066). Used to confirm
that copper-frame energy losses are responsible for the broad inter-peak
plateau.

| File | Description |
|---|---|
| `MiModule.cpp` | Modified main script with GID 1066 hit extraction |
| `MiSD.cpp` / `MiSDCaloHit.cpp` / `MiSDCaloHit.h` | Supporting classes for simulated data |

---

## various_scripts

General-purpose ROOT macros and shell scripts, maintained as the most recent
versions. They can be used as templates or copied into a specific correction
mode folder.

| File | Description |
|---|---|
| `reading_root_totE.cpp` | Total electron energy spectrum from merged ROOT |
| `reading_root_totE_angle.cpp` | Energy spectrum split by emission angle |
| `reading_root_OM_SPECTRA.cpp` | Per-OM energy spectrum, one `.txt` per OM |
| `reading_root_OM_SPECTRA_angle.cpp` | Per-OM spectrum with angular cut |
| `reading_root_OM_SPECTRA_faceCircle5cm.cpp` | Per-OM spectrum with 5 cm face-radius cut |
| `reading_root_OM_SPECTRA_faceCircleAngleCut.cpp` | Per-OM spectrum with face + angle cuts |
| `reading_root_TOTAL_SPECTRUM_OMVertexCut.cpp` | Total spectrum with OM vertex-position cut |
| `reading_root_foil_vert_YZ_DISTRIBUTION.cpp` | Foil vertex (Y, Z) distribution |
| `analyze_spectrum_from_txt.cpp` | Basic spectrum drawing from `.txt` |
| `analyze_spectrum_from_txt_ADV.cpp` | Advanced spectrum drawing with Gaussian fits |
| `draw_spectra_overlay.cpp` | Overlay of multiple spectra, e.g. angular bins |
| `merge.cpp` | Merge per-run ROOT files into one |
| `submit_SNCUTS_on_c2e_array.sh` / `run_SNCUTS_on_c2e_array.sh` | SNCuts SLURM array |
| `submit_MiModule.sh` / `run_MiModule.sh` | MiModule SLURM array |
| `send.sh` | Single-event pipeline test, SNCuts + MiModule sequentially |
| `send_600-800_one_ev.sh` | Single event in the 600-800 keV window for `flvisualize` |

## Related repositories

| Repository | Role |
|------------|------|
| https://github.com/konarfil/CalibrationTools | original calibration software by F. Koňařík |
| https://github.com/YaKozina/Calibration_Constants_Extraction | first stage: extraction of calibration constants |
| https://github.com/YaKozina/Energy_Calibration | second stage: application of calibration constants |
| https://github.com/YaKozina/Calibration_testing | full real/simulation pipeline and spectrum-level analysis |

