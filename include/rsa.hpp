// rsa.hpp
#ifndef CRYPTO_RSA_HPP
#define CRYPTO_RSA_HPP

#include "common.hpp"
#include "rng.hpp"
#include <vector>

namespace crypto {

    // Parámetros por defecto para RSA
    constexpr int    DEFAULT_RSA_BITS   = 2048;
    constexpr long   DEFAULT_RSA_E      = 65537;
    constexpr long   DEFAULT_PRIME_REPS = 25;

    /**
     * Estructura que contiene el par de claves RSA.
     */
    struct RSAKeyPair {
        BigInt n;  // Módulo
        BigInt e;  // Exponente público
        BigInt d;  // Exponente privado
    };

    /**
     * Clase para generación de claves, cifrado y descifrado RSA.
     */
    class RSA {
    public:
        // Genera un par de claves RSA
        static RSAKeyPair generate_key(RNG& rng,
                                      int bits = DEFAULT_RSA_BITS,
                                      long e_val = DEFAULT_RSA_E);

        // Cifra mensaje m (BigInt) con la clave pública
        static BigInt encrypt(const BigInt& m, const RSAKeyPair& kp);

        // Descifra ciphertext c con la clave privada
        static BigInt decrypt(const BigInt& c, const RSAKeyPair& kp);
    };

}

#endif // CRYPTO_RSA_HPP