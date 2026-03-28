#!/usr/bin/env python3
"""
visual_pollard_rho_animated.py
Visualizacion animada en tiempo real del ataque rho de Pollard al ECDLP.

Cada segundo se ejecuta un nuevo paso del algoritmo, mostrando:
  - Panel izquierdo: trayectoria pseudoaleatoria sobre la curva
  - Panel derecho: estructura rho abstracta (cola + ciclo)
  - Panel inferior: estado actual de tortuga, liebre, coeficientes

Se puede pausar/reanudar con la barra espaciadora.

Autor: Leon Elliott Fuller
Fecha: Abril 2026

Uso:
    python3 visual_pollard_rho_animated.py                # Defaults
    python3 visual_pollard_rho_animated.py --speed 500     # 500ms entre pasos
    python3 visual_pollard_rho_animated.py --prime 127     # Campo F_127
    python3 visual_pollard_rho_animated.py --save rho.gif  # Guardar como GIF
"""
import argparse
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.patches import FancyArrowPatch
import sys

# =============================================================================
# EC ARITHMETIC (minimal, for small curves)
# =============================================================================

def ec_points(a, b, p):
    pts = []
    for x in range(p):
        rhs = (x**3 + a*x + b) % p
        for y in range(p):
            if (y*y) % p == rhs:
                pts.append((x, y))
    return pts

def ec_add(P, Q, a, p):
    if P == 'inf': return Q
    if Q == 'inf': return P
    x1, y1 = P; x2, y2 = Q
    if x1 == x2 and y1 != y2: return 'inf'
    if P == Q:
        if y1 == 0: return 'inf'
        lam = (3*x1*x1 + a) * pow(2*y1, -1, p) % p
    else:
        lam = (y2 - y1) * pow(x2 - x1, -1, p) % p
    x3 = (lam*lam - x1 - x2) % p
    y3 = (lam*(x1 - x3) - y1) % p
    return (x3, y3)

def ec_mul(k, P, a, p):
    if k == 0 or P == 'inf': return 'inf'
    R = 'inf'; Q = P; k = abs(k)
    while k > 0:
        if k & 1: R = ec_add(R, Q, a, p)
        Q = ec_add(Q, Q, a, p); k >>= 1
    return R

def ec_order(P, a, p):
    Q = P
    for i in range(1, 2*p + 5):
        if Q == 'inf': return i
        Q = ec_add(Q, P, a, p)
    return None

# =============================================================================
# POLLARD RHO STATE MACHINE
# =============================================================================

class PollardRhoAnimated:
    """Manages the state of Pollard's rho step by step."""

    def __init__(self, P, Q, a, p, n):
        self.P = P
        self.Q = Q
        self.a_coeff = a
        self.p = p
        self.n = n

        # Tortoise state
        self.T = P
        self.at = 1
        self.bt = 0

        # Hare state
        self.H = P
        self.ah = 1
        self.bh = 0

        # History for visualization
        self.tortoise_history = [(P, 1, 0)]
        self.hare_history = [(P, 1, 0)]
        self.step_count = 0
        self.found = False
        self.d = None
        self.collision_step = None

        # Track which partition each step used
        self.partition_history = []

    def partition_label(self, R):
        if R == 'inf': return '$S_1$: $+Q$'
        s = R[0] % 3
        if s == 0: return '$S_1$: $+Q$'
        elif s == 1: return '$S_2$: $2R$'
        else: return '$S_3$: $+P$'

    def partition_color(self, R):
        if R == 'inf': return '#E91E63'
        s = R[0] % 3
        if s == 0: return '#E91E63'    # pink - add Q
        elif s == 1: return '#FF9800'   # orange - double
        else: return '#2196F3'          # blue - add P

    def _step_one(self, R, ar, br):
        if R == 'inf': s = 0
        else: s = R[0] % 3
        if s == 0:
            return ec_add(R, self.Q, self.a_coeff, self.p), ar, (br+1) % self.n
        elif s == 1:
            return ec_add(R, R, self.a_coeff, self.p), (2*ar) % self.n, (2*br) % self.n
        else:
            return ec_add(R, self.P, self.a_coeff, self.p), (ar+1) % self.n, br

    def do_step(self):
        """Execute one step of the algorithm. Returns True if collision found."""
        if self.found:
            return True

        self.step_count += 1

        # Record partition used by tortoise
        self.partition_history.append(self.partition_label(self.T))

        # Tortoise: one step
        self.T, self.at, self.bt = self._step_one(self.T, self.at, self.bt)
        self.tortoise_history.append((self.T, self.at, self.bt))

        # Hare: two steps
        self.H, self.ah, self.bh = self._step_one(self.H, self.ah, self.bh)
        self.H, self.ah, self.bh = self._step_one(self.H, self.ah, self.bh)
        self.hare_history.append((self.H, self.ah, self.bh))

        # Check collision
        if self.T == self.H:
            self.found = True
            self.collision_step = self.step_count
            db = (self.bh - self.bt) % self.n
            da = (self.at - self.ah) % self.n
            if db != 0:
                try:
                    self.d = (da * pow(db, -1, self.n)) % self.n
                except:
                    self.d = None
            return True

        return False


