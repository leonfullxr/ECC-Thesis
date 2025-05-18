#!/usr/bin/env python3
import numpy as np
import matplotlib.pyplot as plt

def main():
    # fixed singular example: y^2 = x^3 -3x + 2
    a, b = -3.0, 2.0

    # sample domain
    x_min, x_max = -2, 4
    x = np.linspace(x_min, x_max, 2000)
    f = x**3 + a*x + b

    # build y_up/y_dn arrays with NaN defaults
    y_up = np.full_like(x, np.nan)
    y_dn = np.full_like(x, np.nan)
    mask = f >= 0
    # only sqrt on non-negative entries
    y_up[mask] = np.sqrt(f[mask])
    y_dn[mask] = -np.sqrt(f[mask])

    # find the two real roots of f(x)=0 for loop bounds
    roots = np.roots([1, 0, a, b])
    real_roots = np.sort([r.real for r in roots if abs(r.imag) < 1e-6])
    left_root, right_root = real_roots[0], real_roots[-1]

    # partition into loop and branch regions
    loop_mask   = (x >= left_root)  & (x <= right_root)
    branch_mask = (x >= right_root)  & (f >= 0)

    # axis range (symmetric)
    R = max(abs(x_min), abs(x_max), np.nanmax(y_up)*1.1)
    mn, mx = -R, R

    fig, ax = plt.subplots(figsize=(6,6))
    # draw center‐axes under everything
    ax.axvline(0, color="black", linestyle="--", linewidth=1, zorder=1)
    ax.axhline(0, color="black", linestyle="--", linewidth=1, zorder=1)

    # 1) finite loop: close between left_root→right_root
    xs = x[loop_mask]
    yu = y_up[loop_mask]
    yd = y_dn[loop_mask]
    xs_loop = np.concatenate([xs, xs[::-1], [xs[0]]])
    ys_loop = np.concatenate([yu, yd[::-1], [yu[0]]])
    ax.plot(xs_loop, ys_loop, color="black", linewidth=2, zorder=2)

    # 2) infinite branch: only where f>=0 and x≥right_root
    xb = x[branch_mask]
    ax.plot(xb, y_up[branch_mask], color="black", linewidth=2, zorder=2)
    ax.plot(xb, y_dn[branch_mask], color="black", linewidth=2, zorder=2)

    # 3) singular point and its two tangents at x=right_root
    x0 = right_root
    ax.scatter([x0], [0], color="red", s=50, zorder=3, label="singular P")
    # tangent slopes m = ±√(f''(x0)/2) = ±√(3*x0)
    m = np.sqrt(3 * x0)
    for sign in (+1, -1):
        xs_t = x0 + np.linspace(-R*0.3, R*0.3, 50)
        ys_t = sign * m * (xs_t - x0)
        ax.plot(xs_t, ys_t, color="green", linestyle="--", linewidth=2, zorder=3,
                label="tangent" if sign>0 else "")
    ax.legend(loc="upper left")

    ax.set_title(r"$y^2 = x^3 - 3x + 2$" + "\n" + r"$\mathrm{over}\;\mathbb{R}$",
                 fontsize=18, pad=20)
    ax.set_xlabel("x")
    ax.set_ylabel("y")
    ax.set_xlim(mn, mx)
    ax.set_ylim(mn, mx)
    ax.grid(True, linestyle=":", alpha=0.5)
    ax.set_aspect("equal", "box")
    fig.subplots_adjust(top=0.85)

    plt.show()

if __name__=="__main__":
    main()
