#!/bin/bash

# ==============================================================================
# NC9 GROUND OVERHEAD MEASUREMENT
# ==============================================================================
# Purpose: Run ground-only simulations to measure β and γ coefficients
# Runtime: ~40 minutes (45 simulations: 3 protocols × 15 seeds)
# Output: results/nc9_overhead_invariance/ground_only/{protocol}_seed{1..15}.csv
# ==============================================================================

# Note: Don't use 'set -e' - we want to continue on individual sim failures

# Configuration
BUILD_PATH="./build/unified-simulation"
OUTPUT_DIR="./results/nc9_overhead_invariance/ground_only"
PROTOCOLS=("aodv" "olsr" "dsdv")
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
BOLD='\033[1m'
NC='\033[0m' # No Color

# Progress tracking
TOTAL_SIMS=$((${#PROTOCOLS[@]} * ${#SEEDS[@]}))
COMPLETED=0
SKIPPED=0
FAILED=0
START_TIME=$(date +%s)

# Per-protocol tracking
declare -A PROTOCOL_COMPLETED
declare -A PROTOCOL_TOTAL
for protocol in "${PROTOCOLS[@]}"; do
    PROTOCOL_COMPLETED[$protocol]=0
    PROTOCOL_TOTAL[$protocol]=${#SEEDS[@]}
done

# ==============================================================================
# Helper Functions
# ==============================================================================

print_header() {
    echo ""
    echo "=================================================="
    echo "NC9: Ground Overhead Measurement"
    echo "=================================================="
    echo "Total simulations: ${TOTAL_SIMS} (${#PROTOCOLS[@]} protocols × ${#SEEDS[@]} seeds)"
    echo "Estimated time: ~40 minutes (full) or ~2 min (test mode)"
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

print_protocol_progress() {
    # Print progress for each protocol on same line
    local progress_str=""
    for protocol in "${PROTOCOLS[@]}"; do
        local completed=${PROTOCOL_COMPLETED[$protocol]}
        local total=${PROTOCOL_TOTAL[$protocol]}
        progress_str="${progress_str}${protocol}=${completed}/${total} "
    done
    echo -ne "\r${BOLD}[Progress: ${progress_str}]${NC}"
}

run_simulation() {
    local protocol=$1
    local seed=$2
    local output_file="${OUTPUT_DIR}/${protocol}_seed${seed}.csv"

    # Skip if already exists (resume capability)
    if [ -f "$output_file" ]; then
        echo -e "${YELLOW}[SKIP]${NC} ${protocol} seed=${seed} (already exists)"
        SKIPPED=$((SKIPPED + 1))
        PROTOCOL_COMPLETED[$protocol]=$((${PROTOCOL_COMPLETED[$protocol]} + 1))
        return 2  # Return 2 for SKIP (different from success/fail)
    fi

    # Run simulation
    local sim_start=$(date +%s)

    if ! $BUILD_PATH \
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
        echo -e "${RED}[FAIL]${NC} ${protocol} seed=${seed}"
        FAILED=$((FAILED + 1))
        return 1
    fi

    local sim_end=$(date +%s)
    local sim_duration=$((sim_end - sim_start))

    echo -e "${GREEN}[PASS]${NC} $(format_time $sim_duration)"
    COMPLETED=$((COMPLETED + 1))  # Increment here instead of in main loop
    PROTOCOL_COMPLETED[$protocol]=$((${PROTOCOL_COMPLETED[$protocol]} + 1))
    return 0
}

print_summary() {
    local total_time=$(($(date +%s) - START_TIME))

    echo ""
    echo ""
    echo "=================================================="
    echo "BETA/GAMMA MEASUREMENT COMPLETE"
    echo "=================================================="
    echo "Successful: ${COMPLETED}/${TOTAL_SIMS}"
    echo "Skipped:    ${SKIPPED}"
    echo "Failed:     ${FAILED}"
    echo "Total runtime: $(format_time $total_time)"
    echo ""

    # Per-protocol summary
    echo "Per-protocol results:"
    for protocol in "${PROTOCOLS[@]}"; do
        local completed=${PROTOCOL_COMPLETED[$protocol]}
        local total=${PROTOCOL_TOTAL[$protocol]}
        echo "  ${protocol}: ${completed}/${total} completed"
    done
    echo ""

    if [ $FAILED -eq 0 ]; then
        echo -e "${GREEN}✓ All simulations completed successfully${NC}"
    else
        echo -e "${YELLOW}⚠ Some simulations failed (see above)${NC}"
    fi

    echo ""
    echo "Next steps:"
    echo "  1. Run satellite overhead: ./scripts/run_nc9_satellite_overhead.sh"
    echo "  2. Or run all: ./scripts/run_nc9_full_experiment.sh"
    echo "  3. Analyze: python3 analysis/analyze_nc9_invariance.py"
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
sim_count=0
for protocol in "${PROTOCOLS[@]}"; do
    for seed in "${SEEDS[@]}"; do
        sim_count=$((sim_count + 1))

        # Progress line
        eta=$(calculate_eta $((COMPLETED + SKIPPED)))
        printf "[%2d/%2d] %-6s seed=%-2d ... " $sim_count $TOTAL_SIMS "$protocol" $seed

        # Run simulation
        run_simulation "$protocol" "$seed"

        # Show ETA and protocol progress (for all outcomes except final sim)
        if [ $sim_count -lt $TOTAL_SIMS ]; then
            echo -n " ETA: $eta  "
            print_protocol_progress
        fi
        echo ""

        # Continue on failure (don't exit)
    done
done

# Print summary
print_summary

# Exit with success if at least some simulations completed
if [ $COMPLETED -gt 0 ] || [ $SKIPPED -gt 0 ]; then
    exit 0
else
    exit 1
fi
