#!/usr/bin/env python3
import sys
import numpy as np
import matplotlib.pyplot as plt

# ——— EC arithmetic ———
def inv_mod(x, p):
    return pow(int(x), -1, p)

def ec_add(P, Q, a, p):
    x1, y1 = P
    x2, y2 = Q
    if P == Q:
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

# ——— Torus mesh ———
def make_torus(R, r, nu=120, nv=60):
    u = np.linspace(0, 2*np.pi, nu)
    v = np.linspace(0, 2*np.pi, nv)
    U, V = np.meshgrid(u, v, indexing='xy')
    X = (R + r*np.cos(V)) * np.cos(U)
    Y = (R + r*np.cos(V)) * np.sin(U)
    Z = r * np.sin(V)
    return X, Y, Z

def main():
    # keep your a,b unchanged
    a, b = 1, 1

    # parse [sum] and [modulus]
    args = [arg.lower() for arg in sys.argv[1:]]
    mode = 'sum' if 'sum' in args else 'normal'
    if 'sum' in args: args.remove('sum')
    try:
        p = int(args[0]) if args else 251
        assert p > 1
    except:
        print("Usage: python3 torus.py [sum] [modulus>1]")
        sys.exit(1)

    pts = points_on_curve(a, b, p)
    if not pts:
        print(f"No points on y^2=x^3+x+1 mod {p}")
        return
    xs, ys = zip(*pts)
    N = len(pts)

    cmap = plt.colormaps['hsv'](np.linspace(0,1,N))

    # —— make torus wider —— 
    R, r = 15.0, 5.0  
    us = 2*np.pi*np.array(xs)/p
    vs = 2*np.pi*np.array(ys)/p
    XP = (R + r*np.cos(vs))*np.cos(us)
    YP = (R + r*np.cos(vs))*np.sin(us)
    ZP = r*np.sin(vs)
    Xs, Ys, Zs = make_torus(R, r)

    # pick P/Q
    P = pts[N//2]
    Q = next(q for q in pts if q[0] != P[0])
    if mode=='sum':
        Rpt = ec_add(P, Q, a, p)

    # —— figure with GridSpec —— 
    fig = plt.figure(figsize=(10,5), constrained_layout=True)
    gs  = fig.add_gridspec(1, 2, width_ratios=[1, 1.5])
    ax1 = fig.add_subplot(gs[0])
    ax2 = fig.add_subplot(gs[1], projection='3d')
    # ─── 2D plot ──────────────────────────────────────
    ax1.scatter(xs, ys, c=cmap, s=30, edgecolor='k')
    mid = p/2
    ax1.axvline(mid, color='green', linestyle='--', label=r'$x=\frac{p}{2}$')
    ax1.axhline(mid, color='red',   linestyle='--', label=r'$y=\frac{p}{2}$')

    if mode=='sum':
        x1,y1 = P; x2,y2 = Q
        # find A,B on box boundary...
        if x2==x1:
            A=(x1,0); B=(x1,p)
        else:
            m = (y2-y1)/(x2-x1); c = y1-m*x1
            pts_edge=[]
            # x=0
            y0=c
            if 0<=y0<=p: pts_edge.append((0,y0))
            # x=p
            yp=m*p+c
            if 0<=yp<=p: pts_edge.append((p,yp))
            # y=0
            x0=-c/m
            if 0<=x0<=p: pts_edge.append((x0,0))
            # y=p
            xp=(p-c)/m
            if 0<=xp<=p: pts_edge.append((xp,p))
            # unique & pick two
            A,B = pts_edge[:2] if len(pts_edge)>=2 else (P,Q)

        ax1.plot([A[0],B[0]],[A[1],B[1]],
                 c='tab:blue', lw=2, label='ℓ(P,Q)')
        ax1.scatter([P[0],Q[0]],[P[1],Q[1]],
                    s=100, facecolors='none', edgecolors='k', label='P, Q')
        ax1.scatter([Rpt[0]],[Rpt[1]],
                    s=120, marker='*', c='tab:blue', edgecolor='k', label='R=P+Q')

    ax1.set_title(f"2D: $y^2=x^3+x+1$ (mod {p})")
    ax1.set_xlim(-1,p); ax1.set_ylim(-1,p)
    step=max(1,p//10)
    ax1.set_xticks(np.arange(0,p+1,step))
    ax1.set_yticks(np.arange(0,p+1,step))
    ax1.set_aspect('equal','box')
    ax1.grid(True, linestyle=':', alpha=0.5)
    ax1.legend(loc='upper right')

    # ─── 3D TORUS ────────────────────────────────────
    ax2.plot_surface(Xs, Ys, Zs,
                     rstride=4, cstride=4,
                     color='lightgray', alpha=0.05, linewidth=0)
    ax2.scatter(XP, YP, ZP, c=cmap, s=12, depthshade=False)

    # mid‐loops
    v_ = np.linspace(0,2*np.pi,200)
    ax2.plot((R+r*np.cos(v_))*np.cos(np.pi),
             (R+r*np.cos(v_))*np.sin(np.pi),
             r*np.sin(v_), c='green', lw=3)
    u_ = np.linspace(0,2*np.pi,200)
    ax2.plot((R+r*np.cos(np.pi))*np.cos(u_),
             (R+r*np.cos(np.pi))*np.sin(u_),
             r*np.sin(np.pi), c='red', lw=3)

    if mode=='sum':
        # embed A→B geodesic
        uA, vA = 2*np.pi*A[0]/p, 2*np.pi*A[1]/p
        uB, vB = 2*np.pi*B[0]/p, 2*np.pi*B[1]/p
        t=np.linspace(0,1,200)
        u_seg=uA+(uB-uA)*t; v_seg=vA+(vB-vA)*t
        Xseg=(R+r*np.cos(v_seg))*np.cos(u_seg)
        Yseg=(R+r*np.cos(v_seg))*np.sin(u_seg)
        Zseg=r*np.sin(v_seg)
        ax2.plot(Xseg, Yseg, Zseg, color='tab:blue', lw=4)
        # highlight R
        u3,v3=2*np.pi*Rpt[0]/p,2*np.pi*Rpt[1]/p
        Xr=(R+r*np.cos(v3))*np.cos(u3)
        Yr=(R+r*np.cos(v3))*np.sin(u3)
        Zr=r*np.sin(v3)
        ax2.scatter([Xr],[Yr],[Zr],s=150,marker='*',
                    c='tab:blue', edgecolor='k')

    ax2.set_title(f"Torus embedding (mod {p})")
    M=R+r+0.3
    ax2.set_xlim(-M,M); ax2.set_ylim(-M,M); ax2.set_zlim(-r-0.3,r+0.3)
    ax2.set_box_aspect((1,1,0.6))
    ax2.axis('off')

    plt.show()

if __name__=="__main__":
    main()
