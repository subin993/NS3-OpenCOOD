#!/bin/bash
# Quick Start Script for V2X Cooperative Perception Pipeline
# This script helps you get started quickly

set -e

echo "================================================"
echo "V2X Cooperative Perception - Quick Start Setup"
echo "================================================"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
V2X_DIR="$(dirname "$SCRIPT_DIR")"

echo "V2X Directory: $V2X_DIR"
echo ""

# Step 1: Check prerequisites
echo "Step 1: Checking prerequisites..."
echo "================================="

# Check if NS-3 is installed
if [ ! -d "$HOME/subin/ns-allinone-3.40/ns-3.40" ] && [ ! -d "$HOME/ns-allinone-3.40/ns-3.40" ]; then
    echo -e "${RED}✗ NS-3.40 not found${NC}"
    echo "  Please install NS-3.40 first. See INSTALL.md for instructions."
    exit 1
else
    if [ -d "$HOME/subin/ns-allinone-3.40/ns-3.40" ]; then
        NS3_DIR="$HOME/subin/ns-allinone-3.40/ns-3.40"
    else
        NS3_DIR="$HOME/ns-allinone-3.40/ns-3.40"
    fi
    echo -e "${GREEN}✓ NS-3.40 found at $NS3_DIR${NC}"
fi

# Check if conda is installed
if ! command -v conda &> /dev/null; then
    echo -e "${YELLOW}⚠ Conda not found. You can still use pip.${NC}"
    USE_CONDA=false
else
    echo -e "${GREEN}✓ Conda found${NC}"
    USE_CONDA=true
fi

# Check if Python 3 is installed
if ! command -v python3 &> /dev/null; then
    echo -e "${RED}✗ Python 3 not found${NC}"
    exit 1
else
    echo -e "${GREEN}✓ Python 3 found: $(python3 --version)${NC}"
fi

echo ""

# Step 2: Copy NS-3 examples
echo "Step 2: Copying NS-3 examples..."
echo "================================="

NS3_EXAMPLES_DIR="$NS3_DIR/contrib/opengym/examples/simple-v2x"

if [ ! -d "$NS3_DIR/contrib/opengym" ]; then
    echo -e "${RED}✗ ns3-gym (OpenGym) not found${NC}"
    echo "  Please install ns3-gym first. See INSTALL.md for instructions."
    exit 1
fi

mkdir -p "$NS3_EXAMPLES_DIR"
cp "$V2X_DIR/ns3-opengym/examples/"* "$NS3_EXAMPLES_DIR/"

echo -e "${GREEN}✓ NS-3 examples copied to $NS3_EXAMPLES_DIR${NC}"
echo ""

# Step 3: Build NS-3
echo "Step 3: Building NS-3..."
echo "========================"

read -p "Do you want to rebuild NS-3? This may take several minutes. (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    cd "$NS3_DIR"
    ./ns3 configure --enable-examples
    ./ns3 build
    echo -e "${GREEN}✓ NS-3 build complete${NC}"
else
    echo -e "${YELLOW}⚠ Skipping NS-3 build${NC}"
fi

echo ""

# Step 4: Setup Python environment
echo "Step 4: Setting up Python environment..."
echo "========================================="

if [ "$USE_CONDA" = true ]; then
    read -p "Create conda environment 'opencood'? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        conda create -n opencood python=3.8 -y
        echo -e "${GREEN}✓ Conda environment 'opencood' created${NC}"
        echo ""
        echo "To activate: conda activate opencood"
        echo "Then install requirements: pip install -r requirements.txt"
    fi
else
    echo "Please install Python packages manually:"
    echo "  pip install -r $V2X_DIR/requirements.txt"
fi

echo ""

# Step 5: Create example data structure
echo "Step 5: Setting up data directories..."
echo "======================================="

echo "Data directories:"
echo "  Source data: $V2X_DIR/data/source/"
echo "  Output data: $V2X_DIR/data/validate/"
echo ""
echo "Please place your OPV2V data in:"
echo "  $V2X_DIR/data/source/your_scenario_name/"
echo ""

# Step 6: Summary
echo ""
echo "================================================"
echo "Setup Complete!"
echo "================================================"
echo ""
echo "Next steps:"
echo ""
echo "1. Install Python packages:"
echo "   conda activate opencood"
echo "   pip install -r $V2X_DIR/requirements.txt"
echo ""
echo "2. Place your source data in:"
echo "   $V2X_DIR/data/source/"
echo ""
echo "3. Run NS-3 simulation (Terminal 1):"
echo "   cd $NS3_DIR"
echo "   ./ns3 run 'simple-v2x-sim \\"
echo "       --sumoTrace=$V2X_DIR/sumo-traces/highway_7_vehicles_fcd.xml \\"
echo "       --simTime=120'"
echo ""
echo "4. Run pipeline (Terminal 2):"
echo "   cd $V2X_DIR/python-scripts"
echo "   ./ns3_proven_pipeline.sh"
echo ""
echo "For detailed instructions, see:"
echo "  - INSTALL.md for installation"
echo "  - docs/USAGE.md for usage examples"
echo ""
