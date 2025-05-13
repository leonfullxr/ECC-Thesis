// rsa.cpp
#include "rsa.hpp"
#include <NTL/ZZ.h>
#include <NTL/BasicThreadPool.h>
#include <stdexcept>

using namespace crypto;
using namespace NTL;

namespace {
    // Genera un primo de 'bits' bits usando rng
    BigInt generate_prime(long bits, RNG& rng) {
        BigInt p;
        do {
            p = rng.get_random_bits(bits);
            SetBit(p, bits-1);
            if (!bit(p,0)) SetBit(p,0);
        } while (!ProbPrime(p, DEFAULT_PRIME_REPS));
        return p;
    }
}

RSAKeyPair RSA::generate_key(RNG& rng, int bits, long e_val) {
    long half = bits/2;
    BigInt p = generate_prime(half, rng);
    BigInt q;
    do { q = generate_prime(half, rng); } while (q == p);

    BigInt n   = p * q;
    BigInt phi = (p-1)*(q-1);
    BigInt e   = ZZ(e_val);
    if (GCD(e,phi) != 1) throw std::runtime_error("e no coprimo con phi(n)");
    BigInt d = InvMod(e, phi);
    return RSAKeyPair{n,e,d};
}

BigInt RSA::encrypt(const BigInt& m, const RSAKeyPair& kp) {
    if (m < 0 || m >= kp.n) throw std::invalid_argument("Mensaje fuera de rango");
    return PowerMod(m, kp.e, kp.n);
}

BigInt RSA::decrypt(const BigInt& c, const RSAKeyPair& kp) {
    if (c < 0 || c >= kp.n) throw std::invalid_argument("Cifrado fuera de rango");
    return PowerMod(c, kp.d, kp.n);
}