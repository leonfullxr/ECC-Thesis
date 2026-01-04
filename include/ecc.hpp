// ecc.hpp
// Elliptic Curve Cryptography (ECC)
// Implementación limpia, escalable, sin números mágicos
// 
// Autor: Leon Elliott Fuller
// Fecha: 2026-01-04

#ifndef ECC_HPP
#define ECC_HPP

#include "common.hpp"
#include "rng.hpp"
#include <NTL/ZZ.h>
#include <NTL/ZZ_p.h>
#include <string>
#include <memory>

namespace crypto {

using namespace NTL;

// ============================================================================
// CURVAS ELÍPTICAS ESTÁNDAR
// ============================================================================

/**
 * @brief Tipo de curva elíptica
 * 
 * Curvas estándar soportadas:
 * - NIST P-256 (secp256r1): Estándar NIST, ampliamente usado
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
 * @brief Parámetros de una curva elíptica
 * 
 * Curva en forma de Weierstrass: y² = x³ + ax + b (mod p)
 * 
 * Parámetros del dominio:
 * - p: Primo que define el campo finito Fp
 * - a, b: Coeficientes de la ecuación de la curva
 * - G: Punto generador (base point)
 * - n: Orden del punto generador (número de puntos)
 * - h: Cofactor (normalmente 1)
 */
struct CurveParams {
    BigInt p;           // Primo del campo (módulo)
    BigInt a;           // Coeficiente a
    BigInt b;           // Coeficiente b
    BigInt Gx;          // Coordenada x del generador
    BigInt Gy;          // Coordenada y del generador
    BigInt n;           // Orden del generador
    BigInt h;           // Cofactor
    
    std::string name;   // Nombre de la curva (para debugging)
    int bits;           // Tamaño en bits (para referencia)
    
    /**
     * @brief Constructor por defecto
     */
    CurveParams() = default;
    
    /**
     * @brief Valida los parámetros de la curva
     * @return true si los parámetros son válidos
     */
    bool validate() const;
    
    /**
     * @brief Imprime los parámetros de la curva
     */
    void print() const;
};

/**
 * @brief Obtiene los parámetros de una curva estándar
 * @param type Tipo de curva
 * @return Parámetros de la curva
 */
CurveParams get_curve_params(CurveType type);

// ============================================================================
// PUNTO EN CURVA ELÍPTICA
// ============================================================================

/**
 * @brief Representa un punto en una curva elíptica
 * 
 * Soporta:
 * - Punto en el infinito (punto de identidad)
 * - Coordenadas afines (x, y)
 * - Coordenadas proyectivas (X, Y, Z) para optimización (futuro)
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
     * @param curve Parámetros de la curva
     */
    ECPoint(const BigInt& x, const BigInt& y, const CurveParams* curve);
    
    // Getters
    const BigInt& x() const { return x_; }
    const BigInt& y() const { return y_; }
    bool is_infinity() const { return is_infinity_; }
    const CurveParams* curve() const { return curve_; }
    
    /**
     * @brief Verifica si el punto está en la curva
     * @return true si el punto satisface la ecuación de la curva
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
// OPERACIONES EN CURVA ELÍPTICA
// ============================================================================

/**
 * @brief Suma de puntos en curva elíptica
 * 
 * Implementa la ley de grupo de la curva elíptica:
 * P + Q = R
 * 
 * Casos especiales:
 * - P + O = P (O = punto infinito)
 * - P + (-P) = O
 * - P + P = 2P (duplicación)
 * 
 * @param P Primer punto
 * @param Q Segundo punto
 * @return Suma P + Q
 */
ECPoint ec_add(const ECPoint& P, const ECPoint& Q);

/**
 * @brief Duplicación de punto (caso especial de suma)
 * @param P Punto a duplicar
 * @return 2P
 */
ECPoint ec_double(const ECPoint& P);

/**
 * @brief Negación de punto
 * @param P Punto
 * @return -P (reflexión sobre eje x)
 */
ECPoint ec_negate(const ECPoint& P);

/**
 * @brief Multiplicación escalar (operación fundamental)
 * 
 * Calcula k*P = P + P + ... + P (k veces)
 * 
 * Usa algoritmo "double-and-add" para eficiencia:
 * Complejidad: O(log k)
 * 
 * @param k Escalar (número de veces)
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
    BigInt private_key;     // Clave privada: número aleatorio en [1, n-1]
    ECPoint public_key;     // Clave pública: Q = d*G
    
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
 * 1. Genera número aleatorio d en [1, n-1] (clave privada)
 * 2. Calcula Q = d*G (clave pública)
 * 
 * @param curve Parámetros de la curva
 * @param rng Generador de números aleatorios
 * @return Par de claves
 */
ECKeyPair generate_keypair(const CurveParams& curve, RNG& rng);

// ============================================================================
// DIFFIE-HELLMAN EN CURVAS ELÍPTICAS (ECDH)
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
 * @param public_key Clave pública del otro
 * @return Punto compartido (usar x como secreto)
 */
ECPoint ecdh_shared_secret(const BigInt& private_key, 
                           const ECPoint& public_key);

/**
 * @brief Deriva clave simétrica del secreto ECDH
 * @param shared_point Punto compartido
 * @param key_bits Tamaño de clave deseado (128, 192, 256)
 * @return Clave simétrica derivada
 */
BigInt ecdh_derive_key(const ECPoint& shared_point, int key_bits = 256);

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