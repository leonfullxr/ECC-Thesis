#!/usr/bin/env python3
"""
Generates the static Pollard rho figure for the thesis chapter.
Curve: y^2 = x^3 + 2x + 1 mod 89,  P=(1,2),  Q=(87,73)=15P,  n=47
"""
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import FancyArrowPatch
import math

# ── EC arithmetic ────────────────────────────────────────────────────────────

def ec_add(P, Q, a, p):
    if P is None: return Q
    if Q is None: return P
    x1, y1 = P; x2, y2 = Q
    if x1 == x2 and (y1 + y2) % p == 0: return None
    if P == Q:
        if y1 == 0: return None
        lam = (3*x1*x1 + a) * pow(2*y1, -1, p) % p
    else:
        lam = (y2 - y1) * pow(x2 - x1, -1, p) % p
    x3 = (lam*lam - x1 - x2) % p
    y3 = (lam*(x1 - x3) - y1) % p
    return (x3, y3)

def scalar_mul(k, P, a, p):
    R = None
    Q = P
    while k:
        if k & 1: R = ec_add(R, Q, a, p)
        Q = ec_add(Q, Q, a, p)
        k >>= 1
    return R

# ── Pollard rho iteration ─────────────────────────────────────────────────────

def f_step(R, aR, bR, P, G, a, p, n):
    x = R[0] % 3
    if x == 0:
        R2 = ec_add(R, G, a, p)
        return R2, aR, (bR + 1) % n
    elif x == 1:
        R2 = ec_add(R, R, a, p)
        return R2, (2*aR) % n, (2*bR) % n
    else:
        R2 = ec_add(R, P, a, p)
        return R2, (aR + 1) % n, bR

# ── Parameters (curve y^2 = x^3 + 2x + 1 mod 89) ────────────────────────────

p = 89; a_coef = 2; b_coef = 1; n = 47
P = (1, 2)
G = scalar_mul(15, P, a_coef, p)   # Q = 15P = (87,73)

# run full sequence to find collision
t, at, bt = P, 1, 0
h, ah, bh = P, 1, 0

seq = [P]
collision_step = None

for i in range(1, 200):
    t, at, bt = f_step(t, at, bt, P, G, a_coef, p, n)
    h, ah, bh = f_step(h, ah, bh, P, G, a_coef, p, n)
    h, ah, bh = f_step(h, ah, bh, P, G, a_coef, p, n)
    seq.append(t)
    if t == h:
        collision_step = i
        break

# also collect full sequence for left panel
full_seq = [P]
R, aR, bR = P, 1, 0
for _ in range(max(collision_step * 2, 30)):
    R, aR, bR = f_step(R, aR, bR, P, G, a_coef, p, n)
    full_seq.append(R)
    if len(full_seq) > 60: break

# ── Figure ───────────────────────────────────────────────────────────────────

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5.5))
fig.patch.set_facecolor('white')

# ── Left panel: trajectory on the curve ──────────────────────────────────────
all_pts = set()
for x in range(p):
    rhs = (x**3 + a_coef*x + b_coef) % p
    for y in range(p):
        if y*y % p == rhs:
            all_pts.add((x, y))

xs_all = [pt[0] for pt in all_pts]
ys_all = [pt[1] for pt in all_pts]
ax1.scatter(xs_all, ys_all, s=4, color='lightgray', zorder=1, label='Puntos de la curva')

steps = min(collision_step + 1, len(full_seq))
traj_x = [pt[0] for pt in full_seq[:steps]]
traj_y = [pt[1] for pt in full_seq[:steps]]

colors = plt.cm.plasma(np.linspace(0.05, 0.95, steps))
for i in range(steps - 1):
    ax1.annotate('', xy=(traj_x[i+1], traj_y[i+1]),
                 xytext=(traj_x[i], traj_y[i]),
                 arrowprops=dict(arrowstyle='->', color=colors[i], lw=1.2))

sc = ax1.scatter(traj_x, traj_y, c=np.arange(steps),
                 cmap='plasma', s=40, zorder=3)

