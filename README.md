# SNA-HCP: NS3-OpenCOOD Co-Simulation Platform

NS-3 based V2X simulation and cooperative perception data augmentation pipeline

**Repository**: [https://github.com/subin993/SNA-HCP](https://github.com/subin993/NS3-OpenCOOD)

## 📋 Overview

This project provides the NS3-OpenCOOD co-simulation platform for implementing SNA-HCP (Semantic- and Network-Aware Hierarchical Collaborative Perception). The platform simulates V2X (Vehicle-to-Everything) communication environments using the NS-3 network simulator and OpenGym, and augments cooperative perception data based on real-time network telemetry for performance evaluation.

## 🏗️ Project Structure

```
ns3_opencood/
├── README.md                      # This file
├── INSTALL.md                     # Installation guide
├── ns3-opengym/                   # NS-3 OpenGym source files
│   ├── src/                       # OpenGym module source (requires separate installation)
│   └── examples/                  # V2X simulation examples
│       ├── simple_v2x_sim.cc      # Basic V2X simulation
│       └── training_v2x_dataset_sim.cc  # For training dataset generation
├── sumo-traces/                   # SUMO FCD XML files
│   ├── highway_7_vehicles_fcd.xml # Default highway scenario
│   ├── experiment_slow_5ms.xml
│   ├── highway_7_vehicles_mixed_extreme.xml
│   ├── highway_7_vehicles_ultra_fast.xml
│   └── highway_7_vehicles_ultra_slow.xml
├── python-scripts/                # Python scripts
│   ├── ns3_guided_proven_data_augmentation.py  # Data augmentation script
│   └── ns3_proven_pipeline.sh     # Complete pipeline execution script
├── data/                          # Data directory
│   ├── source/                    # Source data (user provided)
│   └── validate/                  # Augmented data output
└── docs/                          # Documentation
    └── USAGE.md                   # Usage guide
```

## 🔧 Prerequisites

### System Requirements
- Ubuntu 20.04 or higher
- Python 3.8 or higher
- GCC 9.0 or higher
- CMake 3.10 or higher

### Software Dependencies
- **NS-3.40**: Network simulator
- **ns3-gym (OpenGym)**: Interface between NS-3 and Python
- **SUMO**: Traffic simulation (optional)
- **OpenCOOD**: Cooperative perception framework
- **Python packages**: numpy, open3d, pyyaml, zmq

## 📥 Installation

For detailed installation instructions, see [INSTALL.md](INSTALL.md).

### Quick Installation Summary

```bash
# 1. Install NS-3.40
cd ~
wget https://www.nsnam.org/releases/ns-allinone-3.40.tar.bz2
tar xjf ns-allinone-3.40.tar.bz2
cd ns-allinone-3.40
./build.py

# 2. Install ns3-gym
cd ns-3.40
git clone https://github.com/tkn-tub/ns3-gym.git contrib/opengym
pip install ./contrib/opengym/model

# 3. Copy examples from this project
cp -r /path/to/ns3_opencood/ns3-opengym/examples/* contrib/opengym/examples/

# 4. Build NS-3
./ns3 configure --enable-examples
./ns3 build

# 5. Setup Python environment
conda create -n opencood python=3.8
conda activate opencood
pip install numpy open3d pyyaml pyzmq
```

## 🚀 Usage

### 1. Run NS-3 Simulation (Terminal 1)

```bash
cd ~/ns-allinone-3.40/ns-3.40
./ns3 run 'simple-v2x-sim \
    --sumoTrace=/path/to/ns3_opencood/sumo-traces/highway_7_vehicles_fcd.xml \
    --simTime=120'
```

### 2. Run Data Augmentation Pipeline (Terminal 2)

```bash
cd /path/to/ns3_opencood/python-scripts
conda activate opencood

# Set environment variables for your setup
export SOURCE_DATA="/path/to/your/opv2v/source/data"
export MODEL_DIR="/path/to/your/trained/model"
export OPENCOOD_DIR="/path/to/OpenCOOD"

# Run the pipeline
./ns3_proven_pipeline.sh
```

The pipeline will:
- Connect to NS-3 simulation via OpenGym
- Augment data based on real-time V2X network telemetry
- Run OpenCOOD inference with the augmented data
- Save results and performance metrics

For detailed usage instructions, see [docs/USAGE.md](docs/USAGE.md).

## 📊 Key Features

### 1. NS-3 V2X Simulation
- SUMO FCD file-based vehicle mobility simulation
- IEEE 802.11p WAVE (i.e., DSRC) communication simulation
- Real-time network metrics collection (packet loss rate, latency, etc.)

### 2. Data Augmentation
- Dynamic data augmentation using NS-3 telemetry
- LiDAR point cloud transformation
- Vehicle position and velocity updates
- Network condition integration

### 3. Pipeline Integration
- Automatic integration between NS-3 simulation and Python scripts
- Augmented data validation and inference
- Automatic performance metrics collection

## 🔬 Experimental Scenarios

The project includes SUMO traces for various traffic scenarios:

- **highway_7_vehicles_fcd.xml**: Default highway scenario
- **experiment_slow_5ms.xml**: Slow congestion scenario
- **highway_7_vehicles_ultra_fast.xml**: High-speed scenario
- **highway_7_vehicles_mixed_extreme.xml**: Extreme mixed scenario

## 📝 Citation

If you use this code in your research, please cite the following paper:

```bibtex
@article{snahcp2025,
  title={SNA-HCP: Semantic and Network-Aware Hierarchical Collaborative Perception},
  author={Subin Han, Dusit Niyato, and Sangheon Pack},
  journal={T.B.D.},
  year={2025},
  note={The implementation and co-simulation environment available at \url{https://github.com/subin993/NS3-OpenCOOD}}
}
```

**Note**: For performance evaluation, we implement SNA-HCP within our developed NS3-OpenCOOD co-simulation platform, which can be found at [https://github.com/subin993/NS3-OpenCOOD](https://github.com/subin993/NS3-OpenCOOD).

## 📄 License

This project follows the MIT License. See the LICENSE file for details.

## 📧 Contact

For questions or issues, please contact Subin Han, Korea University, subin993@korea.ac.kr

## 🙏 Acknowledgments

- NS-3 development team 
- ns3-gym (OpenGym) development team
- OpenCOOD development team
- SUMO development team
