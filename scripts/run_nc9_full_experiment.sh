#!/bin/bash

# ==============================================================================
# NC9 FULL EXPERIMENT SUITE
# ==============================================================================
# Purpose: Run complete Week 29 coefficient measurement experiments
# Runtime: ~45 minutes total
#   - Alpha (satellite-only): ~2 min (15 sims)
#   - Beta/Gamma (ground-only): ~40 min (45 sims)
# Total: 60 simulations
# ==============================================================================

# Note: Don't use 'set -e' - we want to continue on individual sim failures

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Timing
SCRIPT_START=$(date +%s)

# Experiment tracking
EXPERIMENTS=("alpha" "beta_gamma")
declare -A EXP_STATUS
declare -A EXP_DURATION

# ==============================================================================
# Helper Functions
# ==============================================================================

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

print_banner() {
    echo ""
    echo "=============================================================="
    echo -e "${BOLD}NC9: Overhead Invariance Experiment (Full Suite)${NC}"
    echo "=============================================================="
    echo "This script will:"
    echo "  1. Run alpha measurement (satellite-only, n=15)"
    echo "  2. Run beta/gamma measurement (ground-only, n=45)"
    echo ""
    echo "Total simulations: 60"
    echo "Total expected runtime: ~45 minutes"
    echo ""
    echo "NOTE: Use caffeinate to prevent macOS sleep during long runs"
    echo "      caffeinate -i bash scripts/run_nc9_full_experiment.sh"
    echo "=============================================================="
    echo ""
}

print_progress() {
    local current=$1
    local total=$2
    local name=$3

    echo ""
    echo -e "${BOLD}═══════════════════════════════════════════════${NC}"
    echo -e "${BOLD}Experiment ${current}/${total}: ${name}${NC}"
    echo -e "${BOLD}═══════════════════════════════════════════════${NC}"
    echo ""
}

# ==============================================================================
# Pre-flight Checks
# ==============================================================================

echo -e "${BLUE}Checking prerequisites...${NC}"

# Check build exists
if [ ! -f "./build/unified-simulation" ]; then
    echo -e "${RED}ERROR: Build not found${NC}"
    echo "Run 'make all' first"
    exit 1
fi

# Check scripts exist
for script in "run_nc9_satellite_overhead.sh" "run_nc9_ground_overhead.sh"; do
    if [ ! -f "./scripts/${script}" ]; then
        echo -e "${RED}ERROR: ./scripts/${script} not found${NC}"
        exit 1
    fi
done

# Check bc available (for time calculation)
if ! command -v bc > /dev/null 2>&1; then
    echo -e "${YELLOW}WARNING: bc not found (time formatting may be limited)${NC}"
fi

echo -e "${GREEN}✓ All prerequisites met${NC}"
echo ""

# ==============================================================================
# Main Execution
# ==============================================================================

print_banner

# Experiment 1: Alpha Measurement (Satellite-Only)
print_progress 1 2 "Alpha Measurement (Satellite-Only)"

EXP_START=$(date +%s)

if bash ./scripts/run_nc9_satellite_overhead.sh; then
    EXP_STATUS["alpha"]="SUCCESS"
    echo -e "${GREEN}✓ Alpha measurement complete${NC}"
else
    EXP_STATUS["alpha"]="FAILED"
    echo -e "${RED}✗ Alpha measurement failed${NC}"
    echo ""
    echo "You can resume by re-running this script (completed simulations will be skipped)"
    exit 1
fi

EXP_END=$(date +%s)
EXP_DURATION["alpha"]=$((EXP_END - EXP_START))
echo ""

# Experiment 2: Beta/Gamma Measurement (Ground-Only)
print_progress 2 2 "Beta/Gamma Measurement (Ground-Only)"

EXP_START=$(date +%s)

if bash ./scripts/run_nc9_ground_overhead.sh; then
    EXP_STATUS["beta_gamma"]="SUCCESS"
    echo -e "${GREEN}✓ Beta/Gamma measurement complete${NC}"
else
    EXP_STATUS["beta_gamma"]="FAILED"
    echo -e "${RED}✗ Beta/Gamma measurement failed${NC}"
    echo ""
    echo "You can resume by re-running this script (completed simulations will be skipped)"
    exit 1
fi

EXP_END=$(date +%s)
EXP_DURATION["beta_gamma"]=$((EXP_END - EXP_START))
echo ""

# ==============================================================================
# Final Summary
# ==============================================================================

SCRIPT_END=$(date +%s)
TOTAL_DURATION=$((SCRIPT_END - SCRIPT_START))

echo ""
echo "=============================================================="
echo -e "${BOLD}NC9 EXPERIMENT COMPLETE${NC}"
echo "=============================================================="

# Count files
ALPHA_COUNT=$(ls results/nc9_overhead_invariance/satellite_only/*.csv 2>/dev/null | wc -l | tr -d ' ')
BETA_GAMMA_COUNT=$(ls results/nc9_overhead_invariance/ground_only/*.csv 2>/dev/null | wc -l | tr -d ' ')
TOTAL_COUNT=$((ALPHA_COUNT + BETA_GAMMA_COUNT))

echo "Simulations completed:"
echo "  Alpha (satellite-only): ${ALPHA_COUNT}/15"
echo "  Beta/Gamma (ground-only): ${BETA_GAMMA_COUNT}/45"
echo "  Total: ${TOTAL_COUNT}/60"
echo ""

echo "Experiment runtimes:"
for exp in "${EXPERIMENTS[@]}"; do
    duration=${EXP_DURATION[$exp]}
    status=${EXP_STATUS[$exp]}

    if [ "$status" = "SUCCESS" ]; then
        echo -e "  ${exp}: $(format_time $duration) ${GREEN}✓${NC}"
    else
        echo -e "  ${exp}: $(format_time $duration) ${RED}✗${NC}"
    fi
done
echo ""

echo "Total runtime: $(format_time $TOTAL_DURATION)"
echo ""

# Overall status
if [ $TOTAL_COUNT -eq 60 ]; then
    echo -e "${GREEN}${BOLD}✓ ALL 60 SIMULATIONS COMPLETE${NC}"
    OVERALL_STATUS=0
elif [ $TOTAL_COUNT -gt 0 ]; then
    echo -e "${YELLOW}${BOLD}⚠ PARTIAL COMPLETION (${TOTAL_COUNT}/60)${NC}"
    echo "Re-run this script to complete remaining simulations"
    OVERALL_STATUS=1
else
    echo -e "${RED}${BOLD}✗ NO SIMULATIONS COMPLETED${NC}"
    OVERALL_STATUS=1
fi

echo ""
echo "Results location:"
echo "  Satellite-only: results/nc9_overhead_invariance/satellite_only/"
echo "  Ground-only: results/nc9_overhead_invariance/ground_only/"
echo ""

echo "Next steps:"
if [ $OVERALL_STATUS -eq 0 ]; then
    echo "  1. Analyze results: python3 analysis/analyze_nc9_invariance.py"
    echo "  2. Generate figures: python3 analysis/plot_nc9_overhead.py"
    echo "  3. Review: results/nc9_overhead_invariance/"
else
    echo "  1. Fix errors and re-run: bash scripts/run_nc9_full_experiment.sh"
    echo "  2. Check logs for failed simulations"
fi

echo "=============================================================="
echo ""

exit $OVERALL_STATUS
