#!/usr/bin/env python3
"""
visualize_benchmarks.py
Generates comparative charts from RSA vs ECC benchmark CSV data.
Author: Leon Elliott Fuller
Date: 2026-02-28

Usage:
    python3 visualize_benchmarks.py <summary.csv> <raw.csv> <output_dir>
    python3 visualize_benchmarks.py results/summary_latest.csv results/raw_latest.csv results/
"""

import sys
import os
import pandas as pd
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

# ============================================================================
# STYLE CONFIGURATION
# ============================================================================

# Color palettes
RSA_COLOR = '#2563EB'       # Blue
ECC_COLOR = '#DC2626'       # Red
RSA_LIGHT = '#93C5FD'       # Light blue
ECC_LIGHT = '#FCA5A5'       # Light red

# Per-param colors for detailed charts
PARAM_COLORS = {
    '1024-bit': '#93C5FD',
    '2048-bit': '#60A5FA',
    '3072-bit': '#3B82F6',
    '4096-bit': '#1D4ED8',
    'secp256k1': '#F87171',
    'P-256':     '#EF4444',
    'P-384':     '#B91C1C',
}

# Security level groupings (NIST SP 800-57)
# Maps equivalent security level to RSA params and ECC params
SECURITY_LEVELS = {
    128: {'RSA': '3072-bit', 'ECC': ['secp256k1', 'P-256']},
    192: {'RSA': '4096-bit', 'ECC': ['P-384']},
}

# Common operations that both RSA and ECC share
COMMON_OPS = ['keygen', 'sign', 'verify']

plt.rcParams.update({
    'figure.facecolor': 'white',
    'axes.facecolor':   'white',
    'axes.grid':        True,
    'grid.alpha':       0.3,
    'font.size':        11,
    'axes.titlesize':   13,
    'axes.labelsize':   11,
    'figure.dpi':       150,
})


# ============================================================================
# DATA LOADING
# ============================================================================

def load_data(summary_path, raw_path=None):
    """Load CSV benchmark data into DataFrames."""
    df = pd.read_csv(summary_path)
    df_raw = pd.read_csv(raw_path) if raw_path and os.path.exists(raw_path) else None
    return df, df_raw


# ============================================================================
# CHART 1: Key Generation Comparison (all params, log scale)
# ============================================================================

def chart_keygen_comparison(df, output_dir):
    """Bar chart comparing key generation times across all RSA sizes and ECC curves."""
    keygen = df[df['operation'] == 'keygen'].copy()
    if keygen.empty:
        return

    fig, ax = plt.subplots(figsize=(10, 6))

    labels = keygen['params'].tolist()
    values = keygen['median_us'].tolist()
    colors = [PARAM_COLORS.get(p, '#888888') for p in labels]

    bars = ax.bar(range(len(labels)), values, color=colors, edgecolor='white',
                  linewidth=0.5, width=0.7)

    # Add value annotations
    for bar, val in zip(bars, values):
        label = _format_time(val)
        y_pos = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2, y_pos * 1.05,
                label, ha='center', va='bottom', fontsize=9, fontweight='bold')

    ax.set_xticks(range(len(labels)))
    ax.set_xticklabels(labels, rotation=0)
    ax.set_ylabel('Time (us) - log scale')
    ax.set_yscale('log')
    ax.set_title('Key Generation Time: RSA vs ECC')

    # Add algorithm group labels
    rsa_count = len(keygen[keygen['algorithm'] == 'RSA'])
    ecc_count = len(keygen[keygen['algorithm'] == 'ECC'])
    if rsa_count > 0 and ecc_count > 0:
        ax.axvline(x=rsa_count - 0.5, color='gray', linestyle='--', alpha=0.5)
        ax.text(rsa_count/2 - 0.5, ax.get_ylim()[0] * 1.5, 'RSA',
                ha='center', fontsize=12, fontstyle='italic', color=RSA_COLOR)
        ax.text(rsa_count + ecc_count/2 - 0.5, ax.get_ylim()[0] * 1.5, 'ECC',
                ha='center', fontsize=12, fontstyle='italic', color=ECC_COLOR)

    plt.tight_layout()
    path = os.path.join(output_dir, 'chart_keygen_comparison.png')
    plt.savefig(path, bbox_inches='tight')
    plt.close()
    print(f'  Saved: {path}')


