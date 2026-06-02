#!/usr/bin/env python3
"""
visualize_compiler_flags.py
Genera una grafica de barras con el rendimiento de ECC (secp256k1) segun el
conjunto de opciones de compilacion, a partir de results/compiler_flags.csv.

Autor: Leon Elliott Fuller
Date: 2026-05-29

Uso:
    python3 visualize_compiler_flags.py [csv_file] [output_dir]

Por defecto:
    csv_file   = results/compiler_flags.csv
    output_dir = results

El CSV de entrada debe tener la cabecera:
    flags,scalar_mult_us,ecdh_us
tal y como lo genera scripts/compare_compiler_flags.sh
"""

import sys
import os
import csv

import matplotlib
matplotlib.use("Agg")  # backend sin ventana, para entornos headless / Docker
import matplotlib.pyplot as plt
import numpy as np

# Paleta coherente con el resto del proyecto (azul / rojo)
COLOR_SM = "#1f77b4"   # scalar_mult
COLOR_DH = "#d62728"   # ecdh


def short_label(flags):
    """Devuelve una etiqueta corta y legible para un conjunto de flags."""
    f = flags.strip()
    mapping = {
        "-O0": "-O0",
        "-O1": "-O1",
        "-O2": "-O2",
        "-O3": "-O3",
        "-Ofast": "-Ofast",
        "-O2 -march=native": "-O2\n-march=native",
    }
    return mapping.get(f, f.replace(" ", "\n"))


def load_data(csv_file):
    """Lee el CSV y devuelve listas paralelas (labels, scalar_mult, ecdh)."""
    labels, sm, dh = [], [], []
    with open(csv_file, newline="") as fh:
        reader = csv.DictReader(fh)
        for row in reader:
            labels.append(short_label(row["flags"]))
            # Tolera celdas vacias (sin medir) convirtiendo a NaN
            sm.append(_to_float(row.get("scalar_mult_us")))
            dh.append(_to_float(row.get("ecdh_us")))
    return labels, sm, dh


def _to_float(value):
    try:
        return float(value)
    except (TypeError, ValueError):
        return float("nan")


def main():
    csv_file = sys.argv[1] if len(sys.argv) > 1 else "results/compiler_flags.csv"
    output_dir = sys.argv[2] if len(sys.argv) > 2 else "results"

    if not os.path.isfile(csv_file):
        print(f"  ERROR: no se encuentra {csv_file}")
        print("  Ejecuta primero: bash scripts/compare_compiler_flags.sh")
        sys.exit(1)

    labels, sm, dh = load_data(csv_file)
    if not labels:
        print(f"  ERROR: {csv_file} no contiene datos")
        sys.exit(1)

    os.makedirs(output_dir, exist_ok=True)

    n = len(labels)
    x = np.arange(n)
    width = 0.38

    fig, ax = plt.subplots(figsize=(11, 6.5), dpi=200)

    bars_sm = ax.bar(x - width / 2, sm, width, label="Multiplicacion escalar",
                     color=COLOR_SM, edgecolor="black", linewidth=0.6)
    bars_dh = ax.bar(x + width / 2, dh, width, label="ECDH",
                     color=COLOR_DH, edgecolor="black", linewidth=0.6)

    # Etiqueta numerica sobre cada barra
    ax.bar_label(bars_sm, fmt="%.0f", padding=3, fontsize=10)
    ax.bar_label(bars_dh, fmt="%.0f", padding=3, fontsize=10)

    # Aceleracion frente a -O0 (si hay dato de -O0 valido) sobre scalar_mult
    baseline = None
    for lbl, val in zip(labels, sm):
        if lbl.startswith("-O0") and not np.isnan(val):
            baseline = val
            break
    if baseline:
        top = np.nanmax([v for v in (sm + dh) if not np.isnan(v)])
        for xi, val in zip(x, sm):
            if not np.isnan(val) and val > 0:
                speedup = baseline / val
                ax.text(xi - width / 2, val + top * 0.06,
                        f"{speedup:.2f}x", ha="center", va="bottom",
                        fontsize=9, color=COLOR_SM, fontweight="bold")

    ax.set_xticks(x)
    ax.set_xticklabels(labels, fontsize=11)
    ax.set_ylabel("Tiempo por operacion (us)", fontsize=12)
    ax.set_title("ECC (secp256k1): rendimiento segun opciones de compilacion",
                 fontsize=14, fontweight="bold")
    ax.legend(fontsize=11, loc="upper right")
    ax.grid(axis="y", linestyle="--", alpha=0.4)
    ax.margins(y=0.18)

    fig.tight_layout()
    out_path = os.path.join(output_dir, "chart_compiler_flags.png")
    fig.savefig(out_path, bbox_inches="tight")
    plt.close(fig)
    print(f"  Saved: {out_path}")


if __name__ == "__main__":
    main()