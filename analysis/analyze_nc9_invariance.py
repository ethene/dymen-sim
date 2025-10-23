#!/usr/bin/env python3
"""
NC9 Overhead Invariance Analysis - NRL (Normalized Routing Load) comparison.

Analyzes routing overhead across AODV, OLSR, DSDV protocols using
packet-level tracing to differentiate control vs data traffic.
"""

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from pathlib import Path
from scipy import stats
from typing import Dict, List, Any


def load_single_csv(csv_path: Path) -> Dict[str, Any]:
    """Load single NC9 CSV file, extract NRL metrics."""
    # Read key-value CSV
    df = pd.read_csv(csv_path)

    # Convert to dictionary
    data = dict(zip(df['metric'], df['value']))

    # Extract required fields
    return {
        'ground_routing': str(data['ground_routing']),
        'seed': int(data['seed']),
        'pdr': float(data['pdr']),
        'avg_delay_ms': float(data['avg_delay_ms']),
        'nrl': float(data['nrl']),
        'data_bytes_tx': int(data['data_bytes_tx']),
        'control_bytes_tx': int(data['control_bytes_tx'])
    }


def load_all_data(results_dir: Path) -> pd.DataFrame:
    """Load NRL data from all CSV files."""
    # Find all CSV files
    all_files = sorted(results_dir.glob("*.csv"))

    # Load all files
    records = []
    for file in all_files:
        try:
            data = load_single_csv(file)
            records.append({
                'protocol': data['ground_routing'].upper(),
                'seed': data['seed'],
                'pdr': data['pdr'],
                'delay_ms': data['avg_delay_ms'],
                'nrl': data['nrl'],
                'data_bytes': data['data_bytes_tx'],
                'control_bytes': data['control_bytes_tx']
            })
        except Exception as e:
            print(f"Warning: Failed to load {file.name}: {e}")
            continue

    return pd.DataFrame(records)


def compute_summary_stats(df: pd.DataFrame) -> Dict[str, Dict[str, float]]:
    """Compute summary statistics per protocol."""
    results = {}

    for protocol in sorted(df['protocol'].unique()):
        protocol_data = df[df['protocol'] == protocol]['nrl']

        # Basic statistics
        mean = protocol_data.mean()
        std = protocol_data.std()
        sem = protocol_data.sem()
        n = len(protocol_data)

        # 95% confidence interval using t-distribution
        from scipy.stats import t
        t_critical = t.ppf(0.975, n - 1)  # Two-tailed, α=0.05
        margin = t_critical * sem

        results[protocol] = {
            'n': int(n),
            'mean': float(mean),
            'std': float(std),
            'sem': float(sem),
            'ci_lower': float(mean - margin),
            'ci_upper': float(mean + margin),
            'cv': float((std / mean) * 100) if mean > 0 else 0.0,
            'min': float(protocol_data.min()),
            'max': float(protocol_data.max())
        }

    return results


def run_anova(df: pd.DataFrame) -> Dict[str, Any]:
    """Run one-way ANOVA to test protocol differences."""
    # Extract NRL data for each protocol
    groups = [df[df['protocol'] == protocol]['nrl'].values
              for protocol in sorted(df['protocol'].unique())]

    # Run one-way ANOVA
    f_stat, p_value = stats.f_oneway(*groups)

    return {
        'f_stat': float(f_stat),
        'p_value': float(p_value),
        'significant': bool(p_value < 0.05)
    }


def check_practical_significance(df: pd.DataFrame, threshold_nrl: float = 0.05) -> Dict[str, Any]:
    """Check if NRL differences are practically meaningful.

    Args:
        threshold_nrl: Minimum NRL difference to be considered meaningful (default 5%)
    """
    # Compute mean NRL per protocol
    means = df.groupby('protocol')['nrl'].mean()

    # Find max difference
    max_mean = means.max()
    min_mean = means.min()
    max_diff = max_mean - min_mean

    # Compute ratio (how many times larger is max vs min)
    ratio = max_mean / min_mean if min_mean > 0 else float('inf')

    # Check if difference exceeds threshold
    matters = max_diff > threshold_nrl

    return {
        'max_mean': float(max_mean),
        'min_mean': float(min_mean),
        'max_diff': float(max_diff),
        'ratio': float(ratio),
        'matters': bool(matters),
        'threshold': float(threshold_nrl)
    }


