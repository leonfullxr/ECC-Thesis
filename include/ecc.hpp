// ecc.hpp
// Elliptic Curve Cryptography (ECC)
// 
// Autor: Leon Elliott Fuller
// Fecha: 2026-02-28

#ifndef ECC_HPP
#define ECC_HPP

#include "common.hpp"
#include "rng.hpp"
#include "sha256.hpp"
#include <NTL/ZZ.h>
#include <NTL/ZZ_p.h>
#include <string>
#include <memory>

namespace crypto {

using namespace NTL;

// ============================================================================
// CURVAS ELIPTICAS ESTANDAR
// ============================================================================

/**
 * @brief Tipo de curva eli­ptica
 * 
 * Curvas estandar soportadas:
 * - NIST P-256 (secp256r1): Estandar NIST, ampliamente usado
 * - NIST P-384: Alta seguridad
 * - secp256k1: Usado en Bitcoin/Ethereum
 * - CUSTOM: Curva definida por el usuario
 */
enum class CurveType {
    NIST_P256,      // NIST P-256 (secp256r1) - 256 bits
    NIST_P384,      // NIST P-384 - 384 bits
    SECP256K1,      // secp256k1 (Bitcoin) - 256 bits
    CUSTOM          // Curva personalizada
};

/**
 * @brief Parametros de una curva eli­ptica
 * 
 * Curva en forma de Weierstrass: yÂ² = xÂ³ + ax + b (mod p)
 * 
 * Parametros del dominio:
 * - p: Primo que define el campo finito Fp
 * - a, b: Coeficientes de la ecuacion de la curva
 * - G: Punto generador (base point)
 * - n: Orden del punto generador (numero de puntos)
 * - h: Cofactor (normalmente 1)
 */
struct CurveParams {
    BigInt p;           // Primo del campo (modulo)
    BigInt a;           // Coeficiente a
    BigInt b;           // Coeficiente b
    BigInt Gx;          // Coordenada x del generador
    BigInt Gy;          // Coordenada y del generador
    BigInt n;           // Orden del generador
    BigInt h;           // Cofactor
    
    std::string name;   // Nombre de la curva (para debugging)
    int bits;           // Tamai±o en bits (para referencia)
    
    /**
     * @brief Constructor por defecto
     */
    CurveParams() = default;
    
    /**
     * @brief Valida los parametros de la curva
     * @return true si los parametros son validos
     */
    bool validate() const;
    
    /**
     * @brief Imprime los parametros de la curva
     */
    void print() const;
};

/**
 * @brief Obtiene los parametros de una curva estandar
 * @param type Tipo de curva
 * @return Parametros de la curva
 */
CurveParams get_curve_params(CurveType type);

// ============================================================================
// PUNTO EN CURVA ELiPTICA
// ============================================================================

/**
 * @brief Representa un punto en una curva eli­ptica
 * 
 * Soporta:
 * - Punto en el infinito (punto de identidad)
 * - Coordenadas afines (x, y)
 * - Coordenadas proyectivas (X, Y, Z) para optimizacion (futuro)
 */
class ECPoint {
private:
    BigInt x_;
    BigInt y_;
    bool is_infinity_;      // true si es el punto en el infinito
    
    // Referencia a la curva (no la poseemos)
    const CurveParams* curve_;
    
public:
    /**
     * @brief Constructor para el punto en el infinito
     */
    ECPoint(const CurveParams* curve);
    
    /**
     * @brief Constructor con coordenadas afines
     * @param x Coordenada x
     * @param y Coordenada y
     * @param curve Parametros de la curva
     */
    ECPoint(const BigInt& x, const BigInt& y, const CurveParams* curve);
    
    // Getters
    const BigInt& x() const { return x_; }
    const BigInt& y() const { return y_; }
    bool is_infinity() const { return is_infinity_; }
    const CurveParams* curve() const { return curve_; }
    
    /**
     * @brief Verifica si el punto esta en la curva
     * @return true si el punto satisface la ecuacion de la curva
     */
    bool is_on_curve() const;
    
