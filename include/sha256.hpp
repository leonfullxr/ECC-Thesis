// sha256.hpp
// Implementacion de SHA-256 siguiendo FIPS PUB 180-4
// Secure Hash Algorithm 256 bits
//
// Autor: Leon Elliott Fuller
// Fecha: 2026-02-28

#ifndef SHA256_HPP
#define SHA256_HPP

#include "common.hpp"
#include <NTL/ZZ.h>
#include <string>
#include <vector>
#include <cstdint>
#include <array>

namespace crypto {

// ============================================================================
// CONSTANTES SHA-256 (FIPS PUB 180-4)
// ============================================================================

/**
 * @brief 64 constantes de ronda para SHA-256
 * 
 * Representan los primeros 32 bits de las partes fraccionales
 * de las raices cubicas de los primeros 64 numeros primos.
 */
extern const std::array<uint32_t, 64> SHA256_K;

/**
 * @brief 8 valores hash iniciales para SHA-256
 * 
 * Representan los primeros 32 bits de las partes fraccionales
 * de las raíces cuadradas de los primeros 8 numeros primos.
 */
extern const std::array<uint32_t, 8> SHA256_H0;

// ============================================================================
// ESTRUCTURA DE RESULTADO SHA-256
// ============================================================================

/**
 * @brief Resultado de un hash SHA-256
 * 
 * Contiene los 256 bits (32 bytes) del digest.
 * Proporciona conversion a diferentes formatos.
 */
struct SHA256Digest {
    std::array<uint8_t, 32> bytes;  // 256 bits = 32 bytes
    
    /**
     * @brief Convierte el digest a representacion hexadecimal
     * @return String hexadecimal de 64 caracteres
     */
    std::string to_hex() const;
    
    /**
     * @brief Convierte el digest a BigInt (NTL::ZZ)
     * @return BigInt representando el valor del hash
     */
    BigInt to_bigint() const;
    
    /**
     * @brief Igualdad de digests
     */
    bool operator==(const SHA256Digest& other) const;
    bool operator!=(const SHA256Digest& other) const { return !(*this == other); }
    
    /**
     * @brief Imprime el digest en formato hexadecimal
     */
    void print() const;
};

// ============================================================================
// CLASE SHA-256
// ============================================================================

/**
 * @brief Implementacion de SHA-256 siguiendo FIPS PUB 180-4
 * 
 * Caracteristicas:
 * - Implementacion completa desde cero
 * - Compatible con vectores de test de NIST
 * - Soporta mensajes de cualquier longitud
 * - Proporciona hash en multiples formatos
 * 
 * Referencia: NIST FIPS PUB 180-4
 * https://csrc.nist.gov/publications/detail/fips/180/4/final
 * 
 * Ejemplo de uso:
 * @code
 *   SHA256Digest hash = SHA256::hash("Hello, World!");
 *   std::cout << hash.to_hex() << std::endl;
 *   
 *   // Hash de bytes arbitrarios
 *   std::vector<uint8_t> data = {0x01, 0x02, 0x03};
 *   SHA256Digest hash2 = SHA256::hash(data);
 * @endcode
 */
class SHA256 {
public:
    // ========================================================================
    // FUNCIONES DE HASH PRINCIPALES
    // ========================================================================
    
    /**
     * @brief Calcula SHA-256 de un string
     * @param message Mensaje a hashear
     * @return Digest de 256 bits
     */
    static SHA256Digest hash(const std::string& message);
    
    /**
     * @brief Calcula SHA-256 de un vector de bytes
     * @param data Datos a hashear
     * @return Digest de 256 bits
     */
    static SHA256Digest hash(const std::vector<uint8_t>& data);
    
    /**
     * @brief Calcula SHA-256 de un buffer de bytes
     * @param data Puntero a los datos
     * @param length Longitud en bytes
     * @return Digest de 256 bits
     */
    static SHA256Digest hash(const uint8_t* data, size_t length);
    
    /**
     * @brief Calcula SHA-256 y retorna como BigInt
     * 
     * Conveniente para uso en ECDSA y otros protocolos criptograficos
     * donde el hash se usa como numero entero.
     * 
     * @param message Mensaje a hashear
     * @return BigInt representando el hash
     */
    static BigInt hash_to_bigint(const std::string& message);

private:
    // ========================================================================
    // OPERACIONES LOGICAS (FIPS PUB 180-4, Seccion 4.1.2)
    // ========================================================================
    
    /**
     * @brief Rotacion circular a la derecha
     * ROTR^n(x) = (x >> n) | (x << (32 - n))
     */
    static uint32_t rotr(uint32_t x, unsigned int n);
    
    /**
     * @brief Desplazamiento a la derecha
     * SHR^n(x) = x >> n
     */
    static uint32_t shr(uint32_t x, unsigned int n);
    
    /**
     * @brief Funcion Ch (Choice)
     * Ch(x,y,z) = (x AND y) XOR (NOT x AND z)
     */
    static uint32_t ch(uint32_t x, uint32_t y, uint32_t z);
    
    /**
     * @brief Funcion Maj (Majority)
     * Maj(x,y,z) = (x AND y) XOR (x AND z) XOR (y AND z)
     */
    static uint32_t maj(uint32_t x, uint32_t y, uint32_t z);
    
    /**
     * @brief Funcion Σ0 (Sigma mayuscula 0)
     * Σ0(x) = ROTR²(x) XOR ROTR¹³(x) XOR ROTR²²(x)
     */
    static uint32_t sigma0(uint32_t x);
    
    /**
     * @brief Funcion Σ1 (Sigma mayuscula 1)
     * Σ1(x) = ROTR⁶(x) XOR ROTR¹¹(x) XOR ROTR²⁵(x)
     */
    static uint32_t sigma1(uint32_t x);
    
    /**
     * @brief Funcion σ0 (sigma minuscula 0)
     * σ0(x) = ROTR⁷(x) XOR ROTR¹⁸(x) XOR SHR³(x)
     */
    static uint32_t lsigma0(uint32_t x);
    
    /**
     * @brief Funcion σ1 (sigma minuscula 1)
     * σ1(x) = ROTR¹⁷(x) XOR ROTR¹⁹(x) XOR SHR¹⁰(x)
     */
    static uint32_t lsigma1(uint32_t x);
    
    // ========================================================================
    // PROCESAMIENTO INTERNO
    // ========================================================================
    
    /**
     * @brief Preprocesamiento del mensaje (padding)
     * 
     * FIPS PUB 180-4:
     * 1. Añadir bit '1' al final del mensaje
     * 2. Añadir bits '0' hasta que longitud ≡ 448 (mod 512)
     * 3. Añadir longitud original del mensaje (64 bits, big-endian)
     * 
     * @param data Datos originales
     * @param length Longitud en bytes
     * @return Mensaje con padding (multiplo de 64 bytes)
     */
    static std::vector<uint8_t> pad_message(const uint8_t* data, size_t length);
    
    /**
     * @brief Procesa un bloque de 512 bits (64 bytes)
     * 
     * FIPS PUB 180-4:
     * Ejecuta 64 rondas de compresion sobre el bloque.
     * 
     * @param block Puntero al bloque de 64 bytes
     * @param state Estado actual del hash (8 words de 32 bits)
     */
    static void process_block(const uint8_t* block, uint32_t state[8]);
};

} // namespace crypto

#endif // SHA256_HPP