def plot_nrl_comparison(df: pd.DataFrame, stats: Dict, output_dir: Path) -> None:
    """Generate NRL comparison plot (box plot + bar chart)."""
    fig, axes = plt.subplots(1, 2, figsize=(14, 5))

    # Sort protocols by mean NRL (ascending)
    protocol_order = sorted(stats.keys(), key=lambda p: stats[p]['mean'])

    # Plot 1: Box plot with individual points
    ax1 = axes[0]
    sns.boxplot(data=df, x='protocol', y='nrl', order=protocol_order, ax=ax1, palette='Set2')
    sns.stripplot(data=df, x='protocol', y='nrl', order=protocol_order, ax=ax1,
                  color='black', alpha=0.3, size=4)
    ax1.set_xlabel('Protocol', fontsize=12)
    ax1.set_ylabel('NRL (Normalized Routing Load)', fontsize=12)
    ax1.set_title('NRL Distribution by Protocol', fontsize=13, fontweight='bold')
    ax1.grid(True, alpha=0.3, axis='y')

    # Plot 2: Bar chart with error bars (95% CI)
    ax2 = axes[1]
    protocols = protocol_order
    means = [stats[p]['mean'] for p in protocols]
    errors = [(stats[p]['mean'] - stats[p]['ci_lower'],
               stats[p]['ci_upper'] - stats[p]['mean']) for p in protocols]
    errors = np.array(errors).T  # Transpose for matplotlib

    x_pos = np.arange(len(protocols))
    bars = ax2.bar(x_pos, means, yerr=errors, capsize=5, alpha=0.7, color=['#66c2a5', '#fc8d62', '#8da0cb'])
    ax2.set_xticks(x_pos)
    ax2.set_xticklabels(protocols)
    ax2.set_xlabel('Protocol', fontsize=12)
    ax2.set_ylabel('Mean NRL ± 95% CI', fontsize=12)
    ax2.set_title('Mean NRL Comparison', fontsize=13, fontweight='bold')
    ax2.grid(True, alpha=0.3, axis='y')

    # Add value labels on bars
    for i, (bar, mean) in enumerate(zip(bars, means)):
        height = bar.get_height()
        ax2.text(bar.get_x() + bar.get_width()/2., height + 0.01,
                f'{mean:.3f}', ha='center', va='bottom', fontsize=10, fontweight='bold')

    plt.tight_layout()

    # Save plots
    plt.savefig(output_dir / 'nc9_nrl_comparison.png', dpi=300, bbox_inches='tight')
    plt.savefig(output_dir / 'nc9_nrl_comparison.pdf', bbox_inches='tight')
    plt.close()

    print(f"  ✓ Saved: {output_dir / 'nc9_nrl_comparison.png'}")
    print(f"  ✓ Saved: {output_dir / 'nc9_nrl_comparison.pdf'}")


def plot_overhead_vs_pdr(df: pd.DataFrame, output_dir: Path) -> None:
    """Generate scatter plot: NRL vs PDR (trade-off analysis)."""
    fig, ax = plt.subplots(figsize=(8, 6))

    # Scatter plot with protocol colors
    for protocol in sorted(df['protocol'].unique()):
        protocol_data = df[df['protocol'] == protocol]
        ax.scatter(protocol_data['nrl'], protocol_data['pdr'],
                  label=protocol, alpha=0.6, s=50)

    ax.set_xlabel('NRL (Normalized Routing Load)', fontsize=12)
    ax.set_ylabel('PDR (%)', fontsize=12)
    ax.set_title('Overhead vs Reliability Trade-off', fontsize=13, fontweight='bold')
    ax.legend(fontsize=11)
    ax.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig(output_dir / 'nc9_overhead_vs_pdr.png', dpi=300, bbox_inches='tight')
    plt.savefig(output_dir / 'nc9_overhead_vs_pdr.pdf', bbox_inches='tight')
    plt.close()

    print(f"  ✓ Saved: {output_dir / 'nc9_overhead_vs_pdr.png'}")


