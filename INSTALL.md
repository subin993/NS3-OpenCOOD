# Installation Guide

## Table of Contents
1. [System Preparation](#system-preparation)
2. [NS-3 Installation](#ns-3-installation)
3. [ns3-gym Installation](#ns3-gym-installation)
4. [V2X Example Integration](#v2x-example-integration)
5. [Python Environment Setup](#python-environment-setup)
6. [OpenCOOD Installation](#opencood-installation)
7. [Installation Verification](#installation-verification)

## System Preparation

### Ubuntu Package Installation

```bash
sudo apt-get update
sudo apt-get install -y \
    g++ python3 python3-dev \
    cmake ninja-build \
    git mercurial \
    gdb valgrind \
    gsl-bin libgsl-dev libgslcblas0 \
    sqlite3 libsqlite3-dev \
    libxml2 libxml2-dev \
    libgtk-3-dev \
    qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools \
    libboost-all-dev \
    libzmq3-dev libprotobuf-dev protobuf-compiler
```

### Miniconda Installation (Optional, Recommended)

```bash
wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh
bash Miniconda3-latest-Linux-x86_64.sh
# Restart terminal after installation
```

## NS-3 Installation

### 1. Download and Build NS-3.40

```bash
cd ~
wget https://www.nsnam.org/releases/ns-allinone-3.40.tar.bz2
tar xjf ns-allinone-3.40.tar.bz2
cd ns-allinone-3.40

# Full build (takes a long time)
./build.py

# Or build NS-3 only
cd ns-3.40
./ns3 configure --enable-examples --enable-tests
./ns3 build
```

### 2. Verify Build

```bash
cd ~/ns-allinone-3.40/ns-3.40
./test.py
```

## ns3-gym Installation

### 1. Clone ns3-gym Module

```bash
cd ~/ns-allinone-3.40/ns-3.40
git clone https://github.com/tkn-tub/ns3-gym.git contrib/opengym
```

### 2. Install Python Package

```bash
# Create Conda environment (recommended)
conda create -n ns3gym python=3.8
conda activate ns3gym

# Install ns3-gym Python package
cd ~/ns-allinone-3.40/ns-3.40/contrib/opengym
pip install ./model/

# Or install in development mode
pip install -e ./model/
```

### 3. Compile Protobuf Messages

```bash
cd ~/ns-allinone-3.40/ns-3.40/contrib/opengym
./model/ns3gym/compile_proto.sh
```

## V2X Example Integration

### 1. Copy Example Files

Let's refer to the location where you clone this repository as `V2X_REPO`.

```bash
# Clone this repository
cd ~
git clone https://github.com/your-username/v2x-cooperative-perception.git
cd v2x-cooperative-perception

# Set environment variables
V2X_REPO=$(pwd)
NS3_DIR=~/ns-allinone-3.40/ns-3.40

# Copy V2X examples
mkdir -p $NS3_DIR/contrib/opengym/examples/simple-v2x
cp $V2X_REPO/ns3-opengym/examples/* $NS3_DIR/contrib/opengym/examples/simple-v2x/
```

### 2. Rebuild NS-3

```bash
cd ~/ns-allinone-3.40/ns-3.40
./ns3 clean
./ns3 configure --enable-examples
./ns3 build
```

### 3. Verify Build

```bash
cd ~/ns-allinone-3.40/ns-3.40
./ns3 run simple-v2x-sim -- --help
```

If properly installed, the help message will be displayed.

## Python Environment Setup

### 1. Create OpenCOOD Environment

```bash
conda create -n opencood python=3.8
conda activate opencood
```

### 2. Install Required Python Packages

```bash
pip install numpy==1.23.5
pip install open3d==0.16.0
pip install pyyaml
pip install pyzmq
pip install protobuf==3.20.3
```

### 3. Install ns3gym Python Package (in opencood environment too)

```bash
conda activate opencood
cd ~/ns-allinone-3.40/ns-3.40/contrib/opengym
pip install ./model/
```

## OpenCOOD Installation

### 1. Clone and Install OpenCOOD

```bash
cd ~
git clone https://github.com/DerrickXuNu/OpenCOOD.git
cd OpenCOOD

# Activate opencood environment
conda activate opencood

# Install dependencies
pip install -r requirements.txt

# Install OpenCOOD
python setup.py develop

# Or
pip install -e .
```

### 2. CUDA and PyTorch Setup (when using GPU)

```bash
# Install PyTorch (CUDA 11.8 example)
conda install pytorch torchvision torchaudio pytorch-cuda=11.8 -c pytorch -c nvidia

# Or latest version
pip install torch torchvision torchaudio
```

### 3. Install Spconv (Required)

```bash
pip install spconv-cu118  # CUDA 11.8
# Or
pip install spconv-cu116  # CUDA 11.6
```

## Installation Verification

### 1. NS-3 Simulation Test

```bash
cd ~/ns-allinone-3.40/ns-3.40

# Basic execution (Random Walk)
./ns3 run 'simple-v2x-sim --simTime=10'

# Using SUMO trace
./ns3 run 'simple-v2x-sim --sumoTrace=/path/to/ns3_opencood/sumo-traces/highway_7_vehicles_fcd.xml --simTime=10'
```

### 2. Python Integration Test

Terminal 1:
```bash
cd ~/ns-allinone-3.40/ns-3.40
conda activate opencood
./ns3 run 'simple-v2x-sim --simTime=30'
```

Terminal 2:
```bash
conda activate opencood
python -c "import ns3gym; print('ns3gym import successful!')"
```

### 3. Full Pipeline Test

First, prepare the source data:

```bash
cd /path/to/ns3_opencood
# Place OPV2V data in data/source/ directory
# Example: data/source/2021_09_11_00_33_16_temp
```

Then run the pipeline:

Terminal 1:
```bash
cd ~/ns-allinone-3.40/ns-3.40
conda activate opencood
./ns3 run 'simple-v2x-sim --sumoTrace=/path/to/ns3_opencood/sumo-traces/highway_7_vehicles_fcd.xml --simTime=120'
```

Terminal 2:
```bash
cd /path/to/ns3_opencood/python-scripts
conda activate opencood
./ns3_proven_pipeline.sh
```

## Troubleshooting

### NS-3 Build Failure

```bash
# Clean build directory
cd ~/ns-allinone-3.40/ns-3.40
./ns3 clean

# Remove cache
rm -rf cmake-cache

# Rebuild
./ns3 configure --enable-examples
./ns3 build
```

### Protobuf Version Conflict

```bash
# Downgrade Protobuf version
pip install protobuf==3.20.3
```

### ZMQ Connection Error

```bash
# Reinstall ZMQ package
pip uninstall pyzmq
pip install pyzmq --no-cache-dir
```

### CUDA Compatibility Issue

```bash
# Check CUDA version
nvidia-smi

# Check PyTorch CUDA version
python -c "import torch; print(torch.version.cuda)"

# Reinstall PyTorch with matching version
```

## Next Steps

Once installation is complete, refer to [docs/USAGE.md](../docs/USAGE.md) to check usage instructions.
