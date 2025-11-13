#!/bin/bash
# NS-3 Guided Proven Data Augmentation + Inference Pipeline
# NS3-OpenCOOD Co-Simulation Platform for SNA-HCP

set -e

echo "🚀 NS-3 Guided Proven Data Augmentation + Inference Pipeline"
echo "============================================================"

# =============================================================================
# Configuration - MODIFY THESE PATHS FOR YOUR ENVIRONMENT
# =============================================================================

# Path to source data (OPV2V format)
SOURCE_DATA="${SOURCE_DATA:-$HOME/subin/mobility_aware_cooperative_perception/data/opv2v_real_scenarios/arxiv/2021_09_11_00_33_16_temp}"

# Output directory for augmented data
VALIDATE_ROOT="${VALIDATE_ROOT:-$HOME/subin/mobility_aware_cooperative_perception/data/opv2v_real_scenarios/validate}"

# OpenCOOD installation path
OPENCOOD_DIR="${OPENCOOD_DIR:-$HOME/subin/OpenCOOD}"

# Model directory (PointPillar CoBEVT or other trained model)
MODEL_DIR="${MODEL_DIR:-$HOME/subin/mobility_aware_cooperative_perception/models/pretrained/pointpillar_cobevt}"

# Fusion method (intermediate, early, late)
FUSION_METHOD="${FUSION_METHOD:-intermediate}"

# NS-3 OpenGym port
NS3_PORT="${NS3_PORT:-5555}"

# Simulation parameters
NS3_SIM_TIME="${NS3_SIM_TIME:-120.0}"
NS3_STEP_TIME="${NS3_STEP_TIME:-0.1}"
LIDAR_NOISE_STD="${LIDAR_NOISE_STD:-0.03}"

# =============================================================================

# Environment setup
if command -v conda &> /dev/null; then
    source ~/miniconda3/etc/profile.d/conda.sh 2>/dev/null || source ~/anaconda3/etc/profile.d/conda.sh 2>/dev/null || true
    conda activate opencood 2>/dev/null || echo "⚠️  Warning: Could not activate opencood conda environment"
fi

# Start time
START_TIME=$(date +%s)
echo "⏰ Pipeline start time: $(date)"
echo ""

# =============================================================================
# Step 1: Check NS-3 Simulation Status
# =============================================================================
echo "📊 Step 1: Checking NS-3 Simulation Status"
echo "=================================="

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo "🔍 Checking NS-3 Simple V2X simulation status..."
if pgrep -f "simple-v2x-sim" > /dev/null; then
    echo "✅ NS-3 Simple V2X simulation is running"
    echo "   (Check NS-3 logs for SUMO trace usage)"
else
    echo "❌ NS-3 Simple V2X simulation is not running!"
    echo ""
    echo "   Please start NS-3 in another terminal first:"
    echo ""
    echo "   # Using SUMO trace (recommended):"
    echo "   cd ~/ns-allinone-3.40/ns-3.40"
    echo "   ./ns3 run 'simple-v2x-sim --sumoTrace=/path/to/ns3_opencood/sumo-traces/highway_7_vehicles_fcd.xml --simTime=120'"
    echo ""
    echo "   # Or using Random Walk:"
    echo "   ./ns3 run 'simple-v2x-sim --simTime=120'"
    echo ""
    exit 1
fi

# =============================================================================
# Step 2: NS-3 Guided Proven Data Augmentation
# =============================================================================
echo ""
echo "📊 Step 2: NS-3 Guided Proven Data Augmentation"
echo "=================================="

SCENARIO_NAME="ns3_proven_$(date +%Y%m%d_%H%M%S)"

# Check if source data exists
if [ ! -d "$SOURCE_DATA" ]; then
    echo "❌ Source data not found: $SOURCE_DATA"
    echo "   Please set SOURCE_DATA environment variable or modify the script"
    exit 1
fi

echo "🔄 Running NS-3 Guided Augmentation..."
echo "   Source (proven data): $SOURCE_DATA"
echo "   Output: $VALIDATE_ROOT/$SCENARIO_NAME"
echo "   NS-3 Port: $NS3_PORT"
echo "   Simulation Time: ${NS3_SIM_TIME}s"
echo ""

python3 "$SCRIPT_DIR/ns3_guided_proven_data_augmentation.py" \
    --source "$SOURCE_DATA" \
    --output-dir "$VALIDATE_ROOT" \
    --scenario-name "$SCENARIO_NAME" \
    --ns3-port "$NS3_PORT" \
    --ns3-sim-time "$NS3_SIM_TIME" \
    --ns3-step-time "$NS3_STEP_TIME" \
    --lidar-noise-std "$LIDAR_NOISE_STD"

AUG_EXIT_CODE=$?

if [ $AUG_EXIT_CODE -ne 0 ]; then
    echo "❌ Data augmentation failed (Exit Code: $AUG_EXIT_CODE)"
    exit 1
fi

echo "✅ NS-3 Guided Proven Data Augmentation Complete!"
V2X_DATA_PATH="$VALIDATE_ROOT/$SCENARIO_NAME"

# Data summary
if [ -d "$V2X_DATA_PATH" ]; then
    VEHICLE_COUNT=$(ls -1 "$V2X_DATA_PATH" | grep -E "^[0-9]+$" | wc -l)
    echo "🚗 Generated vehicles: $VEHICLE_COUNT"
    
    FIRST_VEHICLE=$(ls "$V2X_DATA_PATH" | grep -E "^[0-9]+$" | head -n1)
    if [ -d "$V2X_DATA_PATH/$FIRST_VEHICLE" ]; then
        FRAME_COUNT=$(ls -1 "$V2X_DATA_PATH/$FIRST_VEHICLE" | grep "\.yaml$" | wc -l)
        echo "📸 Generated frames: $FRAME_COUNT"
    fi
fi

# =============================================================================
# Step 3: Config Update and Inference Execution
# =============================================================================
echo ""
echo "📊 Step 3: Model Inference Execution"
echo "=================================="

CONFIG_PATH="$MODEL_DIR/config.yaml"

# Check if model directory exists
if [ ! -d "$MODEL_DIR" ]; then
    echo "❌ Model directory not found: $MODEL_DIR"
    echo "   Please set MODEL_DIR environment variable or modify the script"
    exit 1
fi

# Check if OpenCOOD exists
if [ ! -d "$OPENCOOD_DIR" ]; then
    echo "❌ OpenCOOD directory not found: $OPENCOOD_DIR"
    echo "   Please set OPENCOOD_DIR environment variable or install OpenCOOD"
    exit 1
fi

# Backup config
echo "💾 Backing up config file..."
BACKUP_CONFIG="${CONFIG_PATH}.backup_$(date +%Y%m%d_%H%M%S)"
cp "$CONFIG_PATH" "$BACKUP_CONFIG"
echo "   Backup: $BACKUP_CONFIG"

# Update validate path (OpenCOOD searches for scenarios under validate_dir)
echo "🔧 Updating config file (validate path)..."
sed -i "s|validate_dir:.*|validate_dir: '$VALIDATE_ROOT'|g" "$CONFIG_PATH"
echo "   Validate dir: $VALIDATE_ROOT"
echo "   Scenario: $SCENARIO_NAME"

echo "🚀 Running inference..."
echo "   Model: $(basename $MODEL_DIR)"
echo "   Model Dir: $MODEL_DIR"
echo "   Fusion Method: $FUSION_METHOD"
echo ""

INFERENCE_LOG="inference_ns3_proven_$(date +%Y%m%d_%H%M%S).log"
LOG_DIR="$(dirname "$SCRIPT_DIR")/logs"
mkdir -p "$LOG_DIR"

cd "$OPENCOOD_DIR"
python -m opencood.tools.inference \
    --model_dir "$MODEL_DIR" \
    --fusion_method "$FUSION_METHOD" 2>&1 | tee "$LOG_DIR/$INFERENCE_LOG"

INFERENCE_EXIT_CODE=$?

# Restore config
echo ""
echo "🔄 Restoring config file..."
mv "$BACKUP_CONFIG" "$CONFIG_PATH"

# =============================================================================
# Step 4: Results Summary
# =============================================================================
echo ""
echo "=========================================="
echo "Pipeline Execution Complete!"
echo "=========================================="

END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))
echo "⏱️  Total execution time: ${DURATION} seconds"
echo ""

if [ $INFERENCE_EXIT_CODE -eq 0 ]; then
    echo "✅ Inference successful!"
    echo ""
    echo "📊 Result files:"
    echo "   - Augmented data: $V2X_DATA_PATH"
    echo "   - Inference log: $LOG_DIR/$INFERENCE_LOG"
    echo ""
    echo "📈 Check performance:"
    echo "   grep 'AP@' $LOG_DIR/$INFERENCE_LOG"
    echo ""
    echo "📁 Data structure:"
    echo "   ls -lh $V2X_DATA_PATH"
else
    echo "❌ Inference failed (Exit Code: $INFERENCE_EXIT_CODE)"
    echo "   Check log: $LOG_DIR/$INFERENCE_LOG"
    exit 1
fi

echo ""
echo "🎉 Pipeline Complete!"
echo "=========================================="
echo ""
echo "💡 Tip: You can customize paths by setting environment variables:"
echo "   export SOURCE_DATA=/path/to/your/source/data"
echo "   export VALIDATE_ROOT=/path/to/output"
echo "   export MODEL_DIR=/path/to/your/model"
echo "   export OPENCOOD_DIR=/path/to/OpenCOOD"
echo "   export FUSION_METHOD=intermediate"
echo ""
