#!/usr/bin/env python3
import sys
import argparse
import numpy as np
import matplotlib.pyplot as plt

# Elliptic Curve Plotter with Interactive Zoom & Point‐Addition Visualization

def compute_curve(a, b, x_min, x_max, num=2000):
    """
    Return (x, y_up, y_dn) arrays, with NaN where the curve is not real.
    """
    x = np.linspace(x_min, x_max, num)
    f = x**3 + a * x + b
    y_up = np.full_like(x, np.nan)
    y_dn = np.full_like(x, np.nan)
    mask = f >= 0
    y_up[mask] = np.sqrt(f[mask])
    y_dn[mask] = -np.sqrt(f[mask])
    return x, y_up, y_dn

def main():
    parser = argparse.ArgumentParser(
        description="Plot real elliptic curve $y^2 = x^3 + a x + b$ over ℝ, "
                    "with optional P+Q visualization and interactive zoom."
    )
    parser.add_argument("--a", type=float, default=4.0,
                        help="coefficient a (default: 4.0)")
    parser.add_argument("--b", type=float, default=7.0,
                        help="coefficient b (default: 7.0)")
    parser.add_argument("--P", nargs=2, type=float, metavar=("X1","Y1"),
                        help="coordinates of point P on the curve")
    parser.add_argument("--Q", nargs=2, type=float, metavar=("X2","Y2"),
                        help="coordinates of point Q on the curve")
    args = parser.parse_args()

    a, b = args.a, args.b
    x_min, x_max = -5, 5

    # Compute curve data
    x, y_up, y_dn = compute_curve(a, b, x_min, x_max)
    finite = np.isfinite(y_up)
    y_max_real = np.nanmax(y_up[finite]) if finite.any() else 1
    R = max(abs(x_min), abs(x_max), y_max_real * 1.1)
    plot_min, plot_max = -R, R

    fig, ax = plt.subplots(figsize=(6,6))

    # 1) Draw center axes first (so curve overwrites them)
    ax.axvline(0, color="black", linestyle="--", linewidth=1, zorder=1)
    ax.axhline(0, color="black", linestyle="--", linewidth=1, zorder=1)

    # 2) Plot the elliptic curve branches
    curve_color = "blue"
    ax.plot(x, y_up, color=curve_color, linewidth=2, zorder=2, label="Curve")
    ax.plot(x, y_dn, color=curve_color, linewidth=2, zorder=2)

    # 3) If P and Q are provided, compute and plot secant/tangent, S and R, and S→R
    if args.P and args.Q:
        x1, y1 = args.P
        x2, y2 = args.Q

        # Compute slope m
        if np.isclose(x1, x2) and np.isclose(y1, y2):
            m = (3*x1**2 + a) / (2*y1)
        else:
            m = (y2 - y1) / (x2 - x1)
        c = y1 - m*x1

        # Intersection S
        x3 = m**2 - x1 - x2
        y3p = m*x3 + c
        xS, yS = x3, y3p

        # Sum R
        xR, yR = x3, -y3p

        # Draw secant/tangent line
        xs_line = np.linspace(min(x1, x2, x3)-1, max(x1, x2, x3)+1, 300)
        ys_line = m*xs_line + c
        ax.plot(xs_line, ys_line,
                color="green", linestyle="-.", linewidth=1.5,
                zorder=3, label="Chord/Tangent")

        # Plot P and Q
        ax.scatter(x1, y1, color="red", s=60, zorder=4, label="P")
        ax.annotate("P", (x1, y1), textcoords="offset points", xytext=(5,5))
        ax.scatter(x2, y2, color="red", s=60, zorder=4, label="Q")
        ax.annotate("Q", (x2, y2), textcoords="offset points", xytext=(5,5))

        # Plot S = P*Q
        ax.scatter(xS, yS, color="orange", s=60, zorder=4, label="S = P*Q")
        ax.annotate("S", (xS, yS), textcoords="offset points", xytext=(5,5))

        # Plot R = P+Q
        ax.scatter(xR, yR, color="magenta", s=60, zorder=4, label="R = P+Q")
        ax.annotate("R", (xR, yR), textcoords="offset points", xytext=(5,-10))

        # Line from S to R
        ax.plot([xS, xR], [yS, yR],
                color="purple", linestyle="--", linewidth=1.5,
                zorder=3, label="S→R")

    # Place legend outside the plot
    ax.legend(loc="upper left", bbox_to_anchor=(1.05, 1), borderaxespad=0.)
    fig.subplots_adjust(top=0.85, right=0.75)

    # Title & labels with LaTeX‐style two‐line title
    a_str = f"+ {a}" if a>=0 else f"- {abs(a)}"
    b_str = f"+ {b}" if b>=0 else f"- {abs(b)}"
    eq_line   = rf"$y^2 = x^3\ {a_str}\,x\ {b_str}$"
    over_line = r"$\mathrm{over}\;\mathbb{R}$"
    ax.set_title(f"{eq_line}\n{over_line}", fontsize=18, pad=20)

    ax.set_xlim(plot_min, plot_max)
    ax.set_ylim(plot_min, plot_max)
    ax.set_xlabel("x")
    ax.set_ylabel("y")
    ax.grid(True, linestyle=":", alpha=0.5)
    ax.set_aspect("equal", "box")

    # ---- Interactive zoom handler ----
    base_scale = 1.5
    def on_scroll(event):
        if event.inaxes != ax: 
            return
        xdata, ydata = event.xdata, event.ydata
        cur_xlim = ax.get_xlim()
        cur_ylim = ax.get_ylim()
        if event.button == 'up':
            scale_factor = 1/base_scale
        elif event.button == 'down':
            scale_factor = base_scale
        else:
            return
        new_width = (cur_xlim[1] - cur_xlim[0]) * scale_factor
        new_height = (cur_ylim[1] - cur_ylim[0]) * scale_factor
        relx = (cur_xlim[1] - xdata) / (cur_xlim[1] - cur_xlim[0])
        rely = (cur_ylim[1] - ydata) / (cur_ylim[1] - cur_ylim[0])
        ax.set_xlim([xdata - new_width*(1-relx), xdata + new_width*relx])
        ax.set_ylim([ydata - new_height*(1-rely), ydata + new_height*rely])
        fig.canvas.draw_idle()

    fig.canvas.mpl_connect('scroll_event', on_scroll)

    plt.show()

if __name__ == "__main__":
    main()
