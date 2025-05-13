// rng.hpp
#ifndef CRYPTO_RNG_HPP
#define CRYPTO_RNG_HPP

#include "common.hpp"
#include <NTL/ZZ.h>


namespace crypto {

    /**
     * Interfaz para generadores de números aleatorios de precisión arbitraria.
     */
    class RNG {
    public:
        /**
         * Devuelve un entero pseudoaleatorio de 'bits' longitud (0..2^bits-1).
         */
        virtual BigInt get_random_bits(long bits) = 0;

        /**
         * (Re)semilla el generador con un valor concreto para reproducibilidad.
         */
        virtual void reseed(const BigInt& seed) = 0;

        virtual ~RNG() = default;
    };

    /**
     * Implementación de RNG sobre el PRG interno de NTL.
     */
    class NTLRNG : public RNG {
    public:
        // Constructor: siembra con semilla dada (por defecto 0)
        explicit NTLRNG(const BigInt& seed = BigInt(0)) { reseed(seed); }
        
        // Obtiene un entero pseudoaleatorio de longitud 'bits'
        BigInt get_random_bits(long bits) override { return NTL::RandomBits_ZZ(bits); }

        // Establece la semilla del PRG interno
        void reseed(const BigInt& seed) override { NTL::SetSeed(seed); }
    };

}

#endif // CRYPTO_RNG_HPP