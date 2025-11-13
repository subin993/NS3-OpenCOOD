# V2X Standalone Package - File List

## Generated Directory Structure

```
ns3_opencood/
├── README.md                          # Project overview and quick start guide
├── INSTALL.md                         # Detailed installation guide
├── LICENSE                            # MIT License
├── requirements.txt                   # Python package dependencies
├── .gitignore                         # Git ignore file list
│
├── ns3-opengym/                       # NS-3 OpenGym related files
│   ├── src/                           # OpenGym module source (requires separate installation)
│   └── examples/                      # V2X simulation examples
│       ├── simple_v2x_sim.cc          # Basic V2X simulation
│       └── training_v2x_dataset_sim.cc # For training dataset generation
│
├── sumo-traces/                       # SUMO FCD XML files (5 files)
│   ├── highway_7_vehicles_fcd.xml     # Default highway (α=1.0)
│   ├── experiment_slow_5ms.xml        # Slow congestion (α=0.5)
│   ├── highway_7_vehicles_ultra_fast.xml  # High speed (α=1.5)
│   ├── highway_7_vehicles_ultra_slow.xml  # Ultra slow
│   └── highway_7_vehicles_mixed_extreme.xml  # Extreme mixed
│
├── python-scripts/                    # Python scripts
│   ├── ns3_guided_proven_data_augmentation.py  # Data augmentation
│   └── ns3_proven_pipeline.sh         # Complete pipeline
│
├── scripts/                           # Helper scripts
│   ├── quickstart.sh                  # Quick setup script
│   └── run_ns3.sh                     # NS-3 execution helper
│
├── data/                              # Data directory
│   ├── source/                        # Source data (user provided)
│   │   └── .gitkeep
│   └── validate/                      # Augmented data output
│       └── .gitkeep
│
├── logs/                              # Log files
│   └── .gitkeep
│
└── docs/                              # Documentation
    └── USAGE.md                       # Usage guide
```

## Key File Descriptions

### Documentation
- **README.md**: Project overview, requirements, basic usage
- **INSTALL.md**: Detailed installation guide for NS-3, ns3-gym, Python environment
- **docs/USAGE.md**: Execution options, parameters, advanced usage

### NS-3 Source Files
- **simple_v2x_sim.cc**: V2X simulation based on SUMO trace or Random Walk
- **training_v2x_dataset_sim.cc**: Simulation for training dataset generation

### SUMO Traces
- 5 diverse traffic scenarios
- FCD (Floating Car Data) XML format
- Contains vehicle trajectory, speed, angle information

### Python Scripts
- **ns3_guided_proven_data_augmentation.py**: 
  - Receive NS-3 telemetry
  - Transform LiDAR data
  - Generate augmented data
  
- **ns3_proven_pipeline.sh**:
  - Verify NS-3 connection
  - Execute data augmentation
  - OpenCOOD inference
  - Result logging

### Helper Scripts
- **quickstart.sh**: Automate initial setup
- **run_ns3.sh**: Simplify NS-3 execution

## Usage Workflow

### 1. Initial Setup
```bash
cd ns3_opencood
./scripts/quickstart.sh
```

### 2. Run NS-3 (Terminal 1)
```bash
./scripts/run_ns3.sh highway_7_vehicles_fcd 120
```

Or manually:
```bash
cd ~/subin/ns-allinone-3.40/ns-3.40
./ns3 run 'simple-v2x-sim \
    --sumoTrace=/path/to/ns3_opencood/sumo-traces/highway_7_vehicles_fcd.xml \
    --simTime=120'
```

### 3. Run Pipeline (Terminal 2)
```bash
cd ns3_opencood/python-scripts
./ns3_proven_pipeline.sh
```

## GitHub Sharing Checklist

### ✅ Included Items
- [x] Complete documentation (README, INSTALL, USAGE)
- [x] NS-3 source files
- [x] SUMO traces (5 files)
- [x] Python augmentation scripts
- [x] Helper scripts
- [x] requirements.txt
- [x] LICENSE
- [x] .gitignore
- [x] Directory structure (.gitkeep)

### ⚠️ Excluded Items (large files or user-specific)
- [ ] Full NS-3 build files (user installs separately)
- [ ] Complete OpenCOOD (separate installation)
- [ ] Trained models (user prepares)
- [ ] Source data (data/source/)
- [ ] Generated data (data/validate/)
- [ ] Log files (logs/)

### 📝 What Users Need to Prepare
1. NS-3.40 installation
2. ns3-gym installation
3. OpenCOOD installation
4. Python environment and packages
5. Original OPV2V data
6. Trained models (e.g., PointPillar CoBEVT)

## Test Results

### ✅ Verified
- NS-3 simple-v2x-sim build and execution
- SUMO trace loading
- OpenGym interface waiting state
- run_ns3.sh helper script functionality

### 📋 User Should Test
- Full pipeline (NS-3 + Python integration)
- Data augmentation and inference
- Various scenario execution

## Next Steps

1. **Create GitHub Repository**
   ```bash
   cd ns3_opencood
   git init
   git add .
   git commit -m "Initial commit: V2X Cooperative Perception Pipeline"
   git remote add origin <your-github-repo-url>
   git push -u origin main
   ```

2. **Improve README**
   - Add author information
   - Update paper citation information
   - Add screenshots/demos

3. **Test and Validate**
   - Test installation guide in fresh environment
   - Set up CI/CD (optional)

4. **Enhance Documentation**
   - Add FAQ section
   - Expand troubleshooting guide
   - Add example results
