// rsa.cpp
// Implementación de RSA usando NTL
// Autor: Leon Elliott Fuller
// Fecha: 2025-12-06

#include "rsa.hpp"
#include <NTL/ZZ.h>
#include <sstream>
#include <iomanip>

using namespace NTL;

namespace crypto {

// ============================================================================
// IMPLEMENTACIÓN DE RSAPublicKey
// ============================================================================

long RSAPublicKey::bit_size() const {
    return NumBits(n);
}

std::string RSAPublicKey::to_string() const {
    std::ostringstream oss;
    oss << "RSA Public Key (" << bit_size() << " bits)\n"
        << "  n = " << n << "\n"
        << "  e = " << e;
    return oss.str();
}

// ============================================================================
// IMPLEMENTACIÓN DE RSAPrivateKey
// ============================================================================

long RSAPrivateKey::bit_size() const {
    return NumBits(n);
}

void RSAPrivateKey::compute_crt_params() {
    if (p == 0 || q == 0) {
        has_crt_params = false;
        return;
    }
    
    // dp = d mod (p-1)
    dp = d % (p - 1);
    
    // dq = d mod (q-1)
    dq = d % (q - 1);
    
    // qinv = q^-1 mod p
    qinv = InvMod(q, p);
    
    has_crt_params = true;
}

std::string RSAPrivateKey::to_string() const {
    std::ostringstream oss;
    oss << "RSA Private Key (" << bit_size() << " bits)\n"
        << "  n = " << n << "\n"
        << "  d = " << d << "\n";
    
    if (has_crt_params) {
        oss << "  p = " << p << "\n"
            << "  q = " << q << "\n"
            << "  CRT params available";
    }
    
    return oss.str();
}

// ============================================================================
// IMPLEMENTACIÓN DE RSA
// ============================================================================

BigInt RSA::generate_prime(RNG& rng, long l, const BigInt& e) {
    // Generar primo de l bits que sea coprimo con e
    BigInt prime;
    BigInt gcd_result;
    
    do {
        // Generar primo candidato
        prime = rng.random_prime(l);
        
        // Verificar que gcd(p-1, e) = 1
        GCD(gcd_result, prime - 1, e);
        
    } while (gcd_result != 1);
    
    return prime;
}

RSAKeyPair RSA::generate_key(RNG& rng, int bits, long e) {
    // Validar parámetros
    validate_key_size(bits, MIN_RSA_BITS, MAX_RSA_BITS);
    
    if (bits % 2 != 0) {
        throw CryptoException("RSA key size must be even");
    }
    
    if (e <= 1 || e % 2 == 0) {
        throw CryptoException("RSA public exponent must be odd and > 1");
    }
    
    // Tamaño de cada primo (n = p * q, con p y q de bits/2 bits)
    long prime_bits = bits / 2;
    
    BigInt e_bigint(e);
    
    // Generar primer primo p
    BigInt p = generate_prime(rng, prime_bits, e_bigint);
    
    // Generar segundo primo q (asegurar q != p)
    BigInt q;
    do {
        q = generate_prime(rng, prime_bits, e_bigint);
    } while (q == p);
    
    // Asegurar p > q (convención)
    if (p < q) {
        BigInt temp = p;
        p = q;
        q = temp;
    }
    
    // Calcular n = p * q
    BigInt n = p * q;
    
    // Calcular φ(n) = (p-1)(q-1)
    BigInt phi = (p - 1) * (q - 1);
    
    // Calcular d = e^-1 mod φ(n)
    BigInt d = InvMod(e_bigint, phi);
    
    // Crear claves
    RSAPublicKey public_key(n, e_bigint);
    RSAPrivateKey private_key(n, d);
    
    // Guardar factores primos para CRT
    private_key.p = p;
    private_key.q = q;
    
    // Precalcular parámetros CRT
    private_key.compute_crt_params();
    
    return RSAKeyPair(public_key, private_key);
}

BigInt RSA::encrypt(const BigInt& message, const RSAPublicKey& public_key) {
    // Validar mensaje
    if (!validate_message(message, public_key.n)) {
        throw CryptoException("Message out of range for encryption");
    }
    
    // Cifrado RSA: c = m^e mod n
    BigInt ciphertext = PowerMod(message, public_key.e, public_key.n);
    
    return ciphertext;
}

BigInt RSA::decrypt(const BigInt& ciphertext, const RSAPrivateKey& private_key, bool use_crt) {
    // Validar texto cifrado
    if (!validate_message(ciphertext, private_key.n)) {
        throw CryptoException("Ciphertext out of range for decryption");
    }
    
    // Usar CRT si está disponible y se solicita
    if (use_crt && private_key.has_crt_params) {
        return decrypt_crt(ciphertext, private_key);
    }
    
    // Descifrado RSA directo: m = c^d mod n
    BigInt message = PowerMod(ciphertext, private_key.d, private_key.n);
    
    return message;
}

BigInt RSA::decrypt_crt(const BigInt& ciphertext, const RSAPrivateKey& private_key) {
    if (!private_key.has_crt_params) {
        throw CryptoException("CRT parameters not available");
    }
    
    // Reducir ciphertext módulo p y q primero
    // m1 = (c mod p)^dp mod p
    BigInt m1 = PowerMod(ciphertext % private_key.p, private_key.dp, private_key.p);
    
    // m2 = (c mod q)^dq mod q
    BigInt m2 = PowerMod(ciphertext % private_key.q, private_key.dq, private_key.q);
    
    // h = (m1 - m2) * qinv mod p
    BigInt h = ((m1 - m2) * private_key.qinv) % private_key.p;
    
    // Asegurar que h sea positivo
    if (h < 0) {
        h += private_key.p;
    }
    
    // m = m2 + h * q
    BigInt message = m2 + h * private_key.q;
    
    return message;
}

BigInt RSA::sign(const BigInt& message_hash, const RSAPrivateKey& private_key, bool use_crt) {
    // La firma es equivalente al descifrado: s = h^d mod n
    return decrypt(message_hash, private_key, use_crt);
}

bool RSA::verify(const BigInt& message_hash, const BigInt& signature, const RSAPublicKey& public_key) {
    // Verificar firma: h' = s^e mod n
    BigInt computed_hash = encrypt(signature, public_key);
    
    // Comparar con el hash original
    return computed_hash == message_hash;
}

bool RSA::validate_message(const BigInt& message, const BigInt& n) {
    return message > 0 && message < n;
}

} // namespace crypto