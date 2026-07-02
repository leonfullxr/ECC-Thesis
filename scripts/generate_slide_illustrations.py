#!/usr/bin/env python3
"""
generate_slide_illustrations.py
Static, non-interactive illustrations for the defense presentation:
  - ec_shape.png    : non-singular vs singular (node, cusp) elliptic curves
  - ec_addition.png : the chord-and-tangent group law (P+Q and 2P) over R

Author: Leon Elliott Fuller
Date: 2026-06-15

Usage:
    python3 generate_slide_illustrations.py [output_dir]

Default output_dir: docs/presentacion_defensa/imagenes
"""

import os
import sys

import numpy as np
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

# ============================================================================
# STYLE
# ============================================================================
CURVE_COLOR = "#1F2933"   # dark slate (UGRdark)
LINE_COLOR = "#3E5C76"    # soft blue  (UGRsoft) - secant / tangent
RESULT_COLOR = "#A6192E"  # granate    (UGRaccent) - the resulting point
POINT_COLOR = "#1F2933"   # input points
THIRD_COLOR = "#1E8449"   # green - third intersection / reflection (auxiliary)
BLUE = "#2563EB"          # first overlaid example
RED = "#A6192E"           # second overlaid example

DPI = 220

plt.rcParams.update({
    "font.size": 13,
    "axes.titlesize": 15,
    "mathtext.fontset": "cm",
})


def curve_branches(a, b, x_min, x_max, num=4000):
    """Return x and the +/- y branches of y^2 = x^3 + a x + b, NaN where f < 0."""
    x = np.linspace(x_min, x_max, num)
    f = x**3 + a * x + b
    mask = f >= 0
    y_up = np.full_like(x, np.nan)
    y_dn = np.full_like(x, np.nan)
    y_up[mask] = np.sqrt(f[mask])
    y_dn[mask] = -np.sqrt(f[mask])
    return x, y_up, y_dn


def style_axes(ax, x_min, x_max, y_min, y_max):
    ax.axhline(0, color="black", linestyle="--", linewidth=0.8, alpha=0.6, zorder=1)
    ax.axvline(0, color="black", linestyle="--", linewidth=0.8, alpha=0.6, zorder=1)
    ax.set_xlim(x_min, x_max)
    ax.set_ylim(y_min, y_max)
    ax.set_aspect("equal", "box")
    ax.grid(True, linestyle=":", alpha=0.4)
    ax.set_xlabel("x")
    ax.set_ylabel("y")


def plot_curve(ax, a, b, x_min, x_max, color=CURVE_COLOR, label=None, lw=2.2):
    x, y_up, y_dn = curve_branches(a, b, x_min, x_max)
    ax.plot(x, y_up, color=color, linewidth=lw, zorder=3, label=label)
    ax.plot(x, y_dn, color=color, linewidth=lw, zorder=3)


def top_legend(ax, header):
    """Place a titled legend as a header block above the axes."""
    ax.legend(title=header, loc="lower center", bbox_to_anchor=(0.5, 1.005),
              fontsize=10.5, title_fontsize=13, framealpha=0.95,
              edgecolor="#CCCCCC", handlelength=1.4, labelspacing=0.35,
              borderaxespad=0.0)


