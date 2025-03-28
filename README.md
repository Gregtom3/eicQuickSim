[![Build CI](https://github.com/Gregtom3/eicQuickSim/actions/workflows/makefile.yml/badge.svg)](https://github.com/Gregtom3/eicQuickSim/actions/workflows/makefile.yml)
[![Monte Carlo Binning CI](https://github.com/Gregtom3/eicQuickSim/actions/workflows/makefile.yml/badge.svg)](https://github.com/Gregtom3/eicQuickSim/actions/workflows/binscheme-runner.yml)
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

### 5. Build macros

```bash
make build <MACRO_NAME>
```





