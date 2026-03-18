#!/usr/bin/env python3
"""
create_collages.py
Combines individual benchmark charts into 2 collage images for README.
Author: Leon Elliott Fuller
Date: 2026-02-28

Usage:
    python3 create_collages.py <charts_dir> <output_dir>
"""

import sys
import os
from PIL import Image

def create_collage(images, output_path, cols=2, padding=20, bg_color=(255, 255, 255)):
    """Create a grid collage from a list of PIL Images."""
    if not images:
        return

    rows = (len(images) + cols - 1) // cols

    # Determine uniform cell size (scale all to same width)
    target_width = max(img.width for img in images)

    # Scale images to uniform width, preserving aspect ratio
    scaled = []
    for img in images:
        ratio = target_width / img.width
        new_h = int(img.height * ratio)
        scaled.append(img.resize((target_width, new_h), Image.LANCZOS))

    # Compute max height per row
    row_heights = []
    for r in range(rows):
        row_imgs = scaled[r * cols : (r + 1) * cols]
        row_heights.append(max(img.height for img in row_imgs))

    total_w = cols * target_width + (cols + 1) * padding
    total_h = sum(row_heights) + (rows + 1) * padding

    canvas = Image.new('RGB', (total_w, total_h), bg_color)

    y = padding
    for r in range(rows):
        x = padding
        row_imgs = scaled[r * cols : (r + 1) * cols]
        for img in row_imgs:
            # Center vertically in row
            y_offset = (row_heights[r] - img.height) // 2
            canvas.paste(img, (x, y + y_offset))
            x += target_width + padding
        y += row_heights[r] + padding

    canvas.save(output_path, quality=95, optimize=True)
    print(f'  Saved collage: {output_path} ({total_w}x{total_h})')


def main():
    charts_dir = sys.argv[1] if len(sys.argv) > 1 else 'results'
    output_dir = sys.argv[2] if len(sys.argv) > 2 else charts_dir

    os.makedirs(output_dir, exist_ok=True)

    def load(name):
        path = os.path.join(charts_dir, name)
        if os.path.exists(path):
            return Image.open(path)
        print(f'  WARNING: {path} not found, skipping.')
        return None

    # ================================================================
    # Collage 1: RSA vs ECC Comparative Overview
    #   Top row:    Summary table (full width)
    #   Middle row: Keygen comparison + Speedup ratios
    #   Bottom row: Sign/Verify comparison (full width)
    # ================================================================
    print('Creating collage 1: Comparative Overview...')

    summary   = load('chart_summary_table.png')
    keygen    = load('chart_keygen_comparison.png')
    speedup   = load('chart_speedup_ratios.png')
    signverif = load('chart_sign_verify_comparison.png')

    imgs_1 = [img for img in [summary, keygen, speedup, signverif] if img is not None]
    if imgs_1:
        create_collage(imgs_1, os.path.join(output_dir, 'collage_comparison.png'),
                        cols=2, padding=15)

    # ================================================================
    # Collage 2: Detailed Analysis
    #   Top row:    RSA scaling + ECC curves
    #   Bottom row: Distribution sign + Distribution verify
    # ================================================================
    print('Creating collage 2: Detailed Analysis...')

    rsa_scale = load('chart_rsa_scaling.png')
    ecc_curve = load('chart_ecc_curves.png')
    dist_sign = load('chart_distribution_sign.png')
    dist_ver  = load('chart_distribution_verify.png')

    imgs_2 = [img for img in [rsa_scale, ecc_curve, dist_sign, dist_ver] if img is not None]
    if imgs_2:
        create_collage(imgs_2, os.path.join(output_dir, 'collage_detailed.png'),
                        cols=2, padding=15)

    print('Done.')


if __name__ == '__main__':
    main()