    /**
     * @brief Igualdad de puntos
     */
    bool operator==(const ECPoint& other) const;
    bool operator!=(const ECPoint& other) const { return !(*this == other); }
    
    /**
     * @brief Imprime el punto (para debugging)
     */
    void print() const;
};

// ============================================================================
// OPERACIONES EN CURVA ELiPTICA
// ============================================================================

/**
 * @brief Suma de puntos en curva eli­ptica
 * 
 * Implementa la ley de grupo de la curva eli­ptica:
 * P + Q = R
 * 
 * Casos especiales:
 * - P + O = P (O = punto infinito)
 * - P + (-P) = O
 * - P + P = 2P (duplicacion)
 * 
 * @param P Primer punto
 * @param Q Segundo punto
 * @return Suma P + Q
 */
ECPoint ec_add(const ECPoint& P, const ECPoint& Q);

/**
 * @brief Duplicacion de punto (caso especial de suma)
 * @param P Punto a duplicar
 * @return 2P
 */
ECPoint ec_double(const ECPoint& P);

/**
 * @brief Negacion de punto
 * @param P Punto
 * @return -P (reflexion sobre eje x)
 */
ECPoint ec_negate(const ECPoint& P);

/**
 * @brief Multiplicacion escalar (operacion fundamental)
 * 
 * Calcula k*P = P + P + ... + P (k veces)
 * 
 * Usa algoritmo "double-and-add" para eficiencia:
 * Complejidad: O(log k)
 * 
 * @param k Escalar (numero de veces)
 * @param P Punto base
 * @return k*P
 */
ECPoint ec_scalar_mult(const BigInt& k, const ECPoint& P);

// ============================================================================
// CLAVES ECC
// ============================================================================

/**
 * @brief Par de claves ECC
 */
struct ECKeyPair {
    BigInt private_key;     // Clave privada: numero aleatorio en [1, n-1]
    ECPoint public_key;     // Clave publica: Q = d*G
    
    const CurveParams* curve;  // Curva utilizada
    
    /**
     * @brief Imprime las claves (oculta la privada)
     */
    void print(bool show_private = false) const;
};

/**
 * @brief Genera un par de claves ECC
 * 
 * Proceso:
 * 1. Genera numero aleatorio d en [1, n-1] (clave privada)
 * 2. Calcula Q = d*G (clave publica)
 * 
 * @param curve Parametros de la curva
 * @param rng Generador de numeros aleatorios
 * @return Par de claves
 */
ECKeyPair generate_keypair(const CurveParams& curve, RNG& rng);

// ============================================================================
// DIFFIE-HELLMAN EN CURVAS ELIPTICAS (ECDH)
// ============================================================================

/**
 * @brief Calcula secreto compartido usando ECDH
 * 
 * Alice tiene (da, Qa), Bob tiene (db, Qb)
 * 
 * Alice calcula: S = da * Qb
 * Bob calcula:   S = db * Qa
 * Ambos obtienen el mismo punto S (secreto compartido)
 * 
 * @param private_key Clave privada propia
 * @param public_key Clave publica del otro
 * @return Punto compartido (usar x como secreto)
 */
ECPoint ecdh_shared_secret(const BigInt& private_key, 
                           const ECPoint& public_key);

/**
 * @brief Deriva clave simetrica del secreto ECDH
 * @param shared_point Punto compartido
 * @param key_bits Tamaño de clave deseado (128, 192, 256)
 * @return Clave simetrica derivada
 */
BigInt ecdh_derive_key(const ECPoint& shared_point, int key_bits = 256);

// ============================================================================
// ECDSA - FIRMA DIGITAL EN CURVAS ELIPTICAS
// ============================================================================

/**
 * @brief Firma ECDSA
 * 
 * Una firma ECDSA consiste en un par (r, s) donde:
 * - r: coordenada x del punto k*G reducida módulo n
 * - s: prueba criptográfica calculada con la clave privada
 */
struct ECDSASignature {
    BigInt r;   // Componente r de la firma
    BigInt s;   // Componente s de la firma
    
