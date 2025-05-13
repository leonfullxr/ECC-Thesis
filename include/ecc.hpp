// ecc.hpp
#ifndef CRYPTO_ECC_HPP
#define CRYPTO_ECC_HPP

#include "common.hpp"
#include "rng.hpp"
#include <string>

namespace crypto {

    // Curva por defecto
    constexpr char DEFAULT_CURVE[] = "secp256k1";

    /**
     * Estructura para el par de claves ECC.
     */
    struct ECCKeyPair { BigInt x, y, priv; };

    /**
     * Clase para generación de claves ECC.
     */
    class ECC {
    public:
        // Genera un par de claves para la curva indicada
        static ECCKeyPair generate_key(RNG& rng,
                                       const std::string& curve = DEFAULT_CURVE) {
            // Implementación pendiente
            return ECCKeyPair{};
        }
    };

}

#endif // CRYPTO_ECC_HPP