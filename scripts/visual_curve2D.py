#!/usr/bin/env python3
import sys
import argparse
import numpy as np
import matplotlib.pyplot as plt

def compute_curve(a, b, x_min, x_max, num=2000):
    """
    Return (x, y_up, y_dn) of length `num`, with y_up and y_dn NaN
    where x^3 + a*x + b < 0, so matplotlib won’t draw across gaps.
    """
    x = np.linspace(x_min, x_max, num)
    f = x**3 + a*x + b

    # initialize with NaN
    y_up = np.full_like(x, np.nan)
    y_dn = np.full_like(x, np.nan)

    mask = f >= 0
    y_up[mask] = np.sqrt(f[mask])
    y_dn[mask] = -np.sqrt(f[mask])

    return x, y_up, y_dn

def main():
    parser = argparse.ArgumentParser(
        description="Plot the real‐plane elliptic curve $y^2 = x^3 + a\,x + b$ over ℝ."
    )
    parser.add_argument("--a", type=float, default=4.0,
                        help="coefficient a (default: 4.0)")
    parser.add_argument("--b", type=float, default=7.0,
                        help="coefficient b (default: 7.0)")
    args = parser.parse_args()

    a, b = args.a, args.b
    x_min, x_max = -5, 5

    x, y_up, y_dn = compute_curve(a, b, x_min, x_max)
    # pick finite y values for limit
    finite = np.isfinite(y_up)
    y_lim = np.nanmax(y_up[finite]) * 1.1 if finite.any() else 1

    # symmetric axis range
    R = max(abs(x_min), abs(x_max), y_lim)
    plot_min, plot_max = -R, R

    fig, ax = plt.subplots(figsize=(6,6))

    # both branches same color; NaNs break the line
    curve_color = "blue"
    ax.plot(x, y_up, color=curve_color, linewidth=2)
    ax.plot(x, y_dn, color=curve_color, linewidth=2)

    # center lines
    ax.axvline(0, color="red", linestyle="--", linewidth=1)
    ax.axhline(0, color="red", linestyle="--", linewidth=1)

    # two‐line math‐text title without parentheses
    a_str = f"+ {a}" if a >= 0 else f"- {abs(a)}"
    b_str = f"+ {b}" if b >= 0 else f"- {abs(b)}"
    eq_line   = rf"$y^2 = x^3\ {a_str}\,x\ {b_str}$"
    over_line = r"$\mathrm{over}\;\mathbb{R}$"
    ax.set_title(f"{eq_line}\n{over_line}", fontsize=18, pad=20)

    ax.set_xlabel("x")
    ax.set_ylabel("y")
    ax.set_xlim(plot_min, plot_max)
    ax.set_ylim(plot_min, plot_max)
    ax.grid(True, linestyle=":", alpha=0.5)
    ax.set_aspect("equal", "box")

    fig.subplots_adjust(top=0.85)
    plt.show()

if __name__ == "__main__":
    main()