# ============================================================================
# FIGURE 1: non-singular vs singular
# ============================================================================
def make_ec_shape(out_path):
    y_min, y_max = -3.4, 3.4
    fig, axes = plt.subplots(1, 3, figsize=(14.0, 5.6), constrained_layout=True)

    # --- non-singular: two examples, zoomed (tight left x = -1.5) ------
    #   two-component:  y^2 = x^3 - x      one-component:  y^2 = x^3 - x + 1
    ax = axes[0]
    nx_min, nx_max = -1.5, 2.9
    plot_curve(ax, -1.0, 0.0, nx_min, nx_max, color=BLUE,
               label=r"$y^2 = x^3 - x$")
    plot_curve(ax, -1.0, 1.0, nx_min, nx_max, color=RED,
               label=r"$y^2 = x^3 - x + 1$")
    style_axes(ax, nx_min, nx_max, y_min, y_max)
    top_legend(ax, r"No singular  ($\Delta \neq 0$)")

    # --- singular: node + cusp overlaid (both Delta = 0) --------------
    #   node: y^2 = x^3 - 3x + 2 = (x-1)^2 (x+2)   -> self-intersection at (1,0)
    #   cusp: y^2 = x^3                            -> cusp at (0,0)
    ax = axes[1]
    sx_min, sx_max = -2.6, 2.9
    plot_curve(ax, -3.0, 2.0, sx_min, sx_max, color=BLUE,
               label=r"nodo:  $y^2 = x^3 - 3x + 2$")
    plot_curve(ax, 0.0, 0.0, sx_min, sx_max, color=RED,
               label=r"cuspide:  $y^2 = x^3$")
    style_axes(ax, sx_min, sx_max, y_min, y_max)
    ax.scatter([1], [0], color=BLUE, s=55, zorder=6)
    ax.scatter([0], [0], color=RED, s=55, zorder=6)
    top_legend(ax, r"Singular  ($\Delta = 0$)")

    # --- why singular fails: two tangents at the node -----------------
    #   2P slope lambda = (3x^2+a)/(2y); at the node y=0 -> division by 0,
    #   and the node has two branch tangents -> doubling is ambiguous.
    ax = axes[2]
    fx_min, fx_max = -0.6, 2.6
    assert abs(1.0**3 - 3 * 1.0 + 2) < 1e-9  # node (1,0) on curve, y=0
    plot_curve(ax, -3.0, 2.0, fx_min, fx_max, color=BLUE,
               label=r"$y^2 = x^3 - 3x + 2$")
    slope = np.sqrt(3.0)
    xs = np.array([fx_min, fx_max])
    ax.plot(xs, slope * (xs - 1.0), color=RED, linestyle="--", linewidth=1.8,
            zorder=4, label="tangentes en el nodo")
    ax.plot(xs, -slope * (xs - 1.0), color=RED, linestyle="--", linewidth=1.8,
            zorder=4)
    ax.scatter([1], [0], color=POINT_COLOR, s=80, zorder=6)
    style_axes(ax, fx_min, fx_max, y_min, y_max)
    ax.text(0.5 * (fx_min + fx_max), y_min + 0.45,
            r"dos tangentes $\Rightarrow$ $2P$ indefinido",
            ha="center", fontsize=11, color=RED,
            bbox=dict(boxstyle="round,pad=0.3", fc="#FBEAEC", ec=RED, lw=1.0))
    top_legend(ax, r"Singular: la ley de grupo falla")

    fig.savefig(out_path, dpi=DPI, bbox_inches="tight")
    plt.close(fig)
    print(f"wrote {out_path}")


# ============================================================================
# FIGURE 2: the group law (P+Q and 2P)
# ============================================================================
def label_point(ax, xy, text, dx, dy, color, ha="left"):
    ax.annotate(text, xy=xy, xytext=(xy[0] + dx, xy[1] + dy),
                color=color, fontsize=14, fontweight="bold", ha=ha,
                zorder=6)


