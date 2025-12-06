// rng.cpp
// Implementación del sistema de generación de números aleatorios
// Autor: Leon Elliott Fuller
// Fecha: 2025-01-06

#include "rng.hpp"
#include <NTL/ZZ.h>
#include <ctime>
#include <memory>

using namespace NTL;

namespace crypto {

// ============================================================================
// IMPLEMENTACIÓN NTLRNG
// ============================================================================

NTLRNG::NTLRNG(const BigInt& seed) 
    : seed_(seed), initialized_(false) {
    initialize_seed();
}

NTLRNG::NTLRNG() 
    : seed_(time(nullptr)), initialized_(false) {
    initialize_seed();
}

void NTLRNG::initialize_seed() {
    // Usar SetSeed de NTL para inicializar el generador
    // NTL hashea internamente la representación binaria de la semilla
    SetSeed(seed_);
    initialized_ = true;
}

void NTLRNG::set_seed(const BigInt& new_seed) {
    seed_ = new_seed;
    initialize_seed();
}

BigInt NTLRNG::random_bnd(const BigInt& n) {
    if (n <= 0) {
        return BigInt(0);
    }
    
    BigInt result;
    RandomBnd(result, n);
    return result;
}

BigInt NTLRNG::random_len(long l) {
    if (l <= 0) {
        return BigInt(0);
    }
    
    BigInt result;
    RandomLen(result, l);
    return result;
}

BigInt NTLRNG::random_bits(long l) {
    if (l <= 0) {
        return BigInt(0);
    }
    
    BigInt result;
    RandomBits(result, l);
    return result;
}

BigInt NTLRNG::get_seed() const {
    return seed_;
}

BigInt NTLRNG::random_prime(long l, long iterations) {
    if (l <= 1) {
        throw CryptoException("Prime length must be > 1 bit");
    }
    
    BigInt prime;
    
    // Generar número aleatorio de l bits
    RandomLen(prime, l);
    
    // Asegurar que sea impar (requisito para GenPrime)
    // SetBit(prime, 0) pone el bit 0 en 1, haciendo el número impar
    SetBit(prime, 0);
    
    // Generar primo usando GenPrime de NTL
    // GenPrime usa Miller-Rabin internamente
    GenPrime(prime, l, iterations);
    
    return prime;
}

// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================

std::unique_ptr<RNG> create_rng(const std::string& seed_mode, long fixed_value) {
    BigInt seed;
    
    if (seed_mode == "random") {
        seed = BigInt(time(nullptr));
    } else if (seed_mode == "fixed") {
        seed = BigInt(fixed_value);
    } else {
        throw CryptoException("Invalid seed mode: " + seed_mode + 
                            " (must be 'fixed' or 'random')");
    }
    
    return std::make_unique<NTLRNG>(seed);
}

} // namespace crypto