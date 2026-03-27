#!/usr/bin/env python3
"""
visualize_benchmarks.py
Generates comparative charts from RSA vs ECC benchmark CSV data.
Supports: RSA, ECC (affine), ECC_JACOBIAN, ECC_BINARY
Author: Leon Elliott Fuller
Date: 2026-03-18

Usage:
    python3 visualize_benchmarks.py <summary.csv> <raw.csv> <output_dir>
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

# Algorithm family colors
RSA_COLOR     = '#2563EB'   # Blue
ECC_COLOR     = '#DC2626'   # Red (affine)
ECCJ_COLOR    = '#059669'   # Green (Jacobian)
BINARY_COLOR  = '#9333EA'   # Purple (binary field)

RSA_LIGHT     = '#93C5FD'
ECC_LIGHT     = '#FCA5A5'
ECCJ_LIGHT    = '#6EE7B7'
BINARY_LIGHT  = '#C4B5FD'

# Per-param colors for detailed charts
PARAM_COLORS = {
    # RSA
    '1024-bit': '#93C5FD',
    '2048-bit': '#60A5FA',
    '3072-bit': '#3B82F6',
    '4096-bit': '#1D4ED8',
    # ECC prime
    'secp256k1': '#F87171',
    'P-256':     '#EF4444',
    'P-384':     '#B91C1C',
    # Binary
    'sect163k1': '#C084FC',
    'sect233k1': '#A855F7',
    'sect283k1': '#7C3AED',
    'sect233r1': '#DDD6FE',
    'sect283r1': '#8B5CF6',
}

# Algorithm display labels
ALGO_LABELS = {
    'RSA':          'RSA',
    'ECC':          'ECC (Affine)',
    'ECC_JACOBIAN': 'ECC (Jacobian)',
    'ECC_BINARY':   'ECC (Binary)',
}

ALGO_COLORS = {
    'RSA':          RSA_COLOR,
    'ECC':          ECC_COLOR,
    'ECC_JACOBIAN': ECCJ_COLOR,
    'ECC_BINARY':   BINARY_COLOR,
}

# Security level groupings (NIST SP 800-57)
SECURITY_LEVELS = {
    128: {'RSA': '3072-bit', 'ECC': ['secp256k1', 'P-256']},
    192: {'RSA': '4096-bit', 'ECC': ['P-384']},
}

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
    df = pd.read_csv(summary_path)
    df_raw = pd.read_csv(raw_path) if raw_path and os.path.exists(raw_path) else None
    return df, df_raw


# ============================================================================
# CHART 1: Key Generation Comparison (all params, log scale)
# ============================================================================

def chart_keygen_comparison(df, output_dir):
    """Bar chart comparing key generation times across all algorithms."""
    keygen = df[df['operation'] == 'keygen'].copy()
    if keygen.empty:
        return

    fig, ax = plt.subplots(figsize=(14, 6))

    labels = []
    values = []
    colors = []
    edge_colors = []

    for _, row in keygen.iterrows():
        algo = row['algorithm']
        param = row['params']

        if algo == 'ECC_JACOBIAN':
            lbl = f"{param}\n(Jacobian)"
        elif algo == 'ECC_BINARY':
            lbl = f"{param}\n(Binary)"
        elif algo == 'ECC':
            lbl = f"{param}\n(Affine)"
        else:
            lbl = param

        labels.append(lbl)
        values.append(row['median_us'])
        colors.append(ALGO_COLORS.get(algo, '#888'))
        edge_colors.append('white')

    bars = ax.bar(range(len(labels)), values, color=colors, edgecolor=edge_colors,
                  linewidth=0.5, width=0.7)

    for bar, val in zip(bars, values):
        label = _format_time(val)
        ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() * 1.05,
                label, ha='center', va='bottom', fontsize=8, fontweight='bold')

    ax.set_xticks(range(len(labels)))
    ax.set_xticklabels(labels, rotation=0, fontsize=8)
    ax.set_ylabel('Time (us) - log scale')
    ax.set_yscale('log')
    ax.set_title('Key Generation Time: All Algorithms & Coordinate Systems')

    # Legend
    from matplotlib.patches import Patch
    legend_elements = [Patch(facecolor=c, label=l) for l, c in
                       [('RSA', RSA_COLOR), ('ECC Affine', ECC_COLOR),
                        ('ECC Jacobian', ECCJ_COLOR), ('ECC Binary', BINARY_COLOR)]
                       if any(df['algorithm'] == {'RSA': 'RSA', 'ECC Affine': 'ECC',
                              'ECC Jacobian': 'ECC_JACOBIAN',
                              'ECC Binary': 'ECC_BINARY'}.get(l, ''))]
    if legend_elements:
        ax.legend(handles=legend_elements, loc='upper right')

    plt.tight_layout()
    path = os.path.join(output_dir, 'chart_keygen_comparison.png')
    plt.savefig(path, bbox_inches='tight')
    plt.close()
    print(f'  Saved: {path}')


# ============================================================================
# CHART 2: Sign & Verify at Equivalent Security Levels
# ============================================================================

def chart_sign_verify_comparison(df, output_dir):
    """Grouped bar chart comparing sign and verify at equivalent security."""
    fig, axes = plt.subplots(1, 2, figsize=(14, 6))

    for idx, op in enumerate(['sign', 'verify']):
        ax = axes[idx]
        op_data = df[df['operation'] == op]
        if op_data.empty:
            continue

        groups = []
        rsa_vals = []
        ecc_vals = []

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

        ax.bar(x - width/2, rsa_vals, width, label='RSA', color=RSA_COLOR, edgecolor='white')
        ax.bar(x + width/2, ecc_vals, width, label='ECC (Affine)', color=ECC_COLOR, edgecolor='white')

        # Also overlay Jacobian if available
        eccj_vals = []
        for sec_level, mapping in sorted(SECURITY_LEVELS.items()):
            for ecc_param in mapping['ECC']:
                eccj_row = op_data[(op_data['algorithm'] == 'ECC_JACOBIAN') &
                                   (op_data['params'] == ecc_param)]
                if not eccj_row.empty:
                    eccj_vals.append(eccj_row['median_us'].values[0])
                else:
                    eccj_vals.append(0)

        if any(v > 0 for v in eccj_vals):
            ax.bar(x + width/2 + width, eccj_vals, width, label='ECC (Jacobian)',
                   color=ECCJ_COLOR, edgecolor='white')

        ax.set_xticks(x)
        ax.set_xticklabels(groups, fontsize=9)
        ax.set_ylabel('Time (us)')
        ax.set_title(f'{op.capitalize()} Performance')
        ax.legend(fontsize=8)

        all_vals = rsa_vals + ecc_vals + [v for v in eccj_vals if v > 0]
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
    rsa = df[df['algorithm'] == 'RSA'].copy()
    if rsa.empty:
        return

    rsa['key_bits'] = rsa['params'].str.replace('-bit', '').astype(int)
    rsa = rsa.sort_values('key_bits')

    ops_to_plot = ['keygen', 'encrypt', 'decrypt_crt', 'sign', 'verify']
    op_labels = {
        'keygen': 'Key Generation', 'encrypt': 'Encrypt',
        'decrypt_crt': 'Decrypt (CRT)', 'sign': 'Sign (CRT)', 'verify': 'Verify',
    }
    op_styles = {
        'keygen': ('o-', '#1D4ED8'), 'encrypt': ('s--', '#059669'),
        'decrypt_crt': ('^--', '#7C3AED'), 'sign': ('D-', '#EA580C'),
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
# CHART 4: ECC Operations Across Prime Curves (Affine only)
# ============================================================================

def chart_ecc_curves(df, output_dir):
    ecc = df[df['algorithm'] == 'ECC'].copy()
    if ecc.empty:
        return

    ops = ['keygen', 'scalar_mult', 'ecdh', 'sign', 'verify']
    op_labels = {
        'keygen': 'KeyGen', 'scalar_mult': 'Scalar Mult', 'ecdh': 'ECDH',
        'sign': 'ECDSA Sign', 'verify': 'ECDSA Verify',
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
    ax.set_title('ECC Operations by Curve (Affine Coordinates)')
    ax.legend()

    plt.tight_layout()
    path = os.path.join(output_dir, 'chart_ecc_curves.png')
    plt.savefig(path, bbox_inches='tight')
    plt.close()
    print(f'  Saved: {path}')


# ============================================================================
# CHART 5: Speedup Ratios (RSA vs ECC Affine)
# ============================================================================

def chart_speedup_ratios(df, output_dir):
    fig, ax = plt.subplots(figsize=(10, 6))

    entries = []
    for sec_level, mapping in sorted(SECURITY_LEVELS.items()):
        rsa_param = mapping['RSA']
        for op in COMMON_OPS:
            rsa_row = df[(df['algorithm'] == 'RSA') & (df['operation'] == op) &
                         (df['params'] == rsa_param)]
            if rsa_row.empty:
                continue
            rsa_val = rsa_row['median_us'].values[0]

            for ecc_param in mapping['ECC']:
                ecc_row = df[(df['algorithm'] == 'ECC') & (df['operation'] == op) &
                             (df['params'] == ecc_param)]
                if ecc_row.empty:
                    continue
                ecc_val = ecc_row['median_us'].values[0]
                if ecc_val > 0 and rsa_val > 0:
                    entries.append({
                        'label': f'{op.capitalize()}\n({rsa_param} vs {ecc_param})',
                        'ratio': rsa_val / ecc_val,
                    })

    if not entries:
        return

    entries.sort(key=lambda e: e['ratio'], reverse=True)

    labels = [e['label'] for e in entries]
    ratios = [e['ratio'] for e in entries]
    colors = [ECC_COLOR if r >= 1 else RSA_COLOR for r in ratios]

    y = range(len(labels))
    bars = ax.barh(y, ratios, color=colors, edgecolor='white', height=0.6)

    ax.axvline(x=1, color='gray', linestyle='-', linewidth=1.5, alpha=0.7)

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
# CHART 6: Full Summary Table (all algorithms)
# ============================================================================

def chart_summary_table(df, output_dir):
    all_ops = ['keygen', 'encrypt', 'decrypt_crt', 'sign', 'verify',
               'scalar_mult', 'ecdh']
    op_labels = {
        'keygen': 'KeyGen', 'encrypt': 'Encrypt', 'decrypt_crt': 'Decrypt\n(CRT)',
        'sign': 'Sign', 'verify': 'Verify', 'scalar_mult': 'Scalar\nMult',
        'ecdh': 'ECDH',
    }

    # Build row list: (display_label, algorithm, params)
    rows_def = []

    # RSA rows
    for p in ['1024-bit', '2048-bit', '3072-bit', '4096-bit']:
        if p in df[df['algorithm'] == 'RSA']['params'].values:
            rows_def.append((f'RSA {p}', 'RSA', p))

    # ECC affine
    for p in ['secp256k1', 'P-256', 'P-384']:
        if p in df[df['algorithm'] == 'ECC']['params'].values:
            rows_def.append((f'ECC {p} (Affine)', 'ECC', p))

    # ECC Jacobian
    for p in ['secp256k1', 'P-256', 'P-384']:
        if p in df[df['algorithm'] == 'ECC_JACOBIAN']['params'].values:
            rows_def.append((f'ECC {p} (Jacobian)', 'ECC_JACOBIAN', p))

    # ECC binary
    for p in ['sect163k1', 'sect233k1', 'sect283k1', 'sect233r1', 'sect283r1']:
        if p in df[df['algorithm'] == 'ECC_BINARY']['params'].values:
            rows_def.append((f'Binary {p}', 'ECC_BINARY', p))

    present_ops = [op for op in all_ops if op in df['operation'].unique()]

    if not rows_def or not present_ops:
        return

    row_labels = [r[0] for r in rows_def]
    matrix = np.full((len(rows_def), len(present_ops)), np.nan)

    for i, (_, algo, param) in enumerate(rows_def):
        for j, op in enumerate(present_ops):
            row = df[(df['algorithm'] == algo) & (df['params'] == param) &
                     (df['operation'] == op)]
            if not row.empty:
                matrix[i, j] = row['median_us'].values[0]

    fig, ax = plt.subplots(figsize=(14, max(6, len(rows_def) * 0.45)))

    log_matrix = np.where(np.isnan(matrix), np.nan, np.log10(np.maximum(matrix, 1)))

    im = ax.imshow(log_matrix, aspect='auto', cmap='YlOrRd', interpolation='nearest')

    for i in range(len(rows_def)):
        for j in range(len(present_ops)):
            val = matrix[i, j]
            if not np.isnan(val):
                text = _format_time(val)
                color = 'white' if log_matrix[i, j] > 3.5 else 'black'
                ax.text(j, i, text, ha='center', va='center',
                        fontsize=8, fontweight='bold', color=color)
            else:
                ax.text(j, i, '-', ha='center', va='center', fontsize=8, color='gray')

    ax.set_xticks(range(len(present_ops)))
    ax.set_xticklabels([op_labels.get(op, op) for op in present_ops], fontsize=10)
    ax.set_yticks(range(len(row_labels)))
    ax.set_yticklabels(row_labels, fontsize=9)

    # Separators between groups
    rsa_end = sum(1 for _, a, _ in rows_def if a == 'RSA')
    ecc_end = rsa_end + sum(1 for _, a, _ in rows_def if a == 'ECC')
    eccj_end = ecc_end + sum(1 for _, a, _ in rows_def if a == 'ECC_JACOBIAN')

    for sep in [rsa_end, ecc_end, eccj_end]:
        if 0 < sep < len(rows_def):
            ax.axhline(y=sep - 0.5, color='black', linewidth=2)

    ax.set_title('Benchmark Summary: Median Time per Operation (us)', fontsize=13)
    cbar = plt.colorbar(im, ax=ax, shrink=0.8)
    cbar.set_label('log10(us)')

    plt.tight_layout()
    path = os.path.join(output_dir, 'chart_summary_table.png')
    plt.savefig(path, bbox_inches='tight')
    plt.close()
    print(f'  Saved: {path}')


# ============================================================================
# CHART 7: Distribution (box plots from raw data)
# ============================================================================

def chart_distribution(df_raw, output_dir):
    if df_raw is None or df_raw.empty:
        return

    for op in ['sign', 'verify']:
        op_data = df_raw[df_raw['operation'] == op]
        if op_data.empty:
            continue

        params_present = op_data['params'].unique()
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
            # Affine data
            pdata = op_data[(op_data['params'] == p) &
                            (op_data['algorithm'].isin(['RSA', 'ECC']))]['time_us'].values
            if len(pdata) > 0:
                algo = 'RSA' if 'bit' in p else 'ECC (Affine)'
                data_groups.append(pdata)
                labels.append(f'{p}\n{algo}')
                colors.append(PARAM_COLORS.get(p, '#888'))

            # Jacobian data (only for ECC params)
            if 'bit' not in p:
                jdata = op_data[(op_data['params'] == p) &
                                (op_data['algorithm'] == 'ECC_JACOBIAN')]['time_us'].values
                if len(jdata) > 0:
                    data_groups.append(jdata)
                    labels.append(f'{p}\n(Jacobian)')
                    colors.append(ECCJ_COLOR)

        if len(data_groups) < 2:
            continue

        bp = ax.boxplot(data_groups, tick_labels=labels, patch_artist=True,
                        widths=0.6, showmeans=True,
                        meanprops=dict(marker='D', markerfacecolor='black', markersize=5))

        for patch, color in zip(bp['boxes'], colors):
            patch.set_facecolor(color)
            patch.set_alpha(0.7)

        ax.set_ylabel('Time (us)')
        ax.set_title(f'{op.capitalize()} Time Distribution (incl. Jacobian)')
        ax.tick_params(axis='x', labelsize=8)

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
# CHART 8: Affine vs Jacobian Speedup (NEW)
# ============================================================================

def chart_affine_vs_jacobian(df, output_dir):
    """Grouped bar chart showing affine vs Jacobian times per curve per operation."""
    affine = df[df['algorithm'] == 'ECC'].copy()
    jacobian = df[df['algorithm'] == 'ECC_JACOBIAN'].copy()

    if affine.empty or jacobian.empty:
        return

    ops = ['keygen', 'scalar_mult', 'ecdh', 'sign', 'verify']
    op_labels = {
        'keygen': 'KeyGen', 'scalar_mult': 'Scalar\nMult', 'ecdh': 'ECDH',
        'sign': 'Sign', 'verify': 'Verify',
    }

    curves = sorted(set(affine['params'].unique()) & set(jacobian['params'].unique()))
    if not curves:
        return

    fig, axes = plt.subplots(1, len(curves), figsize=(6 * len(curves), 6), sharey=False)
    if len(curves) == 1:
        axes = [axes]

    for ax, curve in zip(axes, curves):
        aff_data = affine[affine['params'] == curve]
        jac_data = jacobian[jacobian['params'] == curve]

        present_ops = []
        aff_vals = []
        jac_vals = []

        for op in ops:
            a_row = aff_data[aff_data['operation'] == op]
            j_row = jac_data[jac_data['operation'] == op]
            if not a_row.empty and not j_row.empty:
                present_ops.append(op)
                aff_vals.append(a_row['median_us'].values[0])
                jac_vals.append(j_row['median_us'].values[0])

        if not present_ops:
            continue

        x = np.arange(len(present_ops))
        width = 0.35

        bars_a = ax.bar(x - width/2, aff_vals, width, label='Affine',
                        color=ECC_COLOR, edgecolor='white')
        bars_j = ax.bar(x + width/2, jac_vals, width, label='Jacobian',
                        color=ECCJ_COLOR, edgecolor='white')

        # Annotate speedup
        for i, (a, j) in enumerate(zip(aff_vals, jac_vals)):
            if j > 0:
                speedup = a / j
                ax.text(i, max(a, j) * 1.15, f'{speedup:.1f}x',
                        ha='center', va='bottom', fontsize=9, fontweight='bold',
                        color='#374151')

        ax.set_xticks(x)
        ax.set_xticklabels([op_labels.get(op, op) for op in present_ops], fontsize=9)
        ax.set_ylabel('Time (us)')
        ax.set_title(f'{curve}')
        ax.legend(fontsize=9)

    plt.suptitle('Affine vs Jacobian Coordinates: Per-Operation Speedup', fontsize=14)
    plt.tight_layout()
    path = os.path.join(output_dir, 'chart_affine_vs_jacobian.png')
    plt.savefig(path, bbox_inches='tight')
    plt.close()
    print(f'  Saved: {path}')


# ============================================================================
# CHART 9: Binary Field Curves (NEW)
# ============================================================================

def chart_binary_curves(df, output_dir):
    """Grouped bar chart: binary field ECC operations across all curves."""
    binary = df[df['algorithm'] == 'ECC_BINARY'].copy()
    if binary.empty:
        return

    ops = ['keygen', 'scalar_mult', 'ecdh']
    op_labels = {'keygen': 'KeyGen', 'scalar_mult': 'Scalar Mult', 'ecdh': 'ECDH'}

    curves = binary['params'].unique().tolist()
    curve_colors = [PARAM_COLORS.get(c, '#888') for c in curves]

    fig, ax = plt.subplots(figsize=(12, 6))

    x = np.arange(len(ops))
    n_curves = len(curves)
    total_width = 0.8
    bar_width = total_width / n_curves

    for i, (curve, color) in enumerate(zip(curves, curve_colors)):
        curve_data = binary[binary['params'] == curve]
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
    ax.set_title('Binary Field ECC: Operations by Curve (GF(2^m))')
    ax.legend(title='Curve', fontsize=9)

    plt.tight_layout()
    path = os.path.join(output_dir, 'chart_binary_curves.png')
    plt.savefig(path, bbox_inches='tight')
    plt.close()
    print(f'  Saved: {path}')


# ============================================================================
# CHART 10: Prime vs Binary Field Comparison (NEW)
# ============================================================================

def chart_prime_vs_binary(df, output_dir):
    """Compare prime field (affine + Jacobian) vs binary field at same security."""
    # Security 128 bits: P-256 vs sect283k1
    # Security 112 bits: (no prime at 112, but we can compare sect233k1/r1)
    comparisons = [
        ('~128-bit security', [
            ('ECC', 'P-256', 'P-256 (Affine)'),
            ('ECC_JACOBIAN', 'P-256', 'P-256 (Jacobian)'),
            ('ECC_BINARY', 'sect283k1', 'sect283k1 (Binary)'),
        ]),
        ('~112-bit security', [
            ('ECC_BINARY', 'sect233k1', 'sect233k1 (Koblitz)'),
            ('ECC_BINARY', 'sect233r1', 'sect233r1 (Random)'),
        ]),
    ]

    ops = ['keygen', 'scalar_mult', 'ecdh']
    op_labels = {'keygen': 'KeyGen', 'scalar_mult': 'Scalar Mult', 'ecdh': 'ECDH'}
    group_colors = {
        'ECC': ECC_COLOR, 'ECC_JACOBIAN': ECCJ_COLOR, 'ECC_BINARY': BINARY_COLOR,
    }

    fig, axes = plt.subplots(1, len(comparisons), figsize=(7 * len(comparisons), 6))
    if len(comparisons) == 1:
        axes = [axes]

    for ax, (title, entries) in zip(axes, comparisons):
        x = np.arange(len(ops))
        n = len(entries)
        total_width = 0.8
        bar_width = total_width / max(n, 1)

        for i, (algo, param, label) in enumerate(entries):
            data = df[(df['algorithm'] == algo) & (df['params'] == param)]
            if data.empty:
                continue

            vals = []
            for op in ops:
                row = data[data['operation'] == op]
                vals.append(row['median_us'].values[0] if not row.empty else 0)

            offset = (i - n/2 + 0.5) * bar_width
            color = group_colors.get(algo, '#888')
            bars = ax.bar(x + offset, vals, bar_width, label=label,
                          color=color, edgecolor='white', alpha=0.85)

            for bar, val in zip(bars, vals):
                if val > 0:
                    ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() * 1.02,
                            _format_time(val), ha='center', va='bottom', fontsize=7)

        ax.set_xticks(x)
        ax.set_xticklabels([op_labels.get(op, op) for op in ops])
        ax.set_ylabel('Time (us)')
        ax.set_title(title)
        ax.legend(fontsize=8)

    plt.suptitle('Field Arithmetic Comparison: Fp (Prime) vs GF(2^m) (Binary)', fontsize=14)
    plt.tight_layout()
    path = os.path.join(output_dir, 'chart_prime_vs_binary.png')
    plt.savefig(path, bbox_inches='tight')
    plt.close()
    print(f'  Saved: {path}')


# ============================================================================
# CHART 11: Jacobian Speedup Summary (NEW)
# ============================================================================

def chart_jacobian_speedup_summary(df, output_dir):
    """Horizontal bar chart showing Jacobian speedup factor per curve per operation."""
    affine = df[df['algorithm'] == 'ECC']
    jacobian = df[df['algorithm'] == 'ECC_JACOBIAN']

    if affine.empty or jacobian.empty:
        return

    entries = []
    for _, a_row in affine.iterrows():
        j_row = jacobian[(jacobian['params'] == a_row['params']) &
                         (jacobian['operation'] == a_row['operation'])]
        if j_row.empty:
            continue
        a_val = a_row['median_us']
        j_val = j_row['median_us'].values[0]
        if j_val > 0 and a_val > 0:
            entries.append({
                'label': f"{a_row['operation'].capitalize()} ({a_row['params']})",
                'speedup': a_val / j_val,
                'curve': a_row['params'],
            })

    if not entries:
        return

    entries.sort(key=lambda e: e['speedup'], reverse=True)

    fig, ax = plt.subplots(figsize=(10, max(4, len(entries) * 0.4)))

    labels = [e['label'] for e in entries]
    speedups = [e['speedup'] for e in entries]
    colors = [PARAM_COLORS.get(e['curve'], ECCJ_COLOR) for e in entries]

    y = range(len(labels))
    bars = ax.barh(y, speedups, color=colors, edgecolor='white', height=0.6, alpha=0.8)

    ax.axvline(x=1, color='gray', linestyle='-', linewidth=1.5, alpha=0.7)

    for bar, spd in zip(bars, speedups):
        ax.text(bar.get_width() + 0.05, bar.get_y() + bar.get_height()/2,
                f'{spd:.2f}x', ha='left', va='center', fontsize=9, fontweight='bold')

    ax.set_yticks(y)
    ax.set_yticklabels(labels, fontsize=9)
    ax.set_xlabel('Speedup Factor (Affine time / Jacobian time)')
    ax.set_title('Jacobian Coordinate Speedup over Affine')

    plt.tight_layout()
    path = os.path.join(output_dir, 'chart_jacobian_speedup.png')
    plt.savefig(path, bbox_inches='tight')
    plt.close()
    print(f'  Saved: {path}')


# ============================================================================
# HELPERS
# ============================================================================

def _format_time(us):
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

    # Show algorithm breakdown
    for algo in df['algorithm'].unique():
        count = len(df[df['algorithm'] == algo])
        print(f'  {ALGO_LABELS.get(algo, algo)}: {count} benchmarks')
    print()

    print('Generating charts:')

    # Original charts (updated for new algo types)
    chart_keygen_comparison(df, output_dir)
    chart_sign_verify_comparison(df, output_dir)
    chart_rsa_scaling(df, output_dir)
    chart_ecc_curves(df, output_dir)
    chart_speedup_ratios(df, output_dir)
    chart_summary_table(df, output_dir)
    chart_distribution(df_raw, output_dir)

    # New charts for Jacobian and binary fields
    chart_affine_vs_jacobian(df, output_dir)
    chart_binary_curves(df, output_dir)
    chart_prime_vs_binary(df, output_dir)
    chart_jacobian_speedup_summary(df, output_dir)

    print()
    print('All charts generated successfully.')


if __name__ == '__main__':
    main()