def make_ec_addition(out_path):
    # smooth curve y^2 = x^3 - x + 1  (Delta = -16*23 != 0)
    a, b = -1.0, 1.0
    x_min, x_max = -1.9, 2.3
    y_min, y_max = -2.9, 2.9

    # narrower figure + tight wspace: with equal-aspect axes this halves the
    # inter-panel gap and makes the figure taller (= bigger on the slide).
    fig, axes = plt.subplots(1, 2, figsize=(8.4, 5.6), constrained_layout=True)
    fig.get_layout_engine().set(wspace=0.0, w_pad=0.02)

    xs_line = np.linspace(x_min, x_max, 400)

    # -------------------------------------------------- left: P + Q -----
    ax = axes[0]
    plot_curve(ax, a, b, x_min, x_max)
    style_axes(ax, x_min, x_max, y_min, y_max)

    # P and Q chosen so the three intersections (-1.2, 0.05, 1.2) are
    # well separated and no marker/label overlaps another.
    Px = -1.2
    Py = float(np.sqrt(Px**3 + a * Px + b))
    Qx = 1.2
    Qy = float(np.sqrt(Qx**3 + a * Qx + b))
    lam = (Qy - Py) / (Qx - Px)
    Rx = lam**2 - Px - Qx           # x of third intersection / of P+Q
    Ry_line = lam * (Rx - Px) + Py  # y on the secant at Rx = third intersection
    Sy = -Ry_line                   # reflection -> y of P+Q

    # secant through P and Q
    ax.plot(xs_line, Py + lam * (xs_line - Px), color=LINE_COLOR,
            linewidth=1.6, zorder=2)
    # vertical reflection line
    ax.plot([Rx, Rx], [Ry_line, Sy], color=THIRD_COLOR, linestyle=":",
            linewidth=1.6, zorder=2)

    ax.scatter([Px, Qx], [Py, Qy], color=POINT_COLOR, s=95, zorder=5)
    ax.scatter([Rx], [Ry_line], facecolors="white", edgecolors=THIRD_COLOR,
               s=95, linewidths=2.0, zorder=5)
    ax.scatter([Rx], [Sy], color=RESULT_COLOR, s=125, zorder=5)

    label_point(ax, (Px, Py), "P", -0.10, 0.25, POINT_COLOR, ha="right")
    label_point(ax, (Qx, Qy), "Q", 0.14, 0.05, POINT_COLOR)
    label_point(ax, (Rx, Ry_line), r"$-(P{+}Q)$", 0.12, 0.22, THIRD_COLOR)
    label_point(ax, (Rx, Sy), r"$P+Q$", 0.14, -0.45, RESULT_COLOR)
    ax.set_title(r"Suma:  $P + Q$  (cuerda)")

    # -------------------------------------------------- right: 2P -------
    ax = axes[1]
    plot_curve(ax, a, b, x_min, x_max)
    style_axes(ax, x_min, x_max, y_min, y_max)

    P2x, P2y = 1.0, 1.0
    lam2 = (3 * P2x**2 + a) / (2 * P2y)
    Tx = lam2**2 - 2 * P2x           # x of third intersection / of 2P
    Ty_line = lam2 * (Tx - P2x) + P2y  # third intersection (on tangent)
    Dy = -Ty_line                    # reflection -> y of 2P

    # tangent at P
    ax.plot(xs_line, P2y + lam2 * (xs_line - P2x), color=LINE_COLOR,
            linewidth=1.6, zorder=2)
    # vertical reflection line
    ax.plot([Tx, Tx], [Ty_line, Dy], color=THIRD_COLOR, linestyle=":",
            linewidth=1.6, zorder=2)

    ax.scatter([P2x], [P2y], color=POINT_COLOR, s=95, zorder=5)
    ax.scatter([Tx], [Ty_line], facecolors="white", edgecolors=THIRD_COLOR,
               s=95, linewidths=2.0, zorder=5)
    ax.scatter([Tx], [Dy], color=RESULT_COLOR, s=125, zorder=5)

    # label P below the tangent so neither the line nor the curve hides it
    label_point(ax, (P2x, P2y), "P", 0.16, -0.42, POINT_COLOR)
    label_point(ax, (Tx, Ty_line), r"$-2P$", 0.15, -0.52, THIRD_COLOR)
    label_point(ax, (Tx, Dy), r"$2P$", -0.12, 0.30, RESULT_COLOR, ha="right")
    ax.set_title(r"Duplicacion:  $2P$  (tangente)")

    fig.savefig(out_path, dpi=DPI, bbox_inches="tight")
    plt.close(fig)
    print(f"wrote {out_path}")


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        "docs", "presentacion_defensa", "imagenes")
    os.makedirs(out_dir, exist_ok=True)
    make_ec_shape(os.path.join(out_dir, "ec_shape.png"))
    make_ec_addition(os.path.join(out_dir, "ec_addition.png"))


if __name__ == "__main__":
    main()
