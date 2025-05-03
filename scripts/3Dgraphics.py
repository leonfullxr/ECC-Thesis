#!/usr/bin/env python3

import argparse
import numpy as np
import networkx as nx
import plotly.graph_objects as go

# Elliptic curve arithmetic over F_p
def inv_mod(x, p):
    return pow(x, p-2, p)

def point_add(P, Q, a, p):
    if P == 'O': return Q
    if Q == 'O': return P
    x1, y1 = P
    x2, y2 = Q
    # P + (-P) = O
    if x1 == x2 and (y1 + y2) % p == 0:
        return 'O'
    # slope
    if P != Q:
        m = ((y2 - y1) * inv_mod(x2 - x1, p)) % p
    else:
        m = ((3*x1*x1 + a) * inv_mod(2*y1, p)) % p
    x3 = (m*m - x1 - x2) % p
    y3 = (m*(x1 - x3) - y1) % p
    return (x3, y3)

# Generate all points on the curve y^2 = x^3 + a*x + b over F_p
def all_points(a, b, p):
    pts = ['O']
    for x in range(p):
        rhs = (x**3 + a*x + b) % p
        for y in range(p):
            if (y*y) % p == rhs:
                pts.append((x, y))
    return pts

# Build and display interactive 3D Cayley diagram with Plotly

def main():
    parser = argparse.ArgumentParser(
        description='Interactive 3D Cayley diagram of E(F_p) with labels'
    )
    parser.add_argument('mod', type=int, nargs='?', default=97,
                        help='Field prime p')
    parser.add_argument('--a', type=int, default=1,
                        help='Curve coefficient a')
    parser.add_argument('--b', type=int, default=1,
                        help='Curve coefficient b')
    parser.add_argument('--gen', type=int, default=1,
                        help='Index of generator in point list')
    args = parser.parse_args()

    p, a, b = args.mod, args.a, args.b
    pts = all_points(a, b, p)
    G = pts[args.gen]

    # Build Cayley graph wrt addition by G
    C = nx.DiGraph()
    C.add_nodes_from(pts)
    for P in pts:
        Q = point_add(P, G, a, p)
        C.add_edge(P, Q)

    # 3D spring layout positions
    pos_3d = nx.spring_layout(C, dim=3, seed=42)

    # Extract node coordinates
    node_xyz = np.array([pos_3d[P] for P in pts])

    # Build edge traces
    edge_x, edge_y, edge_z = [], [], []
    for u, v in C.edges():
        x0, y0, z0 = pos_3d[u]
        x1, y1, z1 = pos_3d[v]
        edge_x += [x0, x1, None]
        edge_y += [y0, y1, None]
        edge_z += [z0, z1, None]
    edge_trace = go.Scatter3d(
        x=edge_x, y=edge_y, z=edge_z,
        mode='lines', line=dict(width=2, color='gray'), hoverinfo='none'
    )

    # Build node trace with permanent labels
    node_trace = go.Scatter3d(
        x=node_xyz[:,0], y=node_xyz[:,1], z=node_xyz[:,2],
        mode='markers+text',
        marker=dict(size=6, color='skyblue', line=dict(width=0.5, color='darkblue')),
        text=[str(P) for P in pts],
        textposition='top center',
        textfont=dict(size=10, color='black'),
        hoverinfo='none'
    )

    # Create figure
    fig = go.Figure(data=[edge_trace, node_trace])
    fig.update_layout(
        title=f"3D Cayley Diagram of E: y²=x³+{a}x+{b} over F_{p}, G={G}",
        scene=dict(
            xaxis=dict(visible=False),
            yaxis=dict(visible=False),
            zaxis=dict(visible=False)
        ),
        margin=dict(l=0, r=0, t=40, b=0)
    )

    # Show interactive plot with zoom & pan
    fig.show()

if __name__ == '__main__':
    main()
