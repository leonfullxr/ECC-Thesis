#!/usr/bin/env python3
"""
Plot the real Weierstrass surface y^2 = x^3 + a*x + b as z = x^3 + a*x + b - y^2
"""
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import argparse


def main():
    parser = argparse.ArgumentParser(description="Visualize the real Weierstrass surface y^2 = x^3 + a*x + b in 3D")
    parser.add_argument('--a', type=float, default=1.0, help='Coefficient a (default: 1.0)')
    parser.add_argument('--b', type=float, default=1.0, help='Coefficient b (default: 1.0)')
    parser.add_argument('--xrange', type=float, nargs=2, default=[-3, 3],
                        help='Range for x-axis (default: -3 to 3)')
    parser.add_argument('--yrange', type=float, nargs=2, default=[-3, 3],
                        help='Range for y-axis (default: -3 to 3)')
    parser.add_argument('--nx', type=int, default=200, help='Number of points along x (default: 200)')
    parser.add_argument('--ny', type=int, default=200, help='Number of points along y (default: 200)')
    args = parser.parse_args()

    a, b = args.a, args.b
    x_min, x_max = args.xrange
    y_min, y_max = args.yrange

    # create grid
    x = np.linspace(x_min, x_max, args.nx)
    y = np.linspace(y_min, y_max, args.ny)
    X, Y = np.meshgrid(x, y)
    Z = X**3 + a*X + b - Y**2

    # plot
    fig = plt.figure(figsize=(8, 6))
    ax = fig.add_subplot(111, projection='3d')

    # surface patch
    surf = ax.plot_surface(X, Y, Z, cmap='viridis', edgecolor='none', alpha=0.9)
    fig.colorbar(surf, ax=ax, shrink=0.5, aspect=10, label='Z = x^3 + a*x + b - y^2')

    # zero-level contour (the actual curve) in black
    ax.contour3D(X, Y, Z, levels=[0], colors='k', linewidths=2)

    ax.set_xlabel('x')
    ax.set_ylabel('y')
    ax.set_zlabel('z')
    ax.set_title(f"Weierstrass surface y^2 = x^3 + {a}x + {b}")
    plt.tight_layout()
    plt.show()

if __name__ == '__main__':
    main()
