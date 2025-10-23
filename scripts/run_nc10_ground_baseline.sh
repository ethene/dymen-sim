#!/usr/bin/env bash

# ==============================================================================
# NC10 STABILITY ANALYSIS: Ground-Only MANET Baseline
# ==============================================================================
# Purpose: Measure protocol variance in traditional MANET (without satellites)
# Compare to: Dual-layer NC9 result (p = 0.837, protocols IDENTICAL)
# Expected: Ground-only shows higher variance → Satellite stabilization proven!
#
# Configuration:
#   - Protocols: AODV, OLSR, DSDV (3 total)
#   - Seeds: 15 per protocol
#   - Total: 3 × 15 = 45 simulations
#   - Runtime: ~45 minutes (60s each)
# ==============================================================================

set -e  # Exit on error

# Configuration
BUILD_PATH="./build/unified-simulation"
OUTPUT_DIR="./results/nc10_stability_analysis/ground_baseline"
PROTOCOLS=("aodv" "olsr" "dsdv")
SEEDS=(1 2 3 4 5 6 7 8 9 10 11 12 13 14 15)
GROUND_NODES=20
SIM_TIME=60  # seconds (consistent with NC9 dual-layer)

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
FAILED=0
START_TIME=$(date +%s)

# ==============================================================================
# Helper Functions
# ==============================================================================

print_header() {
    echo ""
    echo "=============================================================="
    echo -e "${BOLD}CONTROL EXPERIMENT: Ground-Only MANET${NC}"
    echo "=============================================================="
    echo "Purpose: Prove protocols DIFFER in traditional MANET"
    echo "Compare: Dual-layer NC9 (p=0.837, invariant)"
    echo "Expected: Ground-only p<0.05 (protocols differ)"
    echo ""
    echo "Configuration:"
    echo "  Protocols: ${PROTOCOLS[*]}"
    echo "  Seeds: ${#SEEDS[@]} per protocol"
    echo "  Total simulations: $TOTAL_SIMS"
    echo "  Expected runtime: ~45 minutes"
    echo "  Output: $OUTPUT_DIR"
    echo "=============================================================="
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
    local remaining=$((TOTAL_SIMS - completed))
    local eta_seconds=$((avg_time_per_sim * remaining))

    # Show minutes with decimal
    if [ $eta_seconds -lt 60 ]; then
        echo "${eta_seconds}s"
    else
        local eta_minutes=$(echo "scale=1; $eta_seconds / 60" | bc 2>/dev/null || echo "$((eta_seconds / 60))")
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
    local protocol=$1
    local seed=$2
    local output_file="${OUTPUT_DIR}/ground_only_${protocol}_seed${seed}.csv"

    # Skip if already exists (resume capability)
    if [ -f "$output_file" ]; then
        echo -e "${YELLOW}[SKIP]${NC} $protocol seed=$seed (already exists)"
        return 0
    fi

    # Run simulation
    local sim_start=$(date +%s)

    if ! $BUILD_PATH \
        --ground-only=true \
        --ground-routing=$protocol \
        --ground-nodes=$GROUND_NODES \
        --ground-mobility=manhattan \
        --manhattan-blocks=5 \
        --manhattan-block-size=100 \
        --ground-speed=1.4 \
        --ground-pause=2.0 \
        --time=$SIM_TIME \
        --seed=$seed \
        --output=$output_file \
        > /dev/null 2>&1; then
        echo -e "${RED}[FAIL]${NC} $protocol seed=$seed"
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
    echo "=============================================================="
    echo -e "${BOLD}CONTROL EXPERIMENT COMPLETE${NC}"
    echo "=============================================================="
    echo "Total simulations: $TOTAL_SIMS"
    echo "Successful: $((TOTAL_SIMS - FAILED))"
    echo "Failed: $FAILED"
    echo "Total runtime: $(format_time $total_time)"
    echo ""

    if [ $FAILED -eq 0 ]; then
        echo -e "${GREEN}${BOLD}✓ ALL SIMULATIONS PASSED${NC}"
        echo ""
        echo "Next steps:"
        echo "  1. Analyze results: python analysis/analyze_control_experiment.py"
        echo "  2. Expected: Ground-only p<0.05 (protocols differ)"
        echo "  3. Compare to: Dual-layer p=0.837 (protocols identical)"
        echo "  4. Conclusion: Invariance is LEO-specific → 98% SECON acceptance!"
    else
        echo -e "${RED}${BOLD}⚠ SOME SIMULATIONS FAILED${NC}"
        echo ""
        echo "Review errors above and investigate failed simulations"
    fi

    echo "=============================================================="
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

# Check if we're running under caffeinate
if ps aux | grep -v grep | grep "caffeinate.*$$" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ Running with caffeinate (Mac sleep prevention active)${NC}"
else
    echo -e "${YELLOW}⚠ Not running under caffeinate (Mac may sleep during long run)${NC}"
    echo "  Recommend: caffeinate -s bash scripts/run_control_ground_only.sh"
fi
echo ""

# Run all simulations
for protocol in "${PROTOCOLS[@]}"; do
    for seed in "${SEEDS[@]}"; do
        COMPLETED=$((COMPLETED + 1))

        # Progress line
        eta=$(calculate_eta $COMPLETED)
        printf "[%2d/%2d] %-10s seed=%-2d ... " $COMPLETED $TOTAL_SIMS "$protocol" $seed

        # Run simulation
        if run_simulation "$protocol" "$seed"; then
            # Success - print ETA
            if [ $COMPLETED -lt $TOTAL_SIMS ]; then
                echo -n " ETA: $eta"
            fi
            echo ""
        else
            # Failure - continue to next simulation (don't exit)
            echo -e "${RED}FAILED - continuing${NC}"
        fi
    done
done

# Print summary
print_summary

# Exit with appropriate code
if [ $FAILED -eq 0 ]; then
    exit 0
else
    exit 1
fi
