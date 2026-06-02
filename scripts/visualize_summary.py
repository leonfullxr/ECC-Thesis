#!/usr/bin/env python3
"""
visualize_summary.py
Genera un mapa de calor resumen, legible y de alta resolucion, con la mediana
de todas las operaciones medidas, agrupadas por familia de algoritmo.

Sustituye a la version reducida de chart_summary_table.png por una mas grande
y con mas detalle (fuentes mayores, mayor DPI, separadores por grupo).

Autor: Leon Elliott Fuller
Date: 2026-05-29

Uso:
    python3 visualize_summary.py [summary_csv] [output_dir]

Por defecto:
    summary_csv = results/summary_latest.csv
    output_dir  = results
"""

import sys
import os
import csv
import math

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

# Orden de columnas (operaciones). Solo se muestran las que tengan datos.
COLUMN_ORDER = ["keygen", "encrypt", "decrypt", "decrypt_crt",
                "sign", "verify", "scalar_mult", "ecdh"]
COLUMN_LABELS = {
    "keygen": "Keygen",
    "encrypt": "Encrypt",
    "decrypt": "Decrypt",
    "decrypt_crt": "Decrypt\nCRT",
    "sign": "Sign",
    "verify": "Verify",
    "scalar_mult": "Scalar\nmult",
    "ecdh": "ECDH",
}

# Orden y etiqueta de familias de algoritmo
FAMILY_ORDER = ["RSA", "ECC", "ECC_JACOBIAN", "ECC_BINARY"]
FAMILY_LABEL = {
    "RSA": "RSA",
    "ECC": "ECC afin",
    "ECC_JACOBIAN": "ECC Jacob.",
    "ECC_BINARY": "ECC binario",
}
# Orden preferente de parametros dentro de cada familia
PARAM_ORDER = {
    "1024-bit": 0, "2048-bit": 1, "3072-bit": 2, "4096-bit": 3,
    "secp256k1": 0, "P-256": 1, "P-384": 2,
    "sect163k1": 0, "sect233k1": 1, "sect233r1": 2, "sect283k1": 3, "sect283r1": 4,
}


def load_rows(csv_file):
    rows = []
    with open(csv_file, newline="") as fh:
        for row in csv.DictReader(fh):
            op = row["operation"]
            if op == "sha256_hash":
                continue  # se mide como 0 us, no aporta al resumen
            try:
                median = float(row["median_us"])
            except (TypeError, ValueError):
                continue
            rows.append((row["algorithm"], row["params"], op, median))
    return rows


def main():
    csv_file = sys.argv[1] if len(sys.argv) > 1 else "results/summary_latest.csv"
    output_dir = sys.argv[2] if len(sys.argv) > 2 else "results"

    if not os.path.isfile(csv_file):
        print(f"  ERROR: no se encuentra {csv_file}")
        sys.exit(1)

    rows = load_rows(csv_file)
    if not rows:
        print(f"  ERROR: {csv_file} no contiene datos utiles")
        sys.exit(1)

    os.makedirs(output_dir, exist_ok=True)

    # Construir el conjunto de filas (familia, params) en orden
    row_keys = []
    seen = set()
    for algo, params, _op, _m in rows:
        key = (algo, params)
        if key not in seen:
            seen.add(key)
            row_keys.append(key)

    def row_sort_key(k):
        algo, params = k
        fam_idx = FAMILY_ORDER.index(algo) if algo in FAMILY_ORDER else len(FAMILY_ORDER)
        par_idx = PARAM_ORDER.get(params, 99)
        return (fam_idx, par_idx, params)

    row_keys.sort(key=row_sort_key)

    # Columnas presentes en los datos, respetando COLUMN_ORDER
    present_ops = {op for _a, _p, op, _m in rows}
    columns = [c for c in COLUMN_ORDER if c in present_ops]

    # Matriz de valores
    index = {k: i for i, k in enumerate(row_keys)}
    colidx = {c: j for j, c in enumerate(columns)}
    data = np.full((len(row_keys), len(columns)), np.nan)
    for algo, params, op, median in rows:
        if op in colidx:
            data[index[(algo, params)], colidx[op]] = median

    # Color en escala logaritmica
    with np.errstate(invalid="ignore"):
        log_data = np.log10(data)

    finite = log_data[np.isfinite(log_data)]
    vmin, vmax = (finite.min(), finite.max()) if finite.size else (0, 1)

    # Tamano generoso y dpi alto para legibilidad
    n_rows, n_cols = len(row_keys), len(columns)
    fig_w = max(8.0, 1.5 * n_cols + 3.0)
    fig_h = max(6.0, 0.62 * n_rows + 2.0)
    fig, ax = plt.subplots(figsize=(fig_w, fig_h), dpi=200)

    cmap = plt.get_cmap("YlOrRd").copy()
    cmap.set_bad(color="#f0f0f0")  # celdas sin dato en gris claro
    masked = np.ma.masked_invalid(log_data)
    im = ax.imshow(masked, cmap=cmap, aspect="auto", vmin=vmin, vmax=vmax)

    # Ejes
    ax.set_xticks(np.arange(n_cols))
    ax.set_xticklabels([COLUMN_LABELS.get(c, c) for c in columns], fontsize=12)
    ax.set_yticks(np.arange(n_rows))
    ax.set_yticklabels([f"{FAMILY_LABEL.get(a, a)}  {p}" for (a, p) in row_keys],
                       fontsize=11)
    ax.tick_params(top=True, bottom=False, labeltop=True, labelbottom=False)

    # Anotacion de cada celda con el valor de la mediana (us)
    for i in range(n_rows):
        for j in range(n_cols):
            val = data[i, j]
            if not np.isnan(val):
                # Texto en blanco sobre celdas oscuras, negro sobre claras
                norm = (log_data[i, j] - vmin) / (vmax - vmin) if vmax > vmin else 0.0
                color = "white" if norm > 0.6 else "black"
                txt = f"{val:,.0f}".replace(",", " ")
                ax.text(j, i, txt, ha="center", va="center",
                        fontsize=10, color=color)

    # Separadores horizontales entre familias
    last_family = None
    for i, (algo, _p) in enumerate(row_keys):
        if last_family is not None and algo != last_family:
            ax.axhline(i - 0.5, color="black", linewidth=1.5)
        last_family = algo

    ax.set_title("Resumen de tiempos (mediana, us) por algoritmo y operacion",
                 fontsize=15, fontweight="bold", pad=28)

    cbar = fig.colorbar(im, ax=ax, fraction=0.046, pad=0.04)
    cbar.set_label("log10(mediana en us)", fontsize=11)

    fig.tight_layout()
    out_path = os.path.join(output_dir, "chart_summary_table.png")
    fig.savefig(out_path, bbox_inches="tight")
    plt.close(fig)
    print(f"  Saved: {out_path}  ({n_rows} filas x {n_cols} columnas)")


if __name__ == "__main__":
    main()