    /**
     * @brief Verifica si la firma tiene formato válido
     * @param n Orden del grupo de la curva
     * @return true si r,s ∈ [1, n-1]
     */
    bool is_valid_format(const BigInt& n) const;
    
    /**
     * @brief Imprime la firma
     */
    void print() const;
};

/**
 * @brief Firma un mensaje usando ECDSA
 * 
 * Algoritmo (FIPS 186-4, Sección 6.4):
 * 1. e = SHA-256(message)
 * 2. z = bits más significativos de e (truncado a bit_length(n))
 * 3. Seleccionar k aleatorio en [1, n-1]
 * 4. (x1, y1) = k * G
 * 5. r = x1 mod n  (si r == 0, repetir desde 3)
 * 6. s = k⁻¹ · (z + r·d) mod n  (si s == 0, repetir desde 3)
 * 7. Firma = (r, s)
 * 
 * @param message Mensaje a firmar (string)
 * @param private_key Clave privada (escalar d)
 * @param curve Parámetros de la curva
 * @param rng Generador de números aleatorios
 * @return Firma ECDSA (r, s)
 */
ECDSASignature ecdsa_sign(const std::string& message,
                          const BigInt& private_key,
                          const CurveParams& curve,
                          RNG& rng);

/**
 * @brief Firma un hash (BigInt) directamente usando ECDSA
 * 
 * @param hash_value Hash del mensaje como BigInt
 * @param private_key Clave privada (escalar d)
 * @param curve Parámetros de la curva
 * @param rng Generador de números aleatorios
 * @return Firma ECDSA (r, s)
 */
ECDSASignature ecdsa_sign_hash(const BigInt& hash_value,
                               const BigInt& private_key,
                               const CurveParams& curve,
                               RNG& rng);

/**
 * @brief Verifica una firma ECDSA
 * 
 * Algoritmo (FIPS 186-4, Sección 6.4):
 * 1. Verificar r, s ∈ [1, n-1]
 * 2. e = SHA-256(message)
 * 3. z = bits más significativos de e
 * 4. w = s⁻¹ mod n
 * 5. u1 = z·w mod n
 * 6. u2 = r·w mod n
 * 7. (x1, y1) = u1·G + u2·Q
 * 8. Firma válida si r ≡ x1 (mod n)
 * 
 * @param message Mensaje original
 * @param signature Firma a verificar
 * @param public_key Clave pública (punto Q)
 * @param curve Parámetros de la curva
 * @return true si la firma es válida
 */
bool ecdsa_verify(const std::string& message,
                  const ECDSASignature& signature,
                  const ECPoint& public_key,
                  const CurveParams& curve);

/**
 * @brief Verifica una firma ECDSA a partir de un hash precalculado
 */
bool ecdsa_verify_hash(const BigInt& hash_value,
                       const ECDSASignature& signature,
                       const ECPoint& public_key,
                       const CurveParams& curve);

/**
 * @brief Trunca un hash al tamaño del orden de la curva
 * 
 * Si el hash tiene mas bits que n, se truncan los bits
 * mas significativos según FIPS 186-4.
 * 
 * @param hash Hash como BigInt
 * @param n Orden del grupo
 * @return Hash truncado
 */
BigInt truncate_hash(const BigInt& hash, const BigInt& n);

// ============================================================================
// UTILIDADES
// ============================================================================

/**
 * @brief Convierte tipo de curva a string
 */
std::string curve_type_to_string(CurveType type);

/**
 * @brief Obtiene nivel de seguridad equivalente a RSA
 * @param curve_bits Bits de la curva ECC
 * @return Bits equivalentes de RSA
 */
int ecc_to_rsa_security(int curve_bits);

/**
 * @brief Obtiene curva ECC equivalente a clave RSA
 * @param rsa_bits Bits de la clave RSA
 * @return Tipo de curva recomendada
 */
CurveType rsa_to_ecc_curve(int rsa_bits);

} // namespace crypto

#endif // ECC_HPP