# ============================================================================
# CHART 2: Sign & Verify at Equivalent Security Levels
# ============================================================================

def chart_sign_verify_comparison(df, output_dir):
    """Grouped bar chart comparing sign and verify at equivalent security levels."""
    fig, axes = plt.subplots(1, 2, figsize=(14, 6))

    for idx, op in enumerate(['sign', 'verify']):
        ax = axes[idx]
        op_data = df[df['operation'] == op]
        if op_data.empty:
            continue

        groups = []
        rsa_vals = []
        ecc_vals = []
        ecc_labels_list = []

        for sec_level, mapping in sorted(SECURITY_LEVELS.items()):
            rsa_param = mapping['RSA']
            ecc_params = mapping['ECC']

            rsa_row = op_data[(op_data['algorithm'] == 'RSA') &
                              (op_data['params'] == rsa_param)]
            if rsa_row.empty:
                continue

            rsa_val = rsa_row['median_us'].values[0]

            for ecc_param in ecc_params:
                ecc_row = op_data[(op_data['algorithm'] == 'ECC') &
                                  (op_data['params'] == ecc_param)]
                if ecc_row.empty:
                    continue

                ecc_val = ecc_row['median_us'].values[0]
                groups.append(f'{sec_level}-bit\n{rsa_param} vs {ecc_param}')
                rsa_vals.append(rsa_val)
                ecc_vals.append(ecc_val)

        if not groups:
            continue

        x = np.arange(len(groups))
        width = 0.35

        bars_rsa = ax.bar(x - width/2, rsa_vals, width, label='RSA',
                          color=RSA_COLOR, edgecolor='white')
        bars_ecc = ax.bar(x + width/2, ecc_vals, width, label='ECC',
                          color=ECC_COLOR, edgecolor='white')

        # Annotate values
        for bars in [bars_rsa, bars_ecc]:
            for bar in bars:
                h = bar.get_height()
                ax.text(bar.get_x() + bar.get_width()/2, h * 1.02,
                        _format_time(h), ha='center', va='bottom', fontsize=8)

        ax.set_xticks(x)
        ax.set_xticklabels(groups, fontsize=9)
        ax.set_ylabel('Time (us)')
        ax.set_title(f'{op.capitalize()} Performance')
        ax.legend()

        # Use log scale if difference is > 10x
        all_vals = rsa_vals + ecc_vals
        if all_vals and max(all_vals) / max(min(all_vals), 1) > 10:
            ax.set_yscale('log')
            ax.set_ylabel('Time (us) - log scale')

    plt.suptitle('Digital Signatures: RSA vs ECC at Equivalent Security', fontsize=14)
    plt.tight_layout()
    path = os.path.join(output_dir, 'chart_sign_verify_comparison.png')
    plt.savefig(path, bbox_inches='tight')
    plt.close()
    print(f'  Saved: {path}')


# ============================================================================
# CHART 3: RSA Scaling with Key Size
# ============================================================================

def chart_rsa_scaling(df, output_dir):
    """Line chart showing how RSA operations scale with key size."""
    rsa = df[df['algorithm'] == 'RSA'].copy()
    if rsa.empty:
        return

    # Extract numeric key size from params like "2048-bit"
    rsa['key_bits'] = rsa['params'].str.replace('-bit', '').astype(int)
    rsa = rsa.sort_values('key_bits')

    ops_to_plot = ['keygen', 'encrypt', 'decrypt_crt', 'sign', 'verify']
    op_labels = {
        'keygen': 'Key Generation',
        'encrypt': 'Encrypt',
        'decrypt_crt': 'Decrypt (CRT)',
        'sign': 'Sign (CRT)',
        'verify': 'Verify',
    }
    op_styles = {
        'keygen': ('o-', '#1D4ED8'),
        'encrypt': ('s--', '#059669'),
        'decrypt_crt': ('^--', '#7C3AED'),
        'sign': ('D-', '#EA580C'),
        'verify': ('v-', '#DB2777'),
    }

    fig, ax = plt.subplots(figsize=(10, 6))

    for op in ops_to_plot:
        op_data = rsa[rsa['operation'] == op]
        if op_data.empty:
            continue
        style, color = op_styles.get(op, ('o-', 'gray'))
        ax.plot(op_data['key_bits'], op_data['median_us'],
                style, label=op_labels.get(op, op), color=color,
                markersize=7, linewidth=2)

    ax.set_xlabel('RSA Key Size (bits)')
    ax.set_ylabel('Time (us) - log scale')
    ax.set_yscale('log')
    ax.set_xticks([1024, 2048, 3072, 4096])
    ax.legend(loc='upper left')
    ax.set_title('RSA Operation Scaling with Key Size')

    plt.tight_layout()
    path = os.path.join(output_dir, 'chart_rsa_scaling.png')
    plt.savefig(path, bbox_inches='tight')
    plt.close()
    print(f'  Saved: {path}')