def generate_text_report(df: pd.DataFrame, stats: Dict, anova: Dict,
                        practical: Dict, output_path: Path) -> None:
    """Generate plain text analysis report."""
    with open(output_path, 'w') as f:
        f.write("=" * 80 + "\n")
        f.write("Week 27 Overhead Analysis Report\n")
        f.write("NRL (Normalized Routing Load) Comparison\n")
        f.write("=" * 80 + "\n\n")

        # Dataset summary
        f.write("DATASET SUMMARY\n")
        f.write("-" * 80 + "\n")
        f.write(f"Total simulations:  {len(df)}\n")
        f.write(f"Protocols:          {', '.join(sorted(df['protocol'].unique()))}\n")
        f.write(f"Seeds per protocol: {df.groupby('protocol').size().min()}-{df.groupby('protocol').size().max()}\n")
        f.write("\n")

        # Per-protocol statistics
        f.write("PROTOCOL STATISTICS\n")
        f.write("-" * 80 + "\n")
        for protocol in sorted(stats.keys()):
            s = stats[protocol]
            f.write(f"\n{protocol} (n={s['n']}):\n")
            f.write(f"  Mean NRL:       {s['mean']:.4f} ({s['mean']*100:.2f}% overhead)\n")
            f.write(f"  Std deviation:  {s['std']:.4f}\n")
            f.write(f"  95% CI:         [{s['ci_lower']:.4f}, {s['ci_upper']:.4f}]\n")
            f.write(f"  Range:          [{s['min']:.4f}, {s['max']:.4f}]\n")
            f.write(f"  Coeff of var:   {s['cv']:.2f}%\n")

        # Protocol ranking
        f.write("\n" + "-" * 80 + "\n")
        f.write("PROTOCOL RANKING (by overhead, ascending)\n")
        f.write("-" * 80 + "\n")
        ranked = sorted(stats.items(), key=lambda x: x[1]['mean'])
        for i, (protocol, s) in enumerate(ranked, 1):
            f.write(f"{i}. {protocol:6s} - NRL: {s['mean']:.4f} ({s['mean']*100:.2f}% overhead)\n")

        # ANOVA results
        f.write("\n" + "-" * 80 + "\n")
        f.write("STATISTICAL SIGNIFICANCE (ANOVA)\n")
        f.write("-" * 80 + "\n")
        f.write(f"F-statistic:     {anova['f_stat']:.4f}\n")
        f.write(f"p-value:         {anova['p_value']:.6f}\n")
        f.write(f"Significant:     {'YES (p < 0.05)' if anova['significant'] else 'NO (p >= 0.05)'}\n")

        # Practical significance
        f.write("\n" + "-" * 80 + "\n")
        f.write("PRACTICAL SIGNIFICANCE\n")
        f.write("-" * 80 + "\n")
        f.write(f"Max NRL:         {practical['max_mean']:.4f} ({practical['max_mean']*100:.2f}%)\n")
        f.write(f"Min NRL:         {practical['min_mean']:.4f} ({practical['min_mean']*100:.2f}%)\n")
        f.write(f"Difference:      {practical['max_diff']:.4f} ({practical['max_diff']*100:.2f} percentage points)\n")
        f.write(f"Ratio:           {practical['ratio']:.2f}× (max/min)\n")
        f.write(f"Threshold:       {practical['threshold']:.4f} ({practical['threshold']*100:.0f}%)\n")
        f.write(f"Matters:         {'YES - significant overhead difference' if practical['matters'] else 'NO - protocols similar'}\n")

        # Interpretation
        f.write("\n" + "=" * 80 + "\n")
        f.write("INTERPRETATION\n")
        f.write("=" * 80 + "\n")

        if anova['significant'] and practical['matters']:
            f.write("✓ Protocols differ significantly in routing overhead.\n")
            f.write(f"  {ranked[0][0]} has lowest overhead ({ranked[0][1]['mean']*100:.1f}%)\n")
            f.write(f"  {ranked[-1][0]} has highest overhead ({ranked[-1][1]['mean']*100:.1f}%)\n")
            f.write(f"  Overhead ratio: {practical['ratio']:.1f}× difference\n\n")
            f.write("Publication claim:\n")
            f.write(f'  "{ranked[0][0]} achieves {(1 - ranked[0][1]["mean"]/ranked[-1][1]["mean"])*100:.0f}% lower overhead than {ranked[-1][0]}\n')
            f.write(f'   ({ranked[0][1]["mean"]*100:.1f}% vs {ranked[-1][1]["mean"]*100:.1f}%) with similar PDR."\n')
        elif anova['significant'] and not practical['matters']:
            f.write("⚠ Statistically significant but not practically meaningful.\n")
            f.write(f"  Difference ({practical['max_diff']*100:.1f}%) below threshold ({practical['threshold']*100:.0f}%)\n")
            f.write("  All protocols effectively equivalent for overhead.\n")
        else:
            f.write("✗ No significant difference in routing overhead.\n")
            f.write("  All protocols perform similarly (NRL invariant).\n")

        f.write("\n" + "=" * 80 + "\n")


