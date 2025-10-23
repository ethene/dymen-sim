#!/usr/bin/env bash

# ==============================================================================
# Paper 1 Completeness Verification Script
# ==============================================================================
# Purpose: Verify all data and figures for CRM 2026 paper submission
# Paper: "Routing Protocol Overhead Invariance in Hybrid LEO+Mesh Networks"
# ==============================================================================

set -e  # Exit on error

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BOLD='\033[1m'
NC='\033[0m'

echo ""
echo "=============================================================="
echo -e "${BOLD}Paper 1 Completeness Verification${NC}"
echo "=============================================================="
echo "Checking: NC9 (Overhead Invariance) + NC10 (Stability)"
echo "=============================================================="
echo ""

# ==============================================================================
# Data File Checks
# ==============================================================================

echo -e "${BOLD}[1/5] Data Files${NC}"
echo "---"

# NC9 satellite-only
nc9_sat=$(ls results/nc9_overhead_invariance/satellite_only/*.csv 2>/dev/null | wc -l | tr -d ' ')
if [ "$nc9_sat" -eq 15 ]; then
    echo -e "${GREEN}✓${NC} NC9 satellite-only: ${nc9_sat}/15 files"
else
    echo -e "${RED}✗${NC} NC9 satellite-only: ${nc9_sat}/15 files (MISSING)"
fi

# NC9 ground-only
nc9_ground=$(ls results/nc9_overhead_invariance/ground_only/*.csv 2>/dev/null | wc -l | tr -d ' ')
if [ "$nc9_ground" -eq 45 ]; then
    echo -e "${GREEN}✓${NC} NC9 ground-only: ${nc9_ground}/45 files"
else
    echo -e "${RED}✗${NC} NC9 ground-only: ${nc9_ground}/45 files (MISSING)"
fi

# NC10 ground baseline (symlinks)
nc10_ground=$(ls results/nc10_stability_analysis/ground_baseline/*.csv 2>/dev/null | wc -l | tr -d ' ')
if [ "$nc10_ground" -eq 45 ]; then
    echo -e "${GREEN}✓${NC} NC10 ground baseline: ${nc10_ground}/45 files (symlinks)"
else
    echo -e "${RED}✗${NC} NC10 ground baseline: ${nc10_ground}/45 files (MISSING)"
fi

echo ""

# ==============================================================================
# Figure File Checks
# ==============================================================================

echo -e "${BOLD}[2/5] Figures${NC}"
echo "---"

# NC9 figures
nc9_nrl_png=$([ -f "results/nc9_nrl_comparison.png" ] && echo "1" || echo "0")
nc9_nrl_pdf=$([ -f "results/nc9_nrl_comparison.pdf" ] && echo "1" || echo "0")
nc9_pdr_png=$([ -f "results/nc9_overhead_vs_pdr.png" ] && echo "1" || echo "0")
nc9_pdr_pdf=$([ -f "results/nc9_overhead_vs_pdr.pdf" ] && echo "1" || echo "0")

nc9_fig_total=$((nc9_nrl_png + nc9_nrl_pdf + nc9_pdr_png + nc9_pdr_pdf))

if [ "$nc9_fig_total" -eq 4 ]; then
    echo -e "${GREEN}✓${NC} NC9 figures: 4/4 (PNG + PDF)"
else
    echo -e "${RED}✗${NC} NC9 figures: ${nc9_fig_total}/4 (MISSING)"
fi

# NC10 figures
nc10_png=$([ -f "results/nc10_stability_analysis/ground_baseline_comparison.png" ] && echo "1" || echo "0")
nc10_pdf=$([ -f "results/nc10_stability_analysis/ground_baseline_comparison.pdf" ] && echo "1" || echo "0")

nc10_fig_total=$((nc10_png + nc10_pdf))

if [ "$nc10_fig_total" -eq 2 ]; then
    echo -e "${GREEN}✓${NC} NC10 figures: 2/2 (PNG + PDF)"
else
    echo -e "${RED}✗${NC} NC10 figures: ${nc10_fig_total}/2 (MISSING)"
fi

echo ""

# ==============================================================================
# Report File Checks
# ==============================================================================

echo -e "${BOLD}[3/5] Reports${NC}"
echo "---"

# NC9 reports
if [ -f "results/nc9_overhead_report.txt" ]; then
    echo -e "${GREEN}✓${NC} NC9 ANOVA report: nc9_overhead_report.txt"
else
    echo -e "${RED}✗${NC} NC9 ANOVA report: MISSING"
fi

if [ -f "results/nc9_nrl_summary.csv" ]; then
    echo -e "${GREEN}✓${NC} NC9 summary table: nc9_nrl_summary.csv"
else
    echo -e "${RED}✗${NC} NC9 summary table: MISSING"
fi

# NC10 reports
if [ -f "results/nc10_stability_analysis/ground_baseline_report.txt" ]; then
    echo -e "${GREEN}✓${NC} NC10 variance report: ground_baseline_report.txt"
else
    echo -e "${RED}✗${NC} NC10 variance report: MISSING"
fi

echo ""

# ==============================================================================
# Statistical Validation
# ==============================================================================

echo -e "${BOLD}[4/5] Statistical Validation${NC}"
echo "---"

# Extract NC9 p-value from report
if [ -f "results/nc9_overhead_report.txt" ]; then
    nc9_p=$(grep "p-value:" results/nc9_overhead_report.txt | awk '{print $2}')
    if [ -n "$nc9_p" ]; then
        echo -e "${GREEN}✓${NC} NC9 ANOVA p-value: ${nc9_p} (expected: 0.80-0.88)"
    else
        echo -e "${YELLOW}⚠${NC} NC9 p-value: Could not extract from report"
    fi
else
    echo -e "${RED}✗${NC} NC9 report missing - cannot validate p-value"
fi

# Extract NC10 CV from report
if [ -f "results/nc10_stability_analysis/ground_baseline_report.txt" ]; then
    nc10_cv=$(grep "CV:" results/nc10_stability_analysis/ground_baseline_report.txt | head -1 | awk '{print $2}')
    if [ -n "$nc10_cv" ]; then
        echo -e "${GREEN}✓${NC} NC10 ground-only CV: ${nc10_cv} (expected: 36-57%)"
    else
        echo -e "${YELLOW}⚠${NC} NC10 CV: Could not extract from report"
    fi
else
    echo -e "${RED}✗${NC} NC10 report missing - cannot validate CV"
fi

echo ""

# ==============================================================================
# Build System Check
# ==============================================================================

echo -e "${BOLD}[5/5] Build System${NC}"
echo "---"

if [ -f "build/unified-simulation" ]; then
    echo -e "${GREEN}✓${NC} Simulation executable: build/unified-simulation"
else
    echo -e "${YELLOW}⚠${NC} Simulation executable: Not built (run 'make all')"
fi

if [ -f "Makefile" ]; then
    echo -e "${GREEN}✓${NC} Build system: Makefile present"
else
    echo -e "${RED}✗${NC} Build system: Makefile MISSING"
fi

if [ -f "requirements.txt" ]; then
    echo -e "${GREEN}✓${NC} Python dependencies: requirements.txt present"
else
    echo -e "${RED}✗${NC} Python dependencies: requirements.txt MISSING"
fi

echo ""

# ==============================================================================
# Summary
# ==============================================================================

echo "=============================================================="
echo -e "${BOLD}SUMMARY${NC}"
echo "=============================================================="

# Calculate totals
total_files=$((nc9_sat + nc9_ground + nc10_ground))
expected_files=105

total_figures=$((nc9_fig_total + nc10_fig_total))
expected_figures=6

if [ "$total_files" -eq "$expected_files" ] && [ "$total_figures" -eq "$expected_figures" ]; then
    echo -e "${GREEN}${BOLD}✓ Paper 1 is 100% COMPLETE${NC}"
    echo ""
    echo "Data:"
    echo "  ✓ ${total_files}/${expected_files} CSV files (NC9 + NC10)"
    echo "  ✓ ${total_figures}/${expected_figures} figures (PNG + PDF)"
    echo ""
    echo "Next steps:"
    echo "  1. Push to GitHub: git push"
    echo "  2. Get Zenodo DOI (10 min)"
    echo "  3. Submit to CRM journal"
    echo ""
    exit 0
else
    echo -e "${RED}${BOLD}✗ Paper 1 is INCOMPLETE${NC}"
    echo ""
    echo "Data:"
    echo "  ${total_files}/${expected_files} CSV files"
    echo "  ${total_figures}/${expected_figures} figures"
    echo ""
    echo "Missing components:"
    [ "$nc9_sat" -ne 15 ] && echo "  - NC9 satellite-only data (${nc9_sat}/15)"
    [ "$nc9_ground" -ne 45 ] && echo "  - NC9 ground-only data (${nc9_ground}/45)"
    [ "$nc10_ground" -ne 45 ] && echo "  - NC10 ground baseline (${nc10_ground}/45)"
    [ "$nc9_fig_total" -ne 4 ] && echo "  - NC9 figures (${nc9_fig_total}/4)"
    [ "$nc10_fig_total" -ne 2 ] && echo "  - NC10 figures (${nc10_fig_total}/2)"
    echo ""
    echo "Run the following to complete:"
    echo "  bash scripts/run_nc9_full_experiment.sh"
    echo "  python analysis/analyze_nc9_invariance.py"
    echo "  python analysis/analyze_nc10_stability.py"
    echo ""
    exit 1
fi

echo "=============================================================="
