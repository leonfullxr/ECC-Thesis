#!/usr/bin/env python3
import sys
import argparse
import numpy as np
import matplotlib.pyplot as plt

def compute_curve(a, b, x_min, x_max, num=2000):
    x = np.linspace(x_min, x_max, num)
    f = x**3 + a*x + b
    y_up = np.full_like(x, np.nan)
    y_dn = np.full_like(x, np.nan)
    mask = f >= 0
    y_up[mask] = np.sqrt(f[mask])
    y_dn[mask] = -np.sqrt(f[mask])
    return x, y_up, y_dn

def find_segments(mask):
    segs, in_seg = [], False
    for i, m in enumerate(mask):
        if m and not in_seg:
            start = i; in_seg = True
        elif not m and in_seg:
            segs.append((start, i-1)); in_seg = False
    if in_seg: segs.append((start, len(mask)-1))
    return segs

def real_roots(a, b):
    roots = np.roots([1, 0, a, b])
    return sorted(r.real for r in roots if abs(r.imag) < 1e-6)

def main():
    p = argparse.ArgumentParser(
        description="Plot real elliptic curve $y^2=x^3+ax+b$ over ℝ"
    )
    p.add_argument("--a", type=float, default=4.0, help="coefficient a")
    p.add_argument("--b", type=float, default=7.0, help="coefficient b")
    args = p.parse_args()

    a, b = args.a, args.b
    x_min, x_max = -5, 5

    x, y_up, y_dn = compute_curve(a, b, x_min, x_max)
    finite = np.isfinite(y_up)
    y_lim = (np.nanmax(y_up[finite]) * 1.1) if finite.any() else 1
    R = max(abs(x_min), abs(x_max), y_lim)
    mn, mx = -R, R

    # find real roots to decide if there's a loop
    roots = real_roots(a, b)

    fig, ax = plt.subplots(figsize=(6,6))

    # 1) draw center‐axes first (so curve overwrites them)
    ax.axvline(0, color="black", linestyle="--", linewidth=1, zorder=1)
    ax.axhline(0, color="black", linestyle="--", linewidth=1, zorder=1)

    # 2) split into contiguous real segments
    segs = find_segments(np.isfinite(y_up))
    curve_color = "black"

    # determine loop interval if 3 real roots
    loop_interval = None
    if len(roots) == 3:
        r1, r2, r3 = roots
        # loop is where f>=0 between r1 and r2
        loop_interval = (r1, r2)

    for s, e in segs:
        xs = x[s:e+1]
        yu = y_up[s:e+1]
        yd = y_dn[s:e+1]

        # check if this segment lies within the loop interval
        if loop_interval:
            r1, r2 = loop_interval
            # approximate: segment midpoint in [r1, r2]
            midx = (xs[0] + xs[-1]) / 2
            if r1 <= midx <= r2:
                # close loop
                xs_loop = np.concatenate([xs, xs[::-1], [xs[0]]])
                ys_loop = np.concatenate([yu, yd[::-1], [yu[0]]])
                ax.plot(xs_loop, ys_loop, color=curve_color, linewidth=2, zorder=2)
                continue

        # otherwise plot open branches (they cross axes normally)
        ax.plot(xs, yu, color=curve_color, linewidth=2, zorder=2)
        ax.plot(xs, yd, color=curve_color, linewidth=2, zorder=2)

    # title & labels
    a_str = f"+ {a}" if a>=0 else f"- {abs(a)}"
    b_str = f"+ {b}" if b>=0 else f"- {abs(b)}"
    eq   = rf"$y^2 = x^3\ {a_str}\,x\ {b_str}$"
    over = r"$\mathrm{over}\;\mathbb{R}$"
    ax.set_title(f"{eq}\n{over}", fontsize=18, pad=20)

    ax.set_xlim(mn, mx)
    ax.set_ylim(mn, mx)
    ax.set_xlabel("x")
    ax.set_ylabel("y")
    ax.grid(True, linestyle=":", alpha=0.5)
    ax.set_aspect("equal", "box")
    fig.subplots_adjust(top=0.85)
    plt.show()

if __name__ == "__main__":
    main()
