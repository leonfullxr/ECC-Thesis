// ecc.cpp
// Implementación stub de ECC
// Autor: Leon Elliott Fuller
// Fecha: 2025-12-06

#include "ecc.hpp"
#include <sstream>

namespace crypto {

// ============================================================================
// IMPLEMENTACIÓN STUB DE ECC
// ============================================================================

std::string ECCPublicKey::to_string() const {
    std::ostringstream oss;
    oss << "ECC Public Key (" << curve_name << ")\n"
        << "  x = " << x << "\n"
        << "  y = " << y;
    return oss.str();
}

std::string ECCPrivateKey::to_string() const {
    std::ostringstream oss;
    oss << "ECC Private Key (" << curve_name << ")\n"
        << "  d = " << d;
    return oss.str();
}

ECCKeyPair ECC::generate_key(RNG& rng, const std::string& curve_name) {
    // STUB: Generar valores dummy para permitir compilación
    // TODO: Implementar generación real de claves ECC
    
    BigInt d = rng.random_bits(256);
    BigInt x = rng.random_bits(256);
    BigInt y = rng.random_bits(256);
    
    ECCPublicKey pub(x, y, curve_name);
    ECCPrivateKey priv(d, curve_name);
    
    return ECCKeyPair(pub, priv);
}

BigInt ECC::sign(const BigInt& message_hash, const ECCPrivateKey& private_key) {
    // STUB: Retornar valor dummy
    // TODO: Implementar ECDSA
    return message_hash;
}

bool ECC::verify(const BigInt& message_hash, const BigInt& signature, const ECCPublicKey& public_key) {
    // STUB: Siempre retorna true
    // TODO: Implementar verificación ECDSA
    return true;
}

} // namespace crypto