# mark collision
cx, cy = full_seq[collision_step]
ax1.scatter([cx], [cy], marker='*', s=300, color='red', zorder=5, label=f'Colisión (paso {collision_step})')

ax1.set_xlim(-2, p+2); ax1.set_ylim(-2, p+2)
ax1.set_xlabel('x', fontsize=11); ax1.set_ylabel('y', fontsize=11)
ax1.set_title(r'Trayectoria pseudoaleatoria sobre $E(\mathbb{F}_{89})$', fontsize=12)
ax1.legend(fontsize=9, loc='upper right')
plt.colorbar(sc, ax=ax1, label='Paso')

# ── Right panel: abstract rho structure ──────────────────────────────────────
# detect tail length lambda and cycle length mu
seen = {}
R2, a2, b2 = P, 1, 0
for idx in range(200):
    if R2 in seen:
        lam = seen[R2]
        mu = idx - lam
        break
    seen[R2] = idx
    R2, a2, b2 = f_step(R2, a2, b2, P, G, a_coef, p, n)
else:
    lam, mu = 2, 13  # fallback

ax2.set_xlim(-1.8, 1.8); ax2.set_ylim(-1.8, 2.2)
ax2.set_aspect('equal'); ax2.axis('off')
ax2.set_title(r'Estructura $\rho$ abstracta ($\lambda=%d$, $\mu=%d$)' % (lam, mu), fontsize=12)

# tail nodes (vertical, above)
tail_positions = []
for i in range(lam + 1):
    tail_positions.append((0.0, 1.0 + i * 0.55))
tail_positions = tail_positions[::-1]  # top to bottom

# cycle nodes (circle below)
cycle_positions = []
radius = 0.9
center = (0.0, 0.0)
for i in range(mu):
    angle = math.pi/2 - 2*math.pi*i/mu
    cycle_positions.append((center[0] + radius*math.cos(angle),
                            center[1] + radius*math.sin(angle)))

def draw_node(ax, pos, label, color, size=0.22):
    circ = plt.Circle(pos, size, color=color, ec='gray', lw=1.2, zorder=3)
    ax.add_patch(circ)
    ax.text(pos[0], pos[1], label, ha='center', va='center', fontsize=8, zorder=4)

def draw_arrow(ax, p1, p2, color='steelblue'):
    dx, dy = p2[0]-p1[0], p2[1]-p1[1]
    norm = math.sqrt(dx*dx+dy*dy)
    r = 0.22
    start = (p1[0]+r*dx/norm, p1[1]+r*dy/norm)
    end   = (p2[0]-r*dx/norm, p2[1]-r*dy/norm)
    ax.annotate('', xy=end, xytext=start,
                arrowprops=dict(arrowstyle='->', color=color, lw=1.5))

# draw tail
for i, pos in enumerate(tail_positions):
    label = f'$x_{i}$' if i < lam else f'$x_{{{lam}}}$'
    draw_node(ax2, pos, f'$x_{i}$', '#AED6F1')
for i in range(len(tail_positions)-1):
    draw_arrow(ax2, tail_positions[i], tail_positions[i+1])

# draw cycle
for i, pos in enumerate(cycle_positions):
    draw_node(ax2, pos, f'$x_{{{lam+i}}}$', '#F1948A')
for i in range(len(cycle_positions)):
    draw_arrow(ax2, cycle_positions[i], cycle_positions[(i+1)%mu], color='crimson')

# connect tail to cycle
draw_arrow(ax2, tail_positions[-1], cycle_positions[0])

# labels
ax2.text(-1.5, 1.3, f'Cola\n$\\lambda={lam}$', fontsize=10, color='steelblue',
         ha='center', style='italic')
ax2.text(1.45, 0.0, f'Ciclo\n$\\mu={mu}$', fontsize=10, color='crimson',
         ha='center', style='italic')

plt.tight_layout(pad=1.5)
out = '/home/leon/Documents/ECC-Thesis/docs/Plantilla_TFG_latex/imagenes/attack_pollard_rho.png'
plt.savefig(out, dpi=150, bbox_inches='tight', facecolor='white')
print(f'Saved: {out}')
plt.close()
