#!/usr/bin/env python3
import sys
import numpy as np
import matplotlib.pyplot as plt

def inv_mod(x, p):
    """Modular inverse of x mod p."""
    return pow(x, -1, p)

def ec_add(P, Q, a, p):
    """Elliptic‐curve addition P+Q for y^2 = x^3 + a x + b over Z/p."""
    x1, y1 = P
    x2, y2 = Q
    if P == Q:
        # tangent
        m = (3*x1*x1 + a) * inv_mod(2*y1, p) % p
    else:
        m = (y2 - y1) * inv_mod(x2 - x1, p) % p
    x3 = (m*m - x1 - x2) % p
    y3 = (m*(x1 - x3) - y1) % p
    return x3, y3

def points_on_curve(a, b, p):
    pts = []
    for x in range(p):
        rhs = (x**3 + a*x + b) % p
        for y in range(p):
            if (y*y) % p == rhs:
                pts.append((x, y))
    return pts

def make_torus(R, r, nu=120, nv=60):
    u = np.linspace(0, 2*np.pi, nu)
    v = np.linspace(0, 2*np.pi, nv)
    U, V = np.meshgrid(u, v, indexing='xy')
    X = (R + r*np.cos(V)) * np.cos(U)
    Y = (R + r*np.cos(V)) * np.sin(U)
    Z = r * np.sin(V)
    return X, Y, Z

def main():
    a, b = -2, 1

    # parse args
    mode = 'normal'
    p = 250
    args = [arg.lower() for arg in sys.argv[1:]]
    if 'sum' in args:
        mode = 'sum'; args.remove('sum')
    if args:
        try:
            p = int(args[0]); assert p > 1
        except:
            print("Usage: python3 torus.py [sum] [modulus>1]")
            sys.exit(1)

    # compute curve points
    pts = points_on_curve(a, b, p)
    if not pts:
        print(f"No solutions mod {p}")
        sys.exit(0)
    xs, ys = zip(*pts)

    # prepare colors (hue by x)
    uniq_x = sorted(set(xs))
    x_to_idx = {x:i for i,x in enumerate(uniq_x)}
    norm = lambda x: x_to_idx[x]/(len(uniq_x)-1)
    colors = [norm(x) for x in xs]

    # torus parameters
    R, r = 4.0, 1.5
    us = 2*np.pi * np.array(xs)/p
    vs = 2*np.pi * np.array(ys)/p
    XP = (R + r*np.cos(vs)) * np.cos(us)
    YP = (R + r*np.cos(vs)) * np.sin(us)
    ZP = r * np.sin(vs)
    Xs, Ys, Zs = make_torus(R, r)

    # pick P,Q
    P = (0, 1)
    Q = (1, 0)
    if mode == 'sum':
        Rpt = ec_add(P, Q, a, p)  # R = P+Q
    # else Rpt unused

    # set up figure
    fig = plt.figure(figsize=(12,5))
    ax1 = fig.add_subplot(1,2,1)
    ax2 = fig.add_subplot(1,2,2, projection='3d')

    # ─── 2D plot ────────────────────────────────────────
    sc = ax1.scatter(xs, ys,
                     c=colors, cmap='hsv',
                     s=30, edgecolor='k')
    cbar = plt.colorbar(sc, ax=ax1, pad=0.02, fraction=0.046)
    cbar.set_label('x-hue', rotation=0, labelpad=10)

    # mid-lines
    mid = p/2
    ax1.axvline(mid, color='green', linestyle='--', label=r'$x=\frac{p}{2}$')
    ax1.axhline(mid, color='red',   linestyle='--', label=r'$y=\frac{p}{2}$')

    # chord P→Q
    ax1.plot([P[0],Q[0]], [P[1],Q[1]],
             c='tab:blue', lw=2, label='ℓ(P,Q)')
    ax1.scatter([P[0],Q[0]], [P[1],Q[1]],
                s=100, facecolors='none', edgecolors='k', label='P,Q')

    if mode == 'sum':
        x3, y3 = Rpt
        ax1.scatter([x3],[y3],
                    s=120, marker='*',
                    c='tab:blue', edgecolor='k',
                    label='R=P+Q')

    ax1.set_title(f"2D: $y^2=x^3-2x+1$ (mod {p})")
    ax1.set_xlim(-1, p); ax1.set_ylim(-1, p)
    step = max(1, p//10)
    ax1.set_xticks(np.arange(0, p+1, step))
    ax1.set_yticks(np.arange(0, p+1, step))
    ax1.set_aspect('equal','box')
    ax1.grid(True, linestyle=':', alpha=0.5)
    ax1.legend(loc='upper right')

    # ─── 3D torus ───────────────────────────────────────
    ax2.plot_surface(Xs, Ys, Zs,
                     rstride=4, cstride=4,
                     color='lightgray', alpha=0.3,
                     linewidth=0)
    ax2.scatter(XP, YP, ZP,
                c=colors, cmap='hsv',
                s=15, depthshade=True)

    # torus mid-loops
    N=200
    u0 = np.pi
    v_ = np.linspace(0,2*np.pi,N)
    xvl = (R + r*np.cos(v_))*np.cos(u0)
    yvl = (R + r*np.cos(v_))*np.sin(u0)
    zvl = r*np.sin(v_)
    ax2.plot(xvl,yvl,zvl, c='green', lw=2)

    v0 = np.pi
    u_ = np.linspace(0,2*np.pi,N)
    xhl = (R + r*np.cos(v0))*np.cos(u_)
    yhl = (R + r*np.cos(v0))*np.sin(u_)
    zhl = r*np.sin(v0)
    ax2.plot(xhl,yhl,zhl, c='red', lw=2)

    # chord geodesic P→Q
    u1, v1 = 2*np.pi*P[0]/p, 2*np.pi*P[1]/p
    u2, v2 = 2*np.pi*Q[0]/p, 2*np.pi*Q[1]/p
    t = np.linspace(0,1,100)
    u_seg = u1 + (u2-u1)*t
    v_seg = v1 + (v2-v1)*t
    Xseg = (R + r*np.cos(v_seg))*np.cos(u_seg)
    Yseg = (R + r*np.cos(v_seg))*np.sin(u_seg)
    Zseg = r*np.sin(v_seg)
    ax2.plot(Xseg, Yseg, Zseg, c='tab:blue', lw=2)

    if mode == 'sum':
        # highlight R on the torus
        u3, v3 = 2*np.pi*Rpt[0]/p, 2*np.pi*Rpt[1]/p
        Xr = (R + r*np.cos(v3))*np.cos(u3)
        Yr = (R + r*np.cos(v3))*np.sin(u3)
        Zr = r*np.sin(v3)
        ax2.scatter([Xr],[Yr],[Zr],
                    s=100, marker='*',
                    c='tab:blue', edgecolor='k')

    ax2.set_title(f"Torus embedding (mod {p})")
    M = R + r + 0.3
    ax2.set_xlim(-M,M); ax2.set_ylim(-M,M); ax2.set_zlim(-r-0.3,r+0.3)
    ax2.set_box_aspect((1,1,0.6))
    ax2.axis('off')

    # manual spacing to avoid tight_layout() bug
    plt.subplots_adjust(left=0.05, right=0.95,
                        top=0.92, bottom=0.08,
                        wspace=0.35)
    plt.show()

if __name__=="__main__":
    main()