# ============================================================================
# CHART 4: ECC Operations Across Curves
# ============================================================================

def chart_ecc_curves(df, output_dir):
    """Grouped bar chart: all ECC operations across the 3 curves."""
    ecc = df[df['algorithm'] == 'ECC'].copy()
    if ecc.empty:
        return

    # Exclude sha256_hash (it's the same for all curves)
    ops = ['keygen', 'scalar_mult', 'ecdh', 'sign', 'verify']
    op_labels = {
        'keygen': 'KeyGen',
        'scalar_mult': 'Scalar Mult',
        'ecdh': 'ECDH',
        'sign': 'ECDSA Sign',
        'verify': 'ECDSA Verify',
    }

    curves = ecc['params'].unique().tolist()
    curve_colors = [PARAM_COLORS.get(c, '#888') for c in curves]

    fig, ax = plt.subplots(figsize=(12, 6))

    x = np.arange(len(ops))
    n_curves = len(curves)
    total_width = 0.75
    bar_width = total_width / n_curves

    for i, (curve, color) in enumerate(zip(curves, curve_colors)):
        curve_data = ecc[ecc['params'] == curve]
        vals = []
        for op in ops:
            row = curve_data[curve_data['operation'] == op]
            vals.append(row['median_us'].values[0] if not row.empty else 0)

        offset = (i - n_curves/2 + 0.5) * bar_width
        bars = ax.bar(x + offset, vals, bar_width, label=curve,
                      color=color, edgecolor='white')

        for bar, val in zip(bars, vals):
            if val > 0:
                ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() * 1.02,
                        _format_time(val), ha='center', va='bottom', fontsize=7)

    ax.set_xticks(x)
    ax.set_xticklabels([op_labels.get(op, op) for op in ops])
    ax.set_ylabel('Time (us)')
    ax.set_title('ECC Operations by Curve')
    ax.legend()

    plt.tight_layout()
    path = os.path.join(output_dir, 'chart_ecc_curves.png')
    plt.savefig(path, bbox_inches='tight')
    plt.close()
    print(f'  Saved: {path}')


# ============================================================================
# CHART 5: Speedup Ratios (RSA vs ECC at equivalent security)
# ============================================================================

def chart_speedup_ratios(df, output_dir):
    """Horizontal bar chart showing ECC speedup (or slowdown) vs RSA."""
    fig, ax = plt.subplots(figsize=(10, 6))

    entries = []

    for sec_level, mapping in sorted(SECURITY_LEVELS.items()):
        rsa_param = mapping['RSA']
        ecc_params = mapping['ECC']

        for op in COMMON_OPS:
            rsa_row = df[(df['algorithm'] == 'RSA') & (df['operation'] == op) &
                         (df['params'] == rsa_param)]
            if rsa_row.empty:
                continue

            rsa_val = rsa_row['median_us'].values[0]

            for ecc_param in ecc_params:
                ecc_row = df[(df['algorithm'] == 'ECC') & (df['operation'] == op) &
                             (df['params'] == ecc_param)]
                if ecc_row.empty:
                    continue

                ecc_val = ecc_row['median_us'].values[0]

                # Positive = ECC faster, Negative = RSA faster
                if ecc_val > 0 and rsa_val > 0:
                    ratio = rsa_val / ecc_val
                    entries.append({
                        'label': f'{op.capitalize()}\n({rsa_param} vs {ecc_param})',
                        'ratio': ratio,
                        'sec': sec_level,
                    })

    if not entries:
        return

    # Sort by ratio
    entries.sort(key=lambda e: e['ratio'], reverse=True)

    labels = [e['label'] for e in entries]
    ratios = [e['ratio'] for e in entries]
    colors = [ECC_COLOR if r >= 1 else RSA_COLOR for r in ratios]

    y = range(len(labels))
    bars = ax.barh(y, ratios, color=colors, edgecolor='white', height=0.6)

    # Reference line at ratio=1 (equal performance)
    ax.axvline(x=1, color='gray', linestyle='-', linewidth=1.5, alpha=0.7)

    # Annotate
    for bar, ratio in zip(bars, ratios):
        if ratio >= 1:
            text = f'{ratio:.1f}x ECC faster'
        else:
            text = f'{1/ratio:.1f}x RSA faster'
        ax.text(bar.get_width() + 0.1, bar.get_y() + bar.get_height()/2,
                text, ha='left', va='center', fontsize=9)

    ax.set_yticks(y)
    ax.set_yticklabels(labels, fontsize=9)
    ax.set_xlabel('Speedup Ratio (higher = ECC faster)')
    ax.set_xscale('log')
    ax.set_title('Performance Ratio: RSA vs ECC at Equivalent Security')

    plt.tight_layout()
    path = os.path.join(output_dir, 'chart_speedup_ratios.png')
    plt.savefig(path, bbox_inches='tight')
    plt.close()
    print(f'  Saved: {path}')


# ============================================================================
# CHART 6: Full Summary Table (all operations, all params)
# ============================================================================

def chart_summary_table(df, output_dir):
    """Overview heatmap-style table of all median times."""
    # Pivot: rows = params (RSA sizes + ECC curves), columns = operations
    all_ops = ['keygen', 'encrypt', 'decrypt_crt', 'sign', 'verify',
               'scalar_mult', 'ecdh']
    op_labels = {
        'keygen': 'KeyGen', 'encrypt': 'Encrypt', 'decrypt_crt': 'Decrypt\n(CRT)',
        'sign': 'Sign', 'verify': 'Verify', 'scalar_mult': 'Scalar\nMult',
        'ecdh': 'ECDH',
    }

    # Order: RSA sizes then ECC curves
    param_order = ['1024-bit', '2048-bit', '3072-bit', '4096-bit',
                   'secp256k1', 'P-256', 'P-384']

    present_params = [p for p in param_order if p in df['params'].unique()]
    present_ops = [op for op in all_ops if op in df['operation'].unique()]

    if not present_params or not present_ops:
        return

    # Build matrix
    matrix = np.full((len(present_params), len(present_ops)), np.nan)
    for i, param in enumerate(present_params):
        for j, op in enumerate(present_ops):
            row = df[(df['params'] == param) & (df['operation'] == op)]
            if not row.empty:
                matrix[i, j] = row['median_us'].values[0]

    fig, ax = plt.subplots(figsize=(12, 5))

    # Use log scale for colors
    log_matrix = np.where(np.isnan(matrix), np.nan, np.log10(np.maximum(matrix, 1)))

    im = ax.imshow(log_matrix, aspect='auto', cmap='YlOrRd',
                   interpolation='nearest')

    # Add text annotations
    for i in range(len(present_params)):
        for j in range(len(present_ops)):
            val = matrix[i, j]
            if not np.isnan(val):
                text = _format_time(val)
                color = 'white' if log_matrix[i, j] > 3.5 else 'black'
                ax.text(j, i, text, ha='center', va='center',
                        fontsize=9, fontweight='bold', color=color)
            else:
                ax.text(j, i, '-', ha='center', va='center',
                        fontsize=9, color='gray')

    ax.set_xticks(range(len(present_ops)))
    ax.set_xticklabels([op_labels.get(op, op) for op in present_ops], fontsize=10)
    ax.set_yticks(range(len(present_params)))
    ax.set_yticklabels(present_params, fontsize=10)

    # Separator between RSA and ECC
    rsa_count = sum(1 for p in present_params if 'bit' in p)
    if 0 < rsa_count < len(present_params):
        ax.axhline(y=rsa_count - 0.5, color='black', linewidth=2)

    ax.set_title('Benchmark Summary: Median Time per Operation (us)', fontsize=13)

    cbar = plt.colorbar(im, ax=ax, shrink=0.8)
    cbar.set_label('log10(us)')

    plt.tight_layout()
    path = os.path.join(output_dir, 'chart_summary_table.png')
    plt.savefig(path, bbox_inches='tight')
    plt.close()
    print(f'  Saved: {path}')


