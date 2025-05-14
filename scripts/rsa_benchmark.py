#!/usr/bin/env python3
import argparse
import random
import time
import sys

def is_prime(n, k=40):
    """Miller–Rabin primality test."""
    if n < 2:
        return False
    # pequeños primos para cribado rápido
    small_primes = [2,3,5,7,11,13,17,19,23,29]
    for p in small_primes:
        if n == p:
            return True
        if n % p == 0:
            return False

    # escribe n-1 = d * 2^s
    s = 0
    d = n - 1
    while d & 1 == 0:
        d >>= 1
        s += 1

    def try_composite(a):
        x = pow(a, d, n)
        if x == 1 or x == n - 1:
            return False
        for _ in range(s - 1):
            x = (x * x) % n
            if x == n - 1:
                return False
        return True  # n es compuesto

    for _ in range(k):
        a = random.randrange(2, n - 1)
        if try_composite(a):
            return False

    return True  # probablemente primo

def gen_prime(bits):
    """Genera un primo de 'bits' bits."""
    while True:
        # generamos un candidato impar de longitud exacta
        p = random.getrandbits(bits) | (1 << (bits - 1)) | 1
        if is_prime(p):
            return p

def gen_rsa_keypair(keysize):
    """Genera (n, e, d) donde n=p*q, e=65537, d = e^{-1} mod φ."""
    half = keysize // 2
    p = gen_prime(half)
    q = gen_prime(half)
    # aseguramos p != q
    while q == p:
        q = gen_prime(half)
    n = p * q
    phi = (p - 1) * (q - 1)
    e = 65537
    # inverso modular en Python 3.8+
    d = pow(e, -1, phi)
    return n, e, d

def main():
    parser = argparse.ArgumentParser(
        description="Benchmark de generación de claves RSA en Python puro"
    )
    parser.add_argument(
        "-k", "--keysize", type=int, required=True,
        help="Tamaño de la clave RSA en bits (e.g. 512, 1024, 2048)"
    )
    parser.add_argument(
        "-i", "--iterations", type=int, default=1,
        help="Número de veces que repetir la generación (por defecto: 1)"
    )
    parser.add_argument(
        "-s", "--seed", type=int, default=42,
        help="Semilla para el generador de números aleatorios (por defecto: 42)"
    )
    args = parser.parse_args()

    random.seed(args.seed)
    times_ms = []

    print(f"RSA benchmark: keysize={args.keysize} bits, iterations={args.iterations}, seed={args.seed}")
    for it in range(1, args.iterations + 1):
        t0 = time.perf_counter()
        n, e, d = gen_rsa_keypair(args.keysize)
        t1 = time.perf_counter()
        elapsed_ms = (t1 - t0) * 1000
        times_ms.append(elapsed_ms)
        print(f"  Iteración {it:2d}: {elapsed_ms:.3f} ms")

    avg = sum(times_ms) / len(times_ms)
    print(f"\nTiempo medio de generación de clave: {avg:.3f} ms")

if __name__ == "__main__":
    main()
