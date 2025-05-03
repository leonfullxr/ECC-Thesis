#!/usr/bin/env python3

import argparse
import numpy as np
import networkx as nx
import plotly.graph_objects as go

# Elliptic-curve arithmetic over F_p

def inv_mod(x, p):
    return pow(x, p-2, p)

def point_add(P, Q, a, p):
    if P == 'O': return Q
    if Q == 'O': return P
    x1, y1 = P
    x2, y2 = Q
    if x1 == x2 and (y1 + y2) % p == 0:
        return 'O'
    if P != Q:
        m = ((y2 - y1) * inv_mod(x2 - x1, p)) % p
    else:
        m = ((3*x1*x1 + a) * inv_mod(2*y1, p)) % p
    x3 = (m*m - x1 - x2) % p
    y3 = (m*(x1 - x3) - y1) % p
    return (x3, y3)

# Collect all points on the curve

def all_points(a, b, p):
    pts = ['O']
    for x in range(p):
        rhs = (x**3 + a*x + b) % p
        for y in range(p):
            if (y*y) % p == rhs:
                pts.append((x, y))
    return pts

# Map point (x,y) to torus coordinates

def torus_embed(P, p, R=3.0, r=1.0):
    if P == 'O':
        return np.array([0.0, 0.0, R + r])
    x, y = P
    theta = 2*np.pi * x / p
    phi = 2*np.pi * y / p
    X = (R + r * np.cos(theta)) * np.cos(phi)
    Y = (R + r * np.cos(theta)) * np.sin(phi)
    Z = r * np.sin(theta)
    return np.array([X, Y, Z])


def main():
    parser = argparse.ArgumentParser(
        description="Interactive 3D Torus Cayley Diagram with zoom & pan"
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

    # Build graph
    C = nx.DiGraph()
    C.add_nodes_from(pts)
    for P in pts:
        Q = point_add(P, G, a, p)
        C.add_edge(P, Q)

    # Embed on torus
    pos = {P: torus_embed(P, p) for P in pts}

    # Node trace
    node_xyz = np.array([pos[P] for P in pts])
    node_trace = go.Scatter3d(
        x=node_xyz[:,0], y=node_xyz[:,1], z=node_xyz[:,2],
        mode='markers', marker=dict(size=4, color='skyblue', line=dict(width=0.5, color='darkblue')),
        hovertext=[str(P) for P in pts]
    )

    # Edge traces
    edge_x, edge_y, edge_z = [], [], []
    for u, v in C.edges():
        x0, y0, z0 = pos[u]
        x1, y1, z1 = pos[v]
        edge_x += [x0, x1, None]
        edge_y += [y0, y1, None]
        edge_z += [z0, z1, None]
    edge_trace = go.Scatter3d(
        x=edge_x, y=edge_y, z=edge_z,
        mode='lines', line=dict(width=1, color='gray'), hoverinfo='none'
    )

    # Build figure
    fig = go.Figure(data=[edge_trace, node_trace])
    fig.update_layout(
        title=f"Torus Cayley Diagram: y² = x³ + {a}x + {b} over F_{p}, G={G}",
        scene=dict(
            xaxis=dict(visible=False),
            yaxis=dict(visible=False),
            zaxis=dict(visible=False)
        ),
        margin=dict(l=0, r=0, t=50, b=0)
    )

    # Show interactive plot
    fig.show()

if __name__ == '__main__':
    main()
