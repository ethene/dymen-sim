#!/bin/bash

# ==============================================================================
# NC9 MASTER SCRIPT - Run All Overhead Invariance Experiments
# ==============================================================================
# Purpose: Execute complete NC9 experimental suite
# Runtime: ~45 minutes total
#   - Satellite overhead (α): ~2 min (15 sims)
#   - Ground overhead (β/γ): ~40 min (45 sims)
# Output: results/nc9_overhead_invariance/{satellite_only,ground_only}/
# ==============================================================================

set -e  # Exit on error

# Colors for output (Mac compatible)
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Track overall status
OVERALL_STATUS=0

echo ""
echo "=============================================================="
echo -e "${BOLD}NC9: Overhead Invariance Experiments${NC}"
echo "=============================================================="
echo "This will run:"
echo "  - Satellite-only overhead (α coefficient): 15 simulations, ~2 min"
echo "  - Ground-only overhead (β/γ coefficients): 45 simulations, ~40 min"
echo ""
echo "Total: 60 simulations, ~45 minutes"
echo ""
echo "NOTE: Use caffeinate to prevent macOS sleep during long runs"
echo "      caffeinate -i bash scripts/run_all_nc9_experiments.sh"
echo "=============================================================="
echo ""

# Confirmation prompt
read -p "Continue? [y/N] " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Aborted."
    exit 1
fi

START_TIME=$(date +%s)

# ==============================================================================
# Phase 1: Satellite Overhead (α coefficient)
# ==============================================================================

echo ""
echo "=============================================================="
echo -e "${BOLD}[1/2] Satellite Overhead Measurement (α)${NC}"
echo "=============================================================="
echo ""

if bash ./scripts/run_nc9_satellite_overhead.sh; then
    echo -e "${GREEN}✓ Satellite overhead measurement complete${NC}"
else
    echo -e "${RED}✗ Satellite overhead measurement failed${NC}"
    OVERALL_STATUS=1
fi

# ==============================================================================
# Phase 2: Ground Overhead (β/γ coefficients)
# ==============================================================================

echo ""
echo "=============================================================="
echo -e "${BOLD}[2/2] Ground Overhead Measurement (β/γ)${NC}"
echo "=============================================================="
echo ""

if bash ./scripts/run_nc9_ground_overhead.sh; then
    echo -e "${GREEN}✓ Ground overhead measurement complete${NC}"
else
    echo -e "${RED}✗ Ground overhead measurement failed${NC}"
    OVERALL_STATUS=1
fi

# ==============================================================================
# Summary
# ==============================================================================

END_TIME=$(date +%s)
TOTAL_DURATION=$((END_TIME - START_TIME))

echo ""
echo "=============================================================="
echo -e "${BOLD}NC9 EXPERIMENTS COMPLETE${NC}"
echo "=============================================================="
echo ""

# Count files
SATELLITE_COUNT=$(ls results/nc9_overhead_invariance/satellite_only/*.csv 2>/dev/null | wc -l | tr -d ' ')
GROUND_COUNT=$(ls results/nc9_overhead_invariance/ground_only/*.csv 2>/dev/null | wc -l | tr -d ' ')
TOTAL_COUNT=$((SATELLITE_COUNT + GROUND_COUNT))

echo "Simulations completed:"
echo "  Satellite overhead (α): ${SATELLITE_COUNT}/15"
echo "  Ground overhead (β/γ): ${GROUND_COUNT}/45"
echo "  Total: ${TOTAL_COUNT}/60"
echo ""

# Calculate runtime
if [ $TOTAL_DURATION -lt 60 ]; then
    echo "Total runtime: ${TOTAL_DURATION}s"
else
    MINUTES=$((TOTAL_DURATION / 60))
    SECONDS=$((TOTAL_DURATION % 60))
    echo "Total runtime: ${MINUTES}m ${SECONDS}s"
fi
echo ""

echo "Results location:"
echo "  Satellite: results/nc9_overhead_invariance/satellite_only/"
echo "  Ground: results/nc9_overhead_invariance/ground_only/"
echo ""

if [ $OVERALL_STATUS -eq 0 ] && [ $TOTAL_COUNT -eq 60 ]; then
    echo -e "${GREEN}✓ All experiments completed successfully!${NC}"
    echo ""
    echo "Next steps:"
    echo "  1. Analyze results: python3 analysis/analyze_nc9_invariance.py"
    echo "  2. Generate figures: python3 analysis/plot_nc9_overhead.py"
    echo "  3. Expected finding: ANOVA p>0.80 (protocols equivalent)"
else
    echo -e "${YELLOW}⚠ Some experiments incomplete or failed${NC}"
    echo ""
    echo "Next steps:"
    echo "  1. Check error messages above"
    echo "  2. Re-run failed phase: ./scripts/run_nc9_{satellite,ground}_overhead.sh"
fi

echo "=============================================================="
echo ""

exit $OVERALL_STATUS
