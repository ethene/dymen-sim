#!/bin/bash

# ==============================================================================
# NC9 SATELLITE OVERHEAD MEASUREMENT
# ==============================================================================
# Purpose: Run satellite-only simulations to measure α coefficient
# Runtime: ~2 minutes (15 simulations: OLSR × 15 seeds)
# Output: results/nc9_overhead_invariance/satellite_only/olsr_seed{1..15}.csv
# ==============================================================================

# Note: Don't use 'set -e' - we want to continue on individual sim failures

# Configuration
BUILD_PATH="./build/unified-simulation"
OUTPUT_DIR="./results/nc9_overhead_invariance/satellite_only"
PROTOCOL="olsr"
TEST_MODE=${TEST_MODE:-false}  # Set TEST_MODE=true for quick validation

if [ "$TEST_MODE" = "true" ]; then
    SEEDS=(1)  # Single seed for testing
else
    SEEDS=($(seq 1 15))  # Seeds 1-15 for production
fi

SIM_TIME=60  # seconds

# Colors for output (Mac compatible)
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Progress tracking
TOTAL_SIMS=${#SEEDS[@]}
COMPLETED=0
SKIPPED=0
FAILED=0
START_TIME=$(date +%s)

# ==============================================================================
# Helper Functions
# ==============================================================================

print_header() {
    echo ""
    echo "=================================================="
    echo "NC9: Satellite Overhead Measurement"
    echo "=================================================="
    echo "Total simulations: ${TOTAL_SIMS} (OLSR satellite-only)"
    echo "Estimated time: ~2 minutes"
    echo "Output directory: ${OUTPUT_DIR}"
    echo "=================================================="
    echo ""
}

calculate_eta() {
    local completed=$1
    if [ $completed -eq 0 ]; then
        echo "calculating..."
        return
    fi

    local current_time=$(date +%s)
    local elapsed=$((current_time - START_TIME))
    local avg_time_per_sim=$((elapsed / completed))
    local remaining=$((TOTAL_SIMS - completed - SKIPPED))
    local eta_seconds=$((avg_time_per_sim * remaining))

    # Show seconds if < 60, otherwise show minutes
    if [ $eta_seconds -lt 60 ]; then
        echo "${eta_seconds}s"
    else
        local eta_minutes=$(echo "scale=1; $eta_seconds / 60" | bc)
        # Handle leading zero for bc output like ".4"
        if [[ $eta_minutes == .* ]]; then
            echo "0${eta_minutes} min"
        else
            echo "${eta_minutes} min"
        fi
    fi
}

format_time() {
    local seconds=$1
    if [ $seconds -lt 60 ]; then
        echo "${seconds}s"
    else
        local minutes=$((seconds / 60))
        local secs=$((seconds % 60))
        echo "${minutes}m ${secs}s"
    fi
}

run_simulation() {
    local seed=$1
    local output_file="${OUTPUT_DIR}/${PROTOCOL}_seed${seed}.csv"

    # Skip if already exists (resume capability)
    if [ -f "$output_file" ]; then
        echo -e "${YELLOW}[SKIP]${NC} seed=${seed} (already exists)"
        SKIPPED=$((SKIPPED + 1))
        return 0
    fi

    # Run simulation
    local sim_start=$(date +%s)

    if ! $BUILD_PATH \
        --satellite-only=true \
        --isl-routing=$PROTOCOL \
        --satellites=24 \
        --time=$SIM_TIME \
        --seed=$seed \
        --output=$output_file \
        > /dev/null 2>&1; then
        echo -e "${RED}[FAIL]${NC} seed=${seed}"
        FAILED=$((FAILED + 1))
        return 1
    fi

    local sim_end=$(date +%s)
    local sim_duration=$((sim_end - sim_start))

    echo -e "${GREEN}[PASS]${NC} $(format_time $sim_duration)"
    return 0
}

print_summary() {
    local total_time=$(($(date +%s) - START_TIME))

    echo ""
    echo "=================================================="
    echo "SATELLITE OVERHEAD MEASUREMENT COMPLETE"
    echo "=================================================="
    echo "Successful: ${COMPLETED}/${TOTAL_SIMS}"
    echo "Skipped:    ${SKIPPED}"
    echo "Failed:     ${FAILED}"
    echo "Total runtime: $(format_time $total_time)"
    echo ""

    if [ $FAILED -eq 0 ]; then
        echo -e "${GREEN}✓ All simulations completed successfully${NC}"
    else
        echo -e "${YELLOW}⚠ Some simulations failed (see above)${NC}"
    fi

    echo ""
    echo "Next steps:"
    echo "  1. Run ground overhead: ./scripts/run_nc9_ground_overhead.sh"
    echo "  2. Or run all: ./scripts/run_nc9_full_experiment.sh"
    echo "=================================================="
    echo ""
}

# ==============================================================================
# Main Execution
# ==============================================================================

# Pre-flight checks
if [ ! -f "$BUILD_PATH" ]; then
    echo -e "${RED}ERROR: Build not found at $BUILD_PATH${NC}"
    echo "Run 'make all' first"
    exit 1
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Print header
print_header

# Run all simulations
for seed in "${SEEDS[@]}"; do
    # Calculate current index for progress display
    idx=$((seed))

    # Progress line
    eta=$(calculate_eta $COMPLETED)
    printf "[%2d/%2d] seed=%-2d ... " $idx $TOTAL_SIMS $seed

    # Run simulation
    if run_simulation "$seed"; then
        if [ $SKIPPED -eq 0 ] || [ $seed -ne ${SEEDS[-1]} ]; then
            # Only increment completed if not skipped
            if [ -f "${OUTPUT_DIR}/${PROTOCOL}_seed${seed}.csv" ]; then
                COMPLETED=$((COMPLETED + 1))
            fi
        fi

        # Show ETA only if not last simulation
        if [ $idx -lt $TOTAL_SIMS ]; then
            echo -n " ETA: $eta"
        fi
        echo ""
    else
        echo ""
        # Continue on failure (don't exit)
    fi
done

# Print summary
print_summary

# Exit with success if at least some simulations completed
if [ $COMPLETED -gt 0 ] || [ $SKIPPED -gt 0 ]; then
    exit 0
else
    exit 1
fi