# ============================================================================
# CHART 7: Per-iteration Distribution (box plots from raw data)
# ============================================================================

def chart_distribution(df_raw, output_dir):
    """Box plots showing measurement distribution for key operations."""
    if df_raw is None or df_raw.empty:
        return

    # Focus on sign and verify at equivalent security
    for op in ['sign', 'verify']:
        op_data = df_raw[df_raw['operation'] == op]
        if op_data.empty:
            continue

        params_present = op_data['params'].unique()
        # Order: RSA first, then ECC
        rsa_params = sorted([p for p in params_present if 'bit' in p],
                            key=lambda x: int(x.replace('-bit', '')))
        ecc_params = [p for p in ['secp256k1', 'P-256', 'P-384'] if p in params_present]
        ordered_params = rsa_params + ecc_params

        if len(ordered_params) < 2:
            continue

        fig, ax = plt.subplots(figsize=(10, 5))

        data_groups = []
        labels = []
        colors = []
        for p in ordered_params:
            pdata = op_data[op_data['params'] == p]['time_us'].values
            if len(pdata) > 0:
                data_groups.append(pdata)
                labels.append(p)
                colors.append(PARAM_COLORS.get(p, '#888'))

        bp = ax.boxplot(data_groups, tick_labels=labels, patch_artist=True,
                        widths=0.6, showmeans=True,
                        meanprops=dict(marker='D', markerfacecolor='black', markersize=5))

        for patch, color in zip(bp['boxes'], colors):
            patch.set_facecolor(color)
            patch.set_alpha(0.7)

        ax.set_ylabel('Time (us)')
        ax.set_title(f'{op.capitalize()} Time Distribution')

        # Log scale if range is large
        all_vals = [v for g in data_groups for v in g]
        if all_vals and max(all_vals) / max(min(all_vals), 1) > 20:
            ax.set_yscale('log')
            ax.set_ylabel('Time (us) - log scale')

        plt.tight_layout()
        path = os.path.join(output_dir, f'chart_distribution_{op}.png')
        plt.savefig(path, bbox_inches='tight')
        plt.close()
        print(f'  Saved: {path}')


# ============================================================================
# HELPERS
# ============================================================================

def _format_time(us):
    """Format microseconds into a human-readable string."""
    if us < 1:
        return '<1us'
    elif us < 1000:
        return f'{us:.0f}us'
    elif us < 1_000_000:
        return f'{us/1000:.1f}ms'
    else:
        return f'{us/1_000_000:.2f}s'


# ============================================================================
# MAIN
# ============================================================================

def main():
    if len(sys.argv) < 3:
        print(f'Usage: {sys.argv[0]} <summary.csv> <raw.csv> [output_dir]')
        print(f'       {sys.argv[0]} results/summary_latest.csv results/raw_latest.csv results/')
        sys.exit(1)

    summary_path = sys.argv[1]
    raw_path = sys.argv[2]
    output_dir = sys.argv[3] if len(sys.argv) > 3 else 'results'

    os.makedirs(output_dir, exist_ok=True)

    print(f'Loading data from {summary_path}...')
    df, df_raw = load_data(summary_path, raw_path)
    print(f'  {len(df)} summary rows loaded.')
    if df_raw is not None:
        print(f'  {len(df_raw)} raw measurement rows loaded.')
    print()

    print('Generating charts:')
    chart_keygen_comparison(df, output_dir)
    chart_sign_verify_comparison(df, output_dir)
    chart_rsa_scaling(df, output_dir)
    chart_ecc_curves(df, output_dir)
    chart_speedup_ratios(df, output_dir)
    chart_summary_table(df, output_dir)
    chart_distribution(df_raw, output_dir)

    print()
    print('All charts generated successfully.')


if __name__ == '__main__':
    main()