// ecc.hpp
// Implementación de ECC (Elliptic Curve Cryptography)
// Autor: Leon Elliott Fuller
// Fecha: 2025-12-06
// NOTA: Esta es una implementación stub para permitir compilación
//       La implementación completa se desarrollará posteriormente

#ifndef ECC_HPP
#define ECC_HPP

#include "common.hpp"
#include "rng.hpp"
#include <string>

namespace crypto {

// ============================================================================
// ESTRUCTURAS ECC (STUB)
// ============================================================================

/**
 * @brief Clave pública ECC
 */
struct ECCPublicKey {
    BigInt x;
    BigInt y;
    std::string curve_name;
    
    ECCPublicKey() = default;
    ECCPublicKey(const BigInt& x_, const BigInt& y_, const std::string& curve)
        : x(x_), y(y_), curve_name(curve) {}
    
    std::string to_string() const;
};

/**
 * @brief Clave privada ECC
 */
struct ECCPrivateKey {
    BigInt d;
    std::string curve_name;
    
    ECCPrivateKey() = default;
    ECCPrivateKey(const BigInt& d_, const std::string& curve)
        : d(d_), curve_name(curve) {}
    
    std::string to_string() const;
};

/**
 * @brief Par de claves ECC
 */
struct ECCKeyPair {
    ECCPublicKey public_key;
    ECCPrivateKey private_key;
    
    ECCKeyPair() = default;
    ECCKeyPair(const ECCPublicKey& pub, const ECCPrivateKey& priv)
        : public_key(pub), private_key(priv) {}
};

// ============================================================================
// CLASE ECC (STUB)
// ============================================================================

/**
 * @brief Clase principal para operaciones ECC
 * 
 * NOTA: Esta es una implementación stub para permitir compilación.
 *       La implementación completa se desarrollará posteriormente.
 */
class ECC {
public:
    /**
     * @brief Genera un par de claves ECC (stub)
     * @param rng Generador de números aleatorios
     * @param curve_name Nombre de la curva (default: secp256k1)
     * @return Par de claves ECC
     */
    static ECCKeyPair generate_key(RNG& rng, const std::string& curve_name = DEFAULT_CURVE);
    
    /**
     * @brief Firma un mensaje usando ECDSA (stub)
     */
    static BigInt sign(const BigInt& message_hash, const ECCPrivateKey& private_key);
    
    /**
     * @brief Verifica una firma ECDSA (stub)
     */
    static bool verify(const BigInt& message_hash, const BigInt& signature, const ECCPublicKey& public_key);
};

} // namespace crypto

#endif // ECC_HPP