#!/bin/bash

# ==============================================================================
# WORKAROUND: Complete Missing AODV/OLSR Simulations
# ==============================================================================
# Purpose: Run AODV/OLSR seeds 2-15 (28 simulations)
# Bug: run_nc9_overhead_invariance/ground_only.sh has broken loop structure
# Time: ~25-30 minutes
# ==============================================================================

BUILD_PATH="./build/unified-simulation"
OUTPUT_DIR="./results/nc9_overhead_invariance/ground_only"
SIM_TIME=60

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

# Check prerequisites
if [ ! -f "$BUILD_PATH" ]; then
    echo -e "${RED}ERROR: Build not found at $BUILD_PATH${NC}"
    exit 1
fi

mkdir -p "$OUTPUT_DIR"

echo ""
echo "========================================================"
echo "WORKAROUND: Complete Missing Ground Overhead Simulations"
echo "========================================================"
echo "Will run:"
echo "  - AODV seeds 2-15 (14 simulations)"
echo "  - OLSR seeds 2-15 (14 simulations)"
echo "Total: 28 simulations (~25-30 minutes)"
echo "========================================================"
echo ""

START_TIME=$(date +%s)
COMPLETED=0
SKIPPED=0
FAILED=0

# Function to run a single simulation
run_sim() {
    local protocol=$1
    local seed=$2
    local output_file="$OUTPUT_DIR/${protocol}_seed${seed}.csv"

    # Skip if exists
    if [ -f "$output_file" ]; then
        echo -e "[${YELLOW}SKIP${NC}] $protocol seed $seed (already exists)"
        SKIPPED=$((SKIPPED + 1))
        return
    fi

    # Run simulation
    local start=$(date +%s)
    printf "[RUN ] $protocol seed $seed ... "

    if $BUILD_PATH \
        --ground-only=true \
        --ground-routing=$protocol \
        --ground-nodes=20 \
        --ground-mobility=manhattan \
        --manhattan-blocks=5 \
        --manhattan-block-size=100 \
        --ground-speed=1.4 \
        --ground-pause=2.0 \
        --time=$SIM_TIME \
        --seed=$seed \
        --output=$output_file \
        > /dev/null 2>&1; then

        local end=$(date +%s)
        local duration=$((end - start))
        echo -e "${GREEN}PASS${NC} (${duration}s)"
        COMPLETED=$((COMPLETED + 1))
    else
        echo -e "${RED}FAIL${NC}"
        FAILED=$((FAILED + 1))
    fi
}

# Run AODV seeds 2-15
echo "Running AODV seeds 2-15..."
for seed in {2..15}; do
    run_sim "aodv" $seed
done

echo ""
echo "Running OLSR seeds 2-15..."
# Run OLSR seeds 2-15
for seed in {2..15}; do
    run_sim "olsr" $seed
done

# Summary
TOTAL_TIME=$(($(date +%s) - START_TIME))
echo ""
echo "========================================================"
echo "COMPLETION STATUS"
echo "========================================================"
echo "Completed: $COMPLETED"
echo "Skipped:   $SKIPPED"
echo "Failed:    $FAILED"
echo "Runtime:   ${TOTAL_TIME}s ($((TOTAL_TIME / 60))m $((TOTAL_TIME % 60))s)"
echo ""

# Verify total count
AODV_COUNT=$(ls $OUTPUT_DIR/aodv_seed*.csv 2>/dev/null | wc -l | tr -d ' ')
OLSR_COUNT=$(ls $OUTPUT_DIR/olsr_seed*.csv 2>/dev/null | wc -l | tr -d ' ')
DSDV_COUNT=$(ls $OUTPUT_DIR/dsdv_seed*.csv 2>/dev/null | wc -l | tr -d ' ')

echo "File counts:"
echo "  AODV: $AODV_COUNT/15"
echo "  OLSR: $OLSR_COUNT/15"
echo "  DSDV: $DSDV_COUNT/15"
echo ""

if [ $AODV_COUNT -eq 15 ] && [ $OLSR_COUNT -eq 15 ] && [ $DSDV_COUNT -eq 15 ]; then
    echo -e "${GREEN}✓ ALL SIMULATIONS COMPLETE (45/45)${NC}"
    echo ""
    echo "Next step: Analyze NC9 results"
    echo "  python3 analysis/analyze_nc9_overhead.py"
else
    echo -e "${YELLOW}⚠ INCOMPLETE${NC}"
    echo "Re-run this script to complete remaining simulations"
fi

echo "========================================================"