def generate_csv_summary(stats: Dict, output_path: Path) -> None:
    """Generate CSV summary for easy import to papers."""
    records = []
    for protocol, s in stats.items():
        records.append({
            'protocol': protocol,
            'n': s['n'],
            'mean_nrl': s['mean'],
            'std_nrl': s['std'],
            'ci_lower': s['ci_lower'],
            'ci_upper': s['ci_upper'],
            'cv_percent': s['cv'],
            'min_nrl': s['min'],
            'max_nrl': s['max']
        })

    df = pd.DataFrame(records)
    df.to_csv(output_path, index=False)
    print(f"  ✓ Saved: {output_path}")


def main():
    """Run Week 27 overhead analysis."""
    # Configuration
    results_dir = Path("results/nc9_overhead_invariance/ground_only")
    output_dir = Path("results")
    output_dir.mkdir(exist_ok=True)

    print("=" * 80)
    print("Week 27 Overhead Analysis - NRL Comparison")
    print("=" * 80)
    print()

    # Load data
    print("Loading data from results/nc9_overhead_invariance/ground_only/...")
    df = load_all_data(results_dir)
    print(f"  ✓ Loaded {len(df)} simulations")
    print(f"  ✓ Protocols: {', '.join(sorted(df['protocol'].unique()))}")
    print()

    # Compute statistics
    print("Computing statistics...")
    stats = compute_summary_stats(df)
    print(f"  ✓ Statistics computed for {len(stats)} protocols")
    print()

    # Run ANOVA
    print("Running ANOVA...")
    anova = run_anova(df)
    print(f"  ✓ ANOVA complete: F={anova['f_stat']:.4f}, p={anova['p_value']:.6f}")
    print()

    # Check practical significance
    print("Checking practical significance...")
    practical = check_practical_significance(df)
    print(f"  ✓ Max difference: {practical['max_diff']:.4f} ({practical['ratio']:.2f}× ratio)")
    print()

    # Generate plots
    print("Generating plots...")
    plot_nrl_comparison(df, stats, output_dir)
    plot_overhead_vs_pdr(df, output_dir)
    print()

    # Generate reports
    print("Generating reports...")
    report_path = output_dir / "nc9_overhead_report.txt"
    generate_text_report(df, stats, anova, practical, report_path)
    print(f"  ✓ Saved: {report_path}")

    csv_path = output_dir / "nc9_nrl_summary.csv"
    generate_csv_summary(stats, csv_path)
    print()

    # Print summary
    print("=" * 80)
    print("SUMMARY")
    print("=" * 80)

    # Protocol ranking
    ranked = sorted(stats.items(), key=lambda x: x[1]['mean'])
    print("\nProtocol Ranking (by overhead):")
    for i, (protocol, s) in enumerate(ranked, 1):
        print(f"  {i}. {protocol:6s} - NRL: {s['mean']:.4f} ± {s['std']:.4f} ({s['mean']*100:.1f}% overhead)")

    # Statistical test
    print(f"\nANOVA: F={anova['f_stat']:.4f}, p={anova['p_value']:.6f}")
    if anova['significant']:
        print("  ✓ Protocols differ significantly (p < 0.05)")
    else:
        print("  ✗ No significant difference (p ≥ 0.05)")

    # Practical significance
    print(f"\nPractical Significance: {practical['ratio']:.2f}× overhead ratio")
    if practical['matters']:
        print(f"  ✓ Difference matters (Δ={practical['max_diff']*100:.1f}% > {practical['threshold']*100:.0f}%)")
    else:
        print(f"  ✗ Difference negligible (Δ={practical['max_diff']*100:.1f}% ≤ {practical['threshold']*100:.0f}%)")

    # Recommendation
    print("\n" + "=" * 80)
    print("RECOMMENDATION")
    print("=" * 80)
    if anova['significant'] and practical['matters']:
        print(f"✓ Clear winner: {ranked[0][0]} has {(1 - ranked[0][1]['mean']/ranked[-1][1]['mean'])*100:.0f}% lower overhead")
        print(f"  Use {ranked[0][0]} for overhead-sensitive deployments")
    else:
        print("✓ All protocols have similar overhead")
        print("  Choose based on other factors (PDR, delay, convergence)")

    print("\n" + "=" * 80)
    print(f"\nDetailed report: {report_path}")
    print(f"Plots: {output_dir}/nc9_nrl_comparison.png")
    print(f"CSV summary: {csv_path}")
    print()


if __name__ == '__main__':
    main()
