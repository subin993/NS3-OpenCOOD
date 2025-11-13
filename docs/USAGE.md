# Usage Guide

## Table of Contents
1. [Basic Usage](#basic-usage)
2. [NS-3 Simulation Options](#ns-3-simulation-options)
3. [Data Augmentation Pipeline](#data-augmentation-pipeline)
4. [Creating Custom Scenarios](#creating-custom-scenarios)
5. [Results Analysis](#results-analysis)

## Basic Usage

### Overall Workflow

The V2X cooperative perception pipeline runs simultaneously in two terminals:

1. **Terminal 1**: NS-3 simulation (provides network environment)
2. **Terminal 2**: Data augmentation and inference pipeline

### Step-by-Step Execution

#### Step 1: Start NS-3 Simulation (Terminal 1)

```bash
cd ~/ns-allinone-3.40/ns-3.40
conda activate opencood

# Using SUMO trace (recommended)
./ns3 run 'simple-v2x-sim \
    --sumoTrace=/path/to/ns3_opencood/sumo-traces/highway_7_vehicles_fcd.xml \
    --simTime=120'
```

When the simulation starts, you'll see:
```
Simulation started. Waiting for Python agent to connect...
OpenGym interface listening on port 5555
```

#### Step 2: Run Data Augmentation Pipeline (Terminal 2)

Open a new terminal:

```bash
cd /path/to/ns3_opencood/python-scripts
conda activate opencood

# Configure environment variables (modify for your setup)
export SOURCE_DATA="/path/to/your/opv2v/source/data"
export VALIDATE_ROOT="/path/to/ns3_opencood/data/validate"
export MODEL_DIR="/path/to/your/trained/model"
export OPENCOOD_DIR="/path/to/OpenCOOD"
export FUSION_METHOD="intermediate"  # or early, late

# Run the pipeline
./ns3_proven_pipeline.sh
```

**Environment Variables**:
- `SOURCE_DATA`: Path to source OPV2V data (required)
- `VALIDATE_ROOT`: Output directory for augmented data (default: `../data/validate`)
- `MODEL_DIR`: Path to trained model directory (required for inference)
- `OPENCOOD_DIR`: OpenCOOD installation path (default: `~/OpenCOOD`)
- `FUSION_METHOD`: Fusion strategy - `intermediate`, `early`, or `late` (default: `intermediate`)
- `NS3_PORT`: OpenGym port (default: `5555`)
- `NS3_SIM_TIME`: Simulation duration in seconds (default: `120.0`)

The pipeline automatically performs the following steps:
1. Verify NS-3 simulation connection
2. Data augmentation based on NS-3 telemetry
3. Backup and update OpenCOOD config
4. Run model inference with augmented data
5. Restore config and save results

## NS-3 Simulation Options

### Basic Options

```bash
./ns3 run 'simple-v2x-sim --help'
```

### Key Parameters

| Parameter | Description | Default | Example |
|---------|------|--------|------|
| `--sumoTrace` | Path to SUMO FCD XML file | (none) | `--sumoTrace=./sumo-traces/highway_7_vehicles_fcd.xml` |
| `--simTime` | Simulation time (seconds) | 60 | `--simTime=120` |
| `--numVehicles` | Number of vehicles (Random Walk mode) | 7 | `--numVehicles=10` |
| `--velocity` | Vehicle speed (m/s, Random Walk) | 20 | `--velocity=25` |
| `--txPower` | Transmission power (dBm) | 20 | `--txPower=23` |
| `--txRange` | Communication range (m) | 300 | `--txRange=500` |
| `--frequency` | Frequency (MHz) | 5900 | `--frequency=5900` |
| `--openGymPort` | OpenGym port | 5555 | `--openGymPort=5556` |

### Usage Examples

#### Example 1: Default Highway Scenario

```bash
./ns3 run 'simple-v2x-sim \
    --sumoTrace=../ns3_opencood/sumo-traces/highway_7_vehicles_fcd.xml \
    --simTime=120'
```

#### Example 2: Slow Congestion Scenario

```bash
./ns3 run 'simple-v2x-sim \
    --sumoTrace=../ns3_opencood/sumo-traces/experiment_slow_5ms.xml \
    --simTime=180'
```

#### Example 3: High-Speed Scenario

```bash
./ns3 run 'simple-v2x-sim \
    --sumoTrace=../ns3_opencood/sumo-traces/highway_7_vehicles_ultra_fast.xml \
    --simTime=100'
```

#### Example 4: Random Walk Mode (without SUMO)

```bash
./ns3 run 'simple-v2x-sim \
    --numVehicles=10 \
    --velocity=25 \
    --simTime=60'
```

## Data Augmentation Pipeline

### Pipeline Components

`ns3_proven_pipeline.sh` consists of the following steps:

1. **NS-3 Connection Check**: Verify simulation is running
2. **Data Augmentation**: Run `ns3_guided_proven_data_augmentation.py`
3. **Config Update**: Modify OpenCOOD config files
4. **Inference Execution**: Run model inference with augmented data
5. **Save Results**: Store performance metrics and logs

### Manual Data Augmentation

You can manually run augmentation instead of the automatic pipeline:

```bash
conda activate opencood
cd /path/to/ns3_opencood/python-scripts

python ns3_guided_proven_data_augmentation.py \
    --source /path/to/source/data \
    --output-dir /path/to/ns3_opencood/data/validate \
    --scenario-name my_scenario_$(date +%Y%m%d_%H%M%S) \
    --ns3-port 5555 \
    --ns3-sim-time 120.0 \
    --ns3-step-time 0.1 \
    --lidar-noise-std 0.03
```

### Parameter Descriptions

| Parameter | Description | Default |
|---------|------|--------|
| `--source` | Source data path | (required) |
| `--output-dir` | Output directory | (required) |
| `--scenario-name` | Scenario name | `ns3_scenario` |
| `--ns3-port` | NS-3 OpenGym port | 5555 |
| `--ns3-sim-time` | Simulation time (seconds) | 120.0 |
| `--ns3-step-time` | Time step (seconds) | 0.1 |
| `--lidar-noise-std` | LiDAR noise standard deviation | 0.03 |

## Creating Custom Scenarios

### Generating SUMO FCD Files

#### 1. Run SUMO Simulation

```bash
# With SUMO GUI
sumo-gui -c your_scenario.sumocfg --fcd-output my_trace.xml

# Or with command line
sumo -c your_scenario.sumocfg --fcd-output my_trace.xml
```

#### 2. Validate FCD XML

The generated FCD file should have the following format:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<fcd-export xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
            xsi:noNamespaceSchemaLocation="http://sumo.dlr.de/xsd/fcd_file.xsd">
    <timestep time="0.00">
        <vehicle id="veh_962" x="10.00" y="49.36" speed="14.38" type="passenger"/>
        <vehicle id="veh_971" x="30.00" y="51.08" speed="15.86" type="passenger"/>
        <vehicle id="veh_980" x="50.00" y="47.00" speed="13.80" type="passenger"/>
    </timestep>
    <timestep time="0.10">
        <vehicle id="veh_962" x="11.50" y="49.38" speed="14.41" type="passenger"/>
        <vehicle id="veh_971" x="31.65" y="51.07" speed="15.83" type="passenger"/>
        <vehicle id="veh_980" x="51.45" y="47.00" speed="13.79" type="passenger"/>
    </timestep>
</fcd-export>
```

**Important**: The FCD file must include:
- `vehicle id`: Unique identifier for each vehicle
- `x`, `y`: Position coordinates (meters)
- `speed`: Vehicle speed (m/s)
- `type`: Vehicle type (e.g., "passenger", "car", "truck")
- `angle` (optional): Vehicle heading angle

#### 3. Use in NS-3

```bash
./ns3 run 'simple-v2x-sim \
    --sumoTrace=/path/to/my_trace.xml \
    --simTime=120'
```

### Preparing Source Data

The pipeline uses OPV2V format data as input:

```
data/source/your_scenario/
├── 0/                    # Vehicle ID
│   ├── 000000.yaml       # Frame 0 metadata
│   ├── 000000.pcd        # Frame 0 LiDAR
│   ├── 000001.yaml
│   ├── 000001.pcd
│   └── ...
├── 1/
│   └── ...
└── ...
```

**YAML Metadata Format** (`000000.yaml`):
```yaml
lidar_pose: [x, y, z, roll, pitch, yaw]  # Vehicle pose
vehicles:
  car1:
    location: [x, y, z]
    angle: [roll, pitch, yaw]
    center: [x, y, z]
    extent: [length, width, height]
```

### OpenCOOD Inference Configuration

Before running inference, you need to prepare:

1. **Trained Model**: Download or train a cooperative perception model (e.g., PointPillar, CoBEVT, AttentiveFusion)
2. **Model Config**: Create/modify the model's `config.yaml`

**Example Config Update** (`config.yaml`):
```yaml
# Data paths
validate_dir: '/path/to/ns3_opencood/data/validate'  # Updated by pipeline
data_dir: '/path/to/original/opv2v/data'

# Model parameters
fusion_method: 'intermediate'  # or 'early', 'late'
model:
  core_method: pointpillar  # or cobevt, attentive_fusion
  
# Inference settings
wild_setting:
  async: false
  async_mode: 'real'
  async_overhead: 100
```

The pipeline automatically:
1. Backs up the original config
2. Updates `validate_dir` to point to augmented data
3. Runs OpenCOOD inference
4. Restores the original config

**Manual Inference** (if not using the pipeline):
```bash
conda activate opencood
cd /path/to/OpenCOOD

python -m opencood.tools.inference \
    --model_dir /path/to/model \
    --fusion_method intermediate
```

## Results Analysis

### Log Files

After pipeline execution, the following files are generated:

```
/path/to/ns3_opencood/
├── data/validate/
│   └── ns3_proven_YYYYMMDD_HHMMSS/   # Augmented data
└── logs/
    └── inference_ns3_proven_YYYYMMDD_HHMMSS.log  # Inference log
```

### Check Performance Metrics

```bash
# Check Average Precision
grep 'AP@' /path/to/logs/inference_ns3_proven_*.log

# View overall performance summary
grep -A 10 'Performance Summary' /path/to/logs/inference_ns3_proven_*.log
```

### Validate Augmented Data

```bash
# Check number of vehicles generated
ls -1d data/validate/ns3_proven_*/[0-9]* | wc -l

# Check number of frames
ls -1 data/validate/ns3_proven_*/0/*.yaml | wc -l

# Check data size
du -sh data/validate/ns3_proven_*
```

### Visualization

```python
import open3d as o3d
import yaml

# Load PCD file
pcd = o3d.io.read_point_cloud("data/validate/scenario/0/000000.pcd")

# Load YAML metadata
with open("data/validate/scenario/0/000000.yaml", 'r') as f:
    meta = yaml.safe_load(f)
    
print(f"Position: {meta['lidar_pose']}")
print(f"Velocity: {meta.get('velocity', 'N/A')}")

# Visualize
o3d.visualization.draw_geometries([pcd])
```

## Advanced Usage

### Batch Processing

Script for automatically processing multiple scenarios:

```bash
#!/bin/bash

SCENARIOS=("highway_7_vehicles_fcd.xml" "experiment_slow_5ms.xml" "highway_7_vehicles_ultra_fast.xml")

for scenario in "${SCENARIOS[@]}"; do
    echo "Processing $scenario..."
    
    # Terminal 1: NS-3 (background)
    cd ~/ns-allinone-3.40/ns-3.40
    ./ns3 run "simple-v2x-sim --sumoTrace=../ns3_opencood/sumo-traces/$scenario --simTime=120" &
    NS3_PID=$!
    
    sleep 5  # Wait for NS-3 to start
    
    # Terminal 2: Pipeline
    cd /path/to/ns3_opencood/python-scripts
    ./ns3_proven_pipeline.sh
    
    # Terminate NS-3
    kill $NS3_PID
    wait $NS3_PID 2>/dev/null
    
    sleep 2
done
```

### Debugging

```bash
# Run NS-3 in debug mode
cd ~/ns-allinone-3.40/ns-3.40
./ns3 configure --enable-examples --build-profile=debug
./ns3 build
gdb --args ./build/contrib/opengym/examples/ns3.40-simple-v2x-sim-debug --simTime=10

# Debug Python scripts
python -m pdb python-scripts/ns3_guided_proven_data_augmentation.py --source ...
```

## Troubleshooting

### NS-3 Connection Failure

```
Error: Could not connect to NS-3 OpenGym interface
```

**Solution**:
1. Verify NS-3 simulation is running
2. Check port numbers match (default: 5555)
3. Check firewall settings

### Out of Memory

```
Error: MemoryError during point cloud processing
```

**Solution**:
- Reduce `--lidar-noise-std` value
- Adjust batch size
- Shorten simulation time

### Data Format Error

```
Error: Invalid YAML format in frame metadata
```

**Solution**:
- Verify source data is in OPV2V format
- Validate YAML file syntax

## Additional Resources

- [NS-3 Documentation](https://www.nsnam.org/documentation/)
- [ns3-gym GitHub](https://github.com/tkn-tub/ns3-gym)
- [OpenCOOD GitHub](https://github.com/DerrickXuNu/OpenCOOD)
- [SUMO Documentation](https://sumo.dlr.de/docs/)
