// common.hpp
// Definiciones comunes para el proyecto de benchmarking RSA vs ECC
// Autor: Leon Elliott Fuller
// Fecha: 2025-12-06

#ifndef COMMON_HPP
#define COMMON_HPP

#include <NTL/ZZ.h>
#include <string>
#include <stdexcept>

namespace crypto {

// ============================================================================
// ALIAS DE TIPOS
// ============================================================================

using BigInt = NTL::ZZ;

// ============================================================================
// CONSTANTES GLOBALES
// ============================================================================

// Constantes para RSA
constexpr int DEFAULT_RSA_BITS = 2048;
constexpr int MIN_RSA_BITS = 512;
constexpr int MAX_RSA_BITS = 8192;

// Valores comunes para tamaños de clave RSA
constexpr int RSA_1024 = 1024;
constexpr int RSA_2048 = 2048;
constexpr int RSA_3072 = 3072;
constexpr int RSA_4096 = 4096;

// Exponente público por defecto para RSA (65537 es el estándar)
constexpr long DEFAULT_RSA_EXPONENT = 65537;

// Constantes para ECC
const std::string DEFAULT_CURVE = "secp256k1";

// Número de iteraciones de Miller-Rabin para test de primalidad
// 40 iteraciones dan una probabilidad de error < 2^-80
constexpr long MILLER_RABIN_ITERATIONS = 40;

// ============================================================================
// ESTRUCTURAS COMUNES
// ============================================================================

// Excepción personalizada para errores criptográficos
class CryptoException : public std::runtime_error {
public:
    explicit CryptoException(const std::string& msg) 
        : std::runtime_error(msg) {}
};

// ============================================================================
// UTILIDADES
// ============================================================================

// Validar que el tamaño de bits esté en un rango válido
inline void validate_key_size(int bits, int min_bits, int max_bits) {
    if (bits < min_bits || bits > max_bits) {
        throw CryptoException(
            "Key size " + std::to_string(bits) + 
            " bits out of range [" + std::to_string(min_bits) + 
            ", " + std::to_string(max_bits) + "]"
        );
    }
}

} // namespace crypto

#endif // COMMON_HPP