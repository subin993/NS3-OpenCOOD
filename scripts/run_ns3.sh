#!/bin/bash
# Helper script to run NS-3 simple-v2x-sim
# Usage: ./run_ns3.sh [scenario] [simTime]

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
V2X_DIR="$(dirname "$SCRIPT_DIR")"

# Default values
NS3_DIR="${NS3_DIR:-$HOME/subin/ns-allinone-3.40/ns-3.40}"
DEFAULT_SIM_TIME=120
DEFAULT_SCENARIO="highway_7_vehicles_fcd.xml"

# Parse arguments
SCENARIO="${1:-$DEFAULT_SCENARIO}"
SIM_TIME="${2:-$DEFAULT_SIM_TIME}"

# Check if NS-3 directory exists
if [ ! -d "$NS3_DIR" ]; then
    echo "Error: NS-3 directory not found at $NS3_DIR"
    echo "Please update NS3_DIR in this script or install NS-3."
    exit 1
fi

# Check if scenario file exists
if [[ "$SCENARIO" != *.xml ]]; then
    SCENARIO="$SCENARIO.xml"
fi

SCENARIO_PATH="$V2X_DIR/sumo-traces/$SCENARIO"

if [ ! -f "$SCENARIO_PATH" ]; then
    echo "Error: Scenario file not found: $SCENARIO_PATH"
    echo ""
    echo "Available scenarios:"
    ls -1 "$V2X_DIR/sumo-traces/"*.xml | xargs -n 1 basename
    exit 1
fi

echo "================================================"
echo "NS-3 V2X Simulation Launcher"
echo "================================================"
echo "NS-3 Directory: $NS3_DIR"
echo "Scenario: $SCENARIO"
echo "Simulation Time: $SIM_TIME seconds"
echo "================================================"
echo ""

# Activate conda environment if it exists
if command -v conda &> /dev/null; then
    if conda env list | grep -q "^opencood "; then
        echo "Activating conda environment 'opencood'..."
        eval "$(conda shell.bash hook)"
        conda activate opencood
    fi
fi

# Change to NS-3 directory
cd "$NS3_DIR"

# Run NS-3 simulation
echo "Starting NS-3 simulation..."
echo "Press Ctrl+C to stop"
echo ""

./ns3 run "simple-v2x-sim \
    --sumoTrace=$SCENARIO_PATH \
    --simTime=$SIM_TIME"
