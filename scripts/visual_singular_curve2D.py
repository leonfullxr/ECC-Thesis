#!/usr/bin/env python3
import numpy as np
import matplotlib.pyplot as plt

# fixed singular example: y^2 = x^3 - 3x + 2
a, b = -3.0, 2.0

def compute_curve(a, b, x_min, x_max, num=2000):
    x = np.linspace(x_min, x_max, num)
    f = x**3 + a*x + b
    y_up = np.full_like(x, np.nan)
    y_dn = np.full_like(x, np.nan)
    mask = f >= 0
    y_up[mask] = np.sqrt(f[mask])
    y_dn[mask] = -np.sqrt(f[mask])
    return x, y_up, y_dn

def draw(ax, x_min, x_max, y_min, y_max):
    ax.clear()
    # draw center-axes in black
    ax.axvline(0, color="black", linestyle="--", linewidth=1, zorder=1)
    ax.axhline(0, color="black", linestyle="--", linewidth=1, zorder=1)

    # compute curve
    x, y_up, y_dn = compute_curve(a, b, x_min, x_max)
    # find real roots to split loop vs branch
    roots = np.roots([1, 0, a, b])
    real_roots = np.sort([r.real for r in roots if abs(r.imag)<1e-6])
    left_root, right_root = real_roots[0], real_roots[-1]

    # finite loop segment
    loop_mask = (x >= left_root) & (x <= right_root)
    xs = x[loop_mask]; yu = y_up[loop_mask]; yd = y_dn[loop_mask]
    xs_loop = np.concatenate([xs, xs[::-1], [xs[0]]])
    ys_loop = np.concatenate([yu, yd[::-1], [yu[0]]])
    ax.plot(xs_loop, ys_loop, color="black", linewidth=2, zorder=2)

    # infinite branch for x >= right_root
    branch_mask = (x >= right_root)
    xb = x[branch_mask]
    ax.plot(xb, y_up[branch_mask], color="black", linewidth=2, zorder=2)
    ax.plot(xb, y_dn[branch_mask], color="black", linewidth=2, zorder=2)

    # singular node and its two tangents at x0 = right_root
    x0 = right_root
    ax.scatter([x0], [0], color="red", s=50, zorder=3, label="singular P")
    m = np.sqrt(3 * x0)
    for sign in (+1, -1):
        xs_t = x0 + np.linspace(- (x_max-x_min)*0.3, (x_max-x_min)*0.3, 50)
        ys_t = sign * m * (xs_t - x0)
        ax.plot(xs_t, ys_t, color="green", linestyle="--", linewidth=2, zorder=3,
                label="tangent" if sign>0 else "")
    ax.legend(loc="upper left")

    # title & labels
    ax.set_title(r"$y^2 = x^3 - 3x + 2$" + "\n" + r"$\mathrm{over}\;\mathbb{R}$",
                 fontsize=18, pad=20)
    ax.set_xlim(x_min, x_max)
    ax.set_ylim(y_min, y_max)
    ax.set_xlabel("x")
    ax.set_ylabel("y")
    ax.grid(True, linestyle=":", alpha=0.5)
    ax.set_aspect("equal", "box")

def on_scroll(event):
    global x_min, x_max, y_min, y_max
    if event.inaxes is not ax:
        return
    # zoom in or out
    factor = 0.9 if event.button == 'up' else 1.1
    cx = (x_min + x_max) / 2
    cy = (y_min + y_max) / 2
    w = (x_max - x_min) * factor
    h = (y_max - y_min) * factor
    x_min, x_max = cx - w/2, cx + w/2
    y_min, y_max = cy - h/2, cy + h/2
    draw(ax, x_min, x_max, y_min, y_max)
    fig.canvas.draw_idle()

# initial view limits
# choose symmetric around zero and large enough to see shape
base = 6
x_min, x_max = -base, base
y_min, y_max = -base, base

fig, ax = plt.subplots(figsize=(6,6))
draw(ax, x_min, x_max, y_min, y_max)

# connect scroll event
fig.canvas.mpl_connect('scroll_event', on_scroll)

plt.tight_layout()
plt.show()
