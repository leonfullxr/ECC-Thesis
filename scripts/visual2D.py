#!/usr/bin/env python3
import sys
import numpy as np
import matplotlib.pyplot as plt

def points_on_curve(a, b, p):
    """
    Return all (x, y) in Z/pZ × Z/pZ with y^2 ≡ x^3 + a x + b (mod p).
    """
    pts = []
    for x in range(p):
        rhs = (x**3 + a*x + b) % p
        # precompute all possible squares mod p
        # could also invert the loop order for speed on large p
        for y in range(p):
            if (y*y) % p == rhs:
                pts.append((x, y))
    return pts

def main():
    # default parameters for secp256k1 curve https://www.secg.org/sec2-v2.pdf
    p = 250
    a = 0
    b = 7

    # override p if given on command line
    if len(sys.argv) > 1:
        try:
            p = int(sys.argv[1])
            if p < 2:
                raise ValueError
        except ValueError:
            print("Usage: python3 visual2D.py [modulus>1]")
            sys.exit(1)

    pts = points_on_curve(a, b, p)
    if not pts:
        print(f"No points found on y^2 = x^3 + {a}x + {b} over Z/{p}Z")
        return

    xs, ys = zip(*pts)

    plt.figure(figsize=(6,6))
    plt.scatter(xs, ys, s=20, edgecolor='k')
    plt.title(f"Points on $y^2 = x^3 + {a}x + {b}$ over $\\mathbb{{Z}}/{p}\\mathbb{{Z}}$")
    plt.xlabel("x")
    plt.ylabel("y")
    plt.xlim(-1, p)
    plt.ylim(-1, p)
    plt.xticks(np.arange(0, p+1, max(1, p//10)))
    plt.yticks(np.arange(0, p+1, max(1, p//10)))
    plt.gca().set_aspect('equal', 'box')
    plt.grid(True, linestyle=':', alpha=0.5)
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    main()
