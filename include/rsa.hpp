// rsa.hpp
// Implementación de RSA usando NTL
// Autor: Leon Elliott Fuller
// Fecha: 2025-12-06

#ifndef RSA_HPP
#define RSA_HPP

#include "common.hpp"
#include "rng.hpp"
#include <NTL/ZZ.h>
#include <string>
#include <memory>

namespace crypto {

// ============================================================================
// ESTRUCTURAS RSA
// ============================================================================

/**
 * @brief Clave pública RSA
 * 
 * Consiste en:
 * - n: módulo (producto de dos primos p y q)
 * - e: exponente público (típicamente 65537)
 */
struct RSAPublicKey {
    BigInt n;  // Módulo
    BigInt e;  // Exponente público
    
    RSAPublicKey() = default;
    RSAPublicKey(const BigInt& n_, const BigInt& e_) : n(n_), e(e_) {}
    
    // Tamaño de la clave en bits
    long bit_size() const;
    
    // Serialización simple
    std::string to_string() const;
};

/**
 * @brief Clave privada RSA
 * 
 * Consiste en:
 * - n: módulo (igual que en la clave pública)
 * - d: exponente privado
 * - p, q: factores primos de n (opcionales, para CRT)
 * - dp, dq, qinv: valores precalculados para CRT (opcional)
 */
struct RSAPrivateKey {
    BigInt n;   // Módulo
    BigInt d;   // Exponente privado
    
    // Factores primos (para optimización CRT)
    BigInt p;
    BigInt q;
    
    // Valores CRT precalculados (opcionales)
    BigInt dp;   // d mod (p-1)
    BigInt dq;   // d mod (q-1)
    BigInt qinv; // q^-1 mod p
    
    bool has_crt_params;
    
    RSAPrivateKey() : has_crt_params(false) {}
    RSAPrivateKey(const BigInt& n_, const BigInt& d_) 
        : n(n_), d(d_), has_crt_params(false) {}
    
    // Tamaño de la clave en bits
    long bit_size() const;
    
    // Precalcular parámetros CRT
    void compute_crt_params();
    
    // Serialización simple
    std::string to_string() const;
};

/**
 * @brief Par de claves RSA (pública + privada)
 */
struct RSAKeyPair {
    RSAPublicKey public_key;
    RSAPrivateKey private_key;
    
    RSAKeyPair() = default;
    RSAKeyPair(const RSAPublicKey& pub, const RSAPrivateKey& priv)
        : public_key(pub), private_key(priv) {}
    
    long bit_size() const { return public_key.bit_size(); }
};

// ============================================================================
// CLASE RSA
// ============================================================================

/**
 * @brief Clase principal para operaciones RSA
 * 
 * Proporciona funciones estáticas para:
 * - Generación de claves
 * - Cifrado/descifrado
 * - Firma/verificación (opcional)
 * 
 * Ejemplo de uso:
 * @code
 *   NTLRNG rng(BigInt(0));  // Semilla fija para reproducibilidad
 *   
 *   // Generar par de claves de 2048 bits
 *   auto keypair = RSA::generate_key(rng, 2048);
 *   
 *   // Cifrar mensaje
 *   BigInt message(42);
 *   BigInt ciphertext = RSA::encrypt(message, keypair.public_key);
 *   
 *   // Descifrar
 *   BigInt decrypted = RSA::decrypt(ciphertext, keypair.private_key);
 * @endcode
 */
class RSA {
public:
    // ========================================================================
    // GENERACIÓN DE CLAVES
    // ========================================================================
    
    /**
     * @brief Genera un par de claves RSA
     * @param rng Generador de números aleatorios
     * @param bits Tamaño de la clave en bits (debe ser par)
     * @param e Exponente público (default: 65537)
     * @return Par de claves RSA
     * 
     * Proceso:
     * 1. Genera dos primos p y q de bits/2 bits cada uno
     * 2. Calcula n = p * q
     * 3. Calcula φ(n) = (p-1)(q-1)
     * 4. Calcula d = e^-1 mod φ(n)
     * 5. Precalcula parámetros CRT para optimización
     */
    static RSAKeyPair generate_key(
        RNG& rng, 
        int bits = DEFAULT_RSA_BITS,
        long e = DEFAULT_RSA_EXPONENT
    );
    
    // ========================================================================
    // CIFRADO Y DESCIFRADO
    // ========================================================================
    
    /**
     * @brief Cifra un mensaje usando la clave pública
     * @param message Mensaje a cifrar (debe ser < n)
     * @param public_key Clave pública
     * @return Texto cifrado c = m^e mod n
     * 
     * IMPORTANTE: Este es cifrado RSA "textbook", no seguro en la práctica.
     * Para uso real, se debe usar OAEP u otro esquema de padding.
     */
    static BigInt encrypt(const BigInt& message, const RSAPublicKey& public_key);
    
    /**
     * @brief Descifra un texto cifrado usando la clave privada
     * @param ciphertext Texto cifrado
     * @param private_key Clave privada
     * @param use_crt Usar optimización CRT si está disponible (default: true)
     * @return Mensaje original m = c^d mod n
     */
    static BigInt decrypt(
        const BigInt& ciphertext, 
        const RSAPrivateKey& private_key,
        bool use_crt = true
    );
    
    // ========================================================================
    // FIRMA Y VERIFICACIÓN (OPCIONAL)
    // ========================================================================
    
    /**
     * @brief Firma un mensaje (hash) usando la clave privada
     * @param message_hash Hash del mensaje a firmar
     * @param private_key Clave privada
     * @param use_crt Usar optimización CRT si está disponible
     * @return Firma s = h^d mod n
     */
    static BigInt sign(
        const BigInt& message_hash,
        const RSAPrivateKey& private_key,
        bool use_crt = true
    );
    
    /**
     * @brief Verifica una firma usando la clave pública
     * @param message_hash Hash del mensaje original
     * @param signature Firma a verificar
     * @param public_key Clave pública
     * @return true si la firma es válida, false en caso contrario
     */
    static bool verify(
        const BigInt& message_hash,
        const BigInt& signature,
        const RSAPublicKey& public_key
    );
    
    // ========================================================================
    // UTILIDADES
    // ========================================================================
    
    /**
     * @brief Valida que un mensaje pueda ser cifrado con una clave
     * @param message Mensaje a validar
     * @param n Módulo de la clave
     * @return true si 0 < message < n
     */
    static bool validate_message(const BigInt& message, const BigInt& n);

private:
    /**
     * @brief Genera un primo aleatorio de l bits, coprimo con e
     * @param rng Generador de números aleatorios
     * @param l Número de bits
     * @param e Exponente público (para verificar coprimality)
     * @return Primo de l bits coprimo con e
     */
    static BigInt generate_prime(RNG& rng, long l, const BigInt& e);
    
    /**
     * @brief Descifrado usando el Teorema Chino del Resto (CRT)
     * @param ciphertext Texto cifrado
     * @param private_key Clave privada con parámetros CRT
     * @return Mensaje descifrado
     * 
     * Usa la optimización CRT que es ~4x más rápida que el descifrado directo:
     * m1 = c^dp mod p
     * m2 = c^dq mod q
     * h = (m1 - m2) * qinv mod p
     * m = m2 + h * q
     */
    static BigInt decrypt_crt(
        const BigInt& ciphertext,
        const RSAPrivateKey& private_key
    );
};

} // namespace crypto

#endif // RSA_HPP