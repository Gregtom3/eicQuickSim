[![Build CI](https://github.com/Gregtom3/eicQuickSim/actions/workflows/makefile.yml/badge.svg)](https://github.com/Gregtom3/eicQuickSim/actions/workflows/makefile.yml)

# eicQuickSim

This is a simulation and analysis framework for EIC-related studies. The purpose is to analyze electron+neutron Monte Carlo simulations to predict e+He3 yields at the future EIC. It uses [eic-shell](https://github.com/eic/eic-shell) to provide a preconfigured environment with ROOT and other dependencies.

## 🔧 Local Setup (Linux or macOS)

First, make sure you have [Apptainer](https://apptainer.org) or [Singularity v3+](https://sylabs.io/guides/3.0/user-guide/) installed.

### 1. Clone the repo

The `--recursive` flag ensures we install the `yaml-cpp` package, which is not included in `eic-shell` by default.

```bash
git clone --recursive https://github.com/Gregtom3/eicQuickSim.git
cd eicQuickSim
```

### 2. Set up the EIC shell 

```bash
curl -L https://github.com/eic/eic-shell/raw/main/install.sh | bash
```

This installs the container and a wrapper scripts called `./eic-shell`.

### 3. Enter the container

```bash
./eic-shell
```

You'll now be inside a shell with ROOT and other tools preinstalled.

### 4. Install relevant python packages

Within the `eicQuickSim/` directory run

```bash
make install
```

## HPC Tutorial

This repository is configured to run analysis on a large set of Monte Carlo files for both `e+p` and `e+n` interactions. These files, which use HepMC3 format, cover various Q² ranges along with the energy configurations. The full list of available Monte Carlo files is maintained in CSV format (`ep_files.csv` for `e+p` and `en_files.csv` for `e+n`) located in `src/eicQuickSim/`. Each CSV contains details such as Q² ranges, number of events, and energy configurations.

The overall strategy is to divide the large CSV files into smaller chunks, and then use SLURM to run analysis macros over each chunk in parallel. **Note:** The current SLURM pipeline is set up for the author (gregory.matousek@duke.edu). If you wish to run this repository in your own environment, please contact me so I can make the necessary adjustments.

### 1. Create a New Project

To begin, run the following command (inside the `eic-shell`) to extract a subset of data from `ep_files.csv` and `en_files.csv`. This command will create a new project directory under `out/<PROJECT_NAME>`:

```bash
s3tools/create_project.rb -n test_v0 -f 3 -m 1000
```

- `-n`: Project name.
- `-f`: Maximum number of files (rows) to take from the original CSV lists.
- `-m`: Maximum number of events to process; set this to a high value (e.g., over 100M) if you want to use all available events.

### 2. Split the Project into Chunks

Once the project is created, divide it into chunks for parallel processing with SLURM by running:

```bash
hpc/run_hpc_jobs.rb -n test_v0 -f 1 -a DIS
```

- `-n`: Must match the project name created earlier.
- `-f`: Maximum number of files (rows) each SLURM job will analyze.
- `-a`: Analysis macro to be used, corresponding to a file in the `macros/` directory (here, `DIS`).

For additional options and details, use the `-h` flag.

### 3. Run SLURM Jobs

After running the splitting script, a shell script called `run_jobs.sh` is generated in the project’s directory (`out/<PROJECT_NAME>/run_jobs.sh`). This file contains all the `sbatch` commands needed to submit each chunk as a separate SLURM job.

Because the `sbatch` command is not recognized within the `eic-shell`, you must run the shell script externally:

```bash
bash out/<PROJECT_NAME>/run_jobs.sh
```

This will submit all the jobs to SLURM for parallel execution.