# =============================================================================
# ANIMATED VISUALIZATION
# =============================================================================

def run_animation(P, Q, a, b, p, speed_ms=1000, save_path=None):
    n = ec_order(P, a, p)
    d_real = next((i for i in range(n) if ec_mul(i, P, a, p) == Q), None)

    rho = PollardRhoAnimated(P, Q, a, p, n)

    # Precompute all curve points for background
    all_pts = ec_points(a, b, p)

    # --- Setup figure ---
    fig = plt.figure(figsize=(15, 8))
    fig.patch.set_facecolor('#FAFAFA')

    # Grid: top-left = curve, top-right = rho structure, bottom = status
    ax_curve = fig.add_axes([0.04, 0.22, 0.44, 0.72])
    ax_rho   = fig.add_axes([0.52, 0.22, 0.46, 0.72])
    ax_info  = fig.add_axes([0.04, 0.02, 0.92, 0.18])
    ax_info.axis('off')

    # Pause state
    state = {'paused': False}

    def on_key(event):
        if event.key == ' ':
            state['paused'] = not state['paused']

    fig.canvas.mpl_connect('key_press_event', on_key)

    # --- Static elements ---
    ax_curve.set_title(f'Rho de Pollard: trayectoria en tiempo real\n'
                       f'$y^2 = x^3 + {a}x + {b}$ mod ${p}$,  '
                       f'$P={P}$,  $Q={Q}$,  $n={n}$', fontsize=10)
    ax_curve.set_xlabel('$x$'); ax_curve.set_ylabel('$y$')
    ax_curve.set_xlim(-3, p+3); ax_curve.set_ylim(-3, p+3)
    ax_curve.set_aspect('equal')
    ax_curve.grid(True, linestyle=':', alpha=0.3)

    # Background curve points
    if all_pts:
        bgx, bgy = zip(*all_pts)
        ax_curve.scatter(bgx, bgy, c='#E0E0E0', s=6, zorder=1)

    ax_rho.set_title('Estructura $\\rho$: cola + ciclo', fontsize=10)
    ax_rho.axis('off')

    # Persistent plot elements
    trail_line, = ax_curve.plot([], [], 'o-', color='#9C27B0', markersize=4,
                                linewidth=0.8, alpha=0.4, zorder=3)
    tortoise_dot, = ax_curve.plot([], [], 'o', color='#4CAF50', markersize=14,
                                   zorder=8, markeredgecolor='darkgreen',
                                   markeredgewidth=1.5)
    hare_dot, = ax_curve.plot([], [], 's', color='#F44336', markersize=12,
                               zorder=8, markeredgecolor='darkred',
                               markeredgewidth=1.5)
    new_pt, = ax_curve.plot([], [], '*', color='#FFD600', markersize=18,
                             zorder=9, markeredgecolor='#F57F17',
                             markeredgewidth=1)

    # Legend
    ax_curve.plot([], [], 'o', color='#4CAF50', markersize=8, label='Tortuga $T_i$')
    ax_curve.plot([], [], 's', color='#F44336', markersize=8, label='Liebre $H_i$')
    ax_curve.plot([], [], '*', color='#FFD600', markersize=10, label='Ultimo paso')
    ax_curve.legend(fontsize=8, loc='upper right')

    # Info text object
    info_text = ax_info.text(0.0, 0.9, '', transform=ax_info.transAxes,
                              fontsize=9, verticalalignment='top',
                              fontfamily='monospace',
                              bbox=dict(boxstyle='round,pad=0.4',
                                        facecolor='#FFFDE7', alpha=0.9))

    collision_text = ax_curve.text(0.5, 0.5, '', transform=ax_curve.transAxes,
                                    fontsize=16, ha='center', va='center',
                                    fontweight='bold', color='#4CAF50',
                                    bbox=dict(boxstyle='round,pad=0.6',
                                              facecolor='white', alpha=0.9,
                                              edgecolor='#4CAF50', linewidth=2),
                                    zorder=20, visible=False)

    # Rho structure tracking
    rho_nodes_x = []
    rho_nodes_y = []
    rho_colors = []
    rho_spacing = 0.9

    def update(frame):
        if state['paused']:
            return

        if rho.found and rho.step_count > rho.collision_step + 3:
            return  # Stop updating after showing collision

        # Execute one step
        collision = rho.do_step()

        # --- Update curve panel ---
        # Trail of all tortoise points
        trail_pts = [(pt[0], pt[1]) for (pt, _, _) in rho.tortoise_history
                     if pt != 'inf']
        if trail_pts:
            tx, ty = zip(*trail_pts)
            trail_line.set_data(tx, ty)

        # Current tortoise position
        if rho.T != 'inf':
            tortoise_dot.set_data([rho.T[0]], [rho.T[1]])
        else:
            tortoise_dot.set_data([], [])

        # Current hare position
        if rho.H != 'inf':
            hare_dot.set_data([rho.H[0]], [rho.H[1]])
        else:
            hare_dot.set_data([], [])

        # Highlight newest point
        if rho.T != 'inf':
            new_pt.set_data([rho.T[0]], [rho.T[1]])
        else:
            new_pt.set_data([], [])

        # Draw arrow from previous to current tortoise point
        if len(trail_pts) >= 2:
            prev = trail_pts[-2]
            curr = trail_pts[-1]
            part_color = rho.partition_color(
                rho.tortoise_history[-2][0] if len(rho.tortoise_history) >= 2 else P)
            ax_curve.annotate('', xy=curr, xytext=prev,
                              arrowprops=dict(arrowstyle='->', color=part_color,
                                              lw=1.5, alpha=0.7),
                              zorder=4)

        # --- Update rho structure panel ---
        ax_rho.clear()
        ax_rho.set_title('Estructura $\\rho$: cola + ciclo', fontsize=10)
        ax_rho.axis('off')

        n_pts = len(rho.tortoise_history)
        # Detect cycle in tortoise history
        seen = {}
        tail_l = n_pts
        cycle_l = 0
        for i, (pt, _, _) in enumerate(rho.tortoise_history):
            k = pt if pt == 'inf' else (pt[0], pt[1])
            if k in seen:
                tail_l = seen[k]
                cycle_l = i - tail_l
                break
            seen[k] = i

        # Layout nodes
        rho_nx = []; rho_ny = []; rho_nc = []
        max_show = min(n_pts, 20)

        for i in range(min(tail_l, max_show)):
            rho_nx.append(0)
            rho_ny.append(-i * rho_spacing)
            rho_nc.append('#2196F3')

        if cycle_l > 0:
            cx, cy = 2.2, -(tail_l - 1) * rho_spacing
            radius = max(0.9, cycle_l * 0.18)
            for j in range(min(cycle_l, max_show - len(rho_nx))):
                angle = np.pi/2 - 2*np.pi*j / cycle_l
                rho_nx.append(cx + radius * np.cos(angle))
                rho_ny.append(cy + radius * np.sin(angle))
                rho_nc.append('#F44336')

        # Draw rho nodes
        for i in range(len(rho_nx)):
            ax_rho.scatter(rho_nx[i], rho_ny[i], c=rho_nc[i], s=90,
                           zorder=5, edgecolors='black', linewidths=0.5)
            ax_rho.annotate(f'$R_{{{i}}}$', (rho_nx[i], rho_ny[i]),
                            textcoords="offset points", xytext=(10, 0),
                            fontsize=6.5, color='#333333')

        # Draw arrows
        for i in range(len(rho_nx) - 1):
            ax_rho.annotate('', xy=(rho_nx[i+1], rho_ny[i+1]),
                            xytext=(rho_nx[i], rho_ny[i]),
                            arrowprops=dict(arrowstyle='->', color='#999999',
                                            lw=1.2, connectionstyle='arc3,rad=0.06'))

        # Close cycle arrow
        if cycle_l > 0 and tail_l + cycle_l <= len(rho_nx):
            last = tail_l + cycle_l - 1
            ax_rho.annotate('', xy=(rho_nx[tail_l], rho_ny[tail_l]),
                            xytext=(rho_nx[last], rho_ny[last]),
                            arrowprops=dict(arrowstyle='->', color='#F44336',
                                            lw=2.0, connectionstyle='arc3,rad=0.3'))

        # Highlight current tortoise and hare in rho diagram
        t_idx = min(rho.step_count, len(rho_nx) - 1)
        if t_idx < len(rho_nx):
            ax_rho.scatter(rho_nx[t_idx], rho_ny[t_idx], c='#4CAF50', s=200,
                           zorder=10, marker='o', edgecolors='darkgreen',
                           linewidths=2)

        # Rho panel legend
        partition_text = (rho.partition_history[-1]
                          if rho.partition_history else '---')
        T_str = f"({rho.T[0]}, {rho.T[1]})" if rho.T != 'inf' else "O"
        H_str = f"({rho.H[0]}, {rho.H[1]})" if rho.H != 'inf' else "O"

        rho_info = (
            f"Paso {rho.step_count}\n\n"
            f"Regla aplicada:\n  {partition_text}\n\n"
            f"Tortuga $T$:\n  {T_str}\n"
            f"  $= {rho.at}P + {rho.bt}Q$\n\n"
            f"Liebre $H$:\n  {H_str}\n"
            f"  $= {rho.ah}P + {rho.bh}Q$\n\n"
            f"$T = H$?  {'SI!' if rho.found else 'No'}"
        )
        ax_rho.text(0.55, 0.97, rho_info, transform=ax_rho.transAxes,
                    fontsize=8, verticalalignment='top', fontfamily='monospace',
                    bbox=dict(boxstyle='round,pad=0.4', facecolor='#FFFDE7',
                              alpha=0.9))

        if len(rho_nx) > 0:
            all_x = rho_nx; all_y = rho_ny
            margin = 1.5
            ax_rho.set_xlim(min(all_x) - margin, max(all_x) + 6)
            ax_rho.set_ylim(min(all_y) - margin, max(all_y) + margin)

        # --- Update info panel ---
        if not collision:
            info = (
                f"Paso {rho.step_count:3d}  |  "
                f"Tortuga: {T_str:>12s} = {rho.at:3d}P + {rho.bt:3d}Q  |  "
                f"Liebre: {H_str:>12s} = {rho.ah:3d}P + {rho.bh:3d}Q  |  "
                f"T == H? No  |  "
                f"[Espacio] para pausar"
            )
            info_text.set_text(info)
        else:
            if rho.d is not None:
                verify_pt = ec_mul(rho.d, P, a, p)
                ok = 'CORRECTO' if rho.d == d_real else 'ERROR'
                result = (
                    f"COLISION en paso {rho.collision_step}!   "
                    f"T = H = {T_str}    |    "
                    f"d = ({rho.at}-{rho.ah}) / ({rho.bh}-{rho.bt}) mod {n} "
                    f"= {rho.d}    |    "
                    f"Verificacion: {rho.d}P = {verify_pt}  {ok}"
                )
            else:
                result = (
                    f"COLISION degenerada en paso {rho.collision_step}   "
                    f"(gcd(b_H - b_T, n) != 1, reintentar con otra particion)"
                )
            info_text.set_text(result)
            info_text.set_bbox(dict(boxstyle='round,pad=0.4',
                                     facecolor='#C8E6C9', alpha=0.9))

            if rho.d is not None:
                collision_text.set_text(
                    f'Colision encontrada!\n'
                    f'$d = {rho.d}$\n'
                    f'en {rho.collision_step} pasos')
            else:
                collision_text.set_text(
                    f'Colision degenerada\n'
                    f'en paso {rho.collision_step}')
            collision_text.set_visible(True)

        fig.canvas.draw_idle()

    # --- Create animation ---
    max_frames = 5 * int(np.sqrt(n)) + 50
    anim = animation.FuncAnimation(fig, update, frames=max_frames,
                                    interval=speed_ms, repeat=False)

    if save_path:
        print(f"Guardando animacion en {save_path}...")
        if save_path.endswith('.gif'):
            anim.save(save_path, writer='pillow', fps=1000//speed_ms)
        else:
            anim.save(save_path, writer='ffmpeg', fps=1000//speed_ms)
        print(f"  Guardado: {save_path}")
    else:
        plt.show()

    plt.close()


# =============================================================================
# MAIN
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description='Visualizacion animada del rho de Pollard (TFG)')
    parser.add_argument('--prime', type=int, default=97,
                        help='Primo del campo (default: 97)')
    parser.add_argument('--a', type=int, default=2,
                        help='Coeficiente a (default: 2)')
    parser.add_argument('--b', type=int, default=3,
                        help='Coeficiente b (default: 3)')
    parser.add_argument('--speed', type=int, default=1000,
                        help='Milisegundos entre pasos (default: 1000)')
    parser.add_argument('--save', type=str, default=None,
                        help='Guardar como GIF/MP4 (e.g., rho.gif)')
    parser.add_argument('--target', type=int, default=None,
                        help='Valor d objetivo (default: n//3)')
    args = parser.parse_args()

    p, a, b = args.prime, args.a, args.b
    print(f"Curva: y^2 = x^3 + {a}x + {b} mod {p}")
    print(f"Velocidad: {args.speed}ms entre pasos")
    if not args.save:
        print(f"[Espacio] para pausar/reanudar")
    print()

    pts = ec_points(a, b, p)
    print(f"  {len(pts)} puntos en la curva")

    # Find good generator
    P, n = None, 0
    for pt in pts:
        order = ec_order(pt, a, p)
        if order and order > 10:
            P = pt; n = order; break

    if P is None:
        print("No se encontro generador con orden > 10.")
        sys.exit(1)

    d_target = args.target if args.target else max(3, n // 3)
    Q = ec_mul(d_target, P, a, p)
    print(f"  Generador P = {P}, orden n = {n}")
    print(f"  Objetivo Q = {Q} = {d_target}*P")
    print(f"  Complejidad esperada: ~sqrt({n}) = ~{int(np.sqrt(n))} pasos")
    print()

    run_animation(P, Q, a, b, p, speed_ms=args.speed, save_path=args.save)


if __name__ == '__main__':
    main()