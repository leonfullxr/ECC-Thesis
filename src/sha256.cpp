// sha256.cpp
// Implementación de SHA-256 siguiendo FIPS PUB 180-4
// 
// Referencia: NIST FIPS PUB 180-4
// https://csrc.nist.gov/publications/detail/fips/180/4/final
//
// Autor: Leon Elliott Fuller
// Fecha: 2026-02-28
// TODO: Quitar los numeros magicos de las funciones logicas y ponerlos como constantes

#include "sha256.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <stdexcept>

namespace crypto {

// ============================================================================
// CONSTANTES SHA-256 (FIPS PUB 180-4, Sección 4.2.2)
// ============================================================================

/**
 * Primeros 32 bits de las partes fraccionales de las
 * raíces cúbicas de los primeros 64 números primos (2..311)
 */
const std::array<uint32_t, 64> SHA256_K = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

/**
 * Primeros 32 bits de las partes fraccionales de las
 * raíces cuadradas de los primeros 8 números primos (2..19)
 */
const std::array<uint32_t, 8> SHA256_H0 = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

// ============================================================================
// SHA256Digest - IMPLEMENTACION
// ============================================================================

std::string SHA256Digest::to_hex() const {
    std::ostringstream oss;
    for (size_t i = 0; i < 32; ++i) {
        oss << std::hex << std::setfill('0') << std::setw(2) 
            << static_cast<unsigned int>(bytes[i]);
    }
    return oss.str();
}

BigInt SHA256Digest::to_bigint() const {
    // Construir BigInt a partir de los bytes del digest (big-endian)
    BigInt result;
    result = 0;
    
    for (size_t i = 0; i < 32; ++i) {
        result <<= 8;
        result += bytes[i];
    }
    
    return result;
}

bool SHA256Digest::operator==(const SHA256Digest& other) const {
    return bytes == other.bytes;
}

void SHA256Digest::print() const {
    std::cout << "SHA-256: " << to_hex() << "\n";
}

// ============================================================================
// SHA-256 - OPERACIONES LOGICAS (FIPS PUB 180-4)
// ============================================================================

uint32_t SHA256::rotr(uint32_t x, unsigned int n) {
    return (x >> n) | (x << (32 - n));
}

uint32_t SHA256::shr(uint32_t x, unsigned int n) {
    return x >> n;
}

uint32_t SHA256::ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

uint32_t SHA256::maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

uint32_t SHA256::sigma0(uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

uint32_t SHA256::sigma1(uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

uint32_t SHA256::lsigma0(uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ shr(x, 3);
}

uint32_t SHA256::lsigma1(uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ shr(x, 10);
}

// ============================================================================
// SHA-256 - PREPROCESAMIENTO (FIPS PUB 180-4)
// ============================================================================

std::vector<uint8_t> SHA256::pad_message(const uint8_t* data, size_t length) {
    // Calcular longitud total con padding
    // Mensaje + 1 byte (0x80) + padding zeros + 8 bytes (longitud)
    // Total debe ser múltiplo de 64 bytes (512 bits)
    
    size_t bit_length = length * 8;  // Longitud del mensaje en bits
    
    // Necesitamos espacio para: mensaje + 1 byte + 8 bytes de longitud
    // Redondeado al próximo múltiplo de 64
    size_t padded_length = length + 1;  // +1 para el byte 0x80
    
    // Añadir zeros hasta que padded_length ≡ 56 (mod 64)
    // (56 = 64 - 8, dejamos 8 bytes para la longitud)
    while (padded_length % 64 != 56) {
        padded_length++;
    }
    
    padded_length += 8;  // 8 bytes para la longitud en bits (big-endian)
    
    // Crear mensaje con padding
    std::vector<uint8_t> padded(padded_length, 0);
    
    // Copiar mensaje original
    std::memcpy(padded.data(), data, length);
    
    // Añadir bit '1' (como byte 0x80)
    padded[length] = 0x80;
    
    // Añadir longitud original en bits (64 bits, big-endian)
    // Los bytes de padding intermedio ya son 0
    uint64_t bit_len_be = bit_length;
    for (int i = 0; i < 8; ++i) {
        padded[padded_length - 1 - i] = static_cast<uint8_t>(bit_len_be & 0xFF);
        bit_len_be >>= 8;
    }
    
    return padded;
}

// ============================================================================
// SHA-256 - PROCESAMIENTO DE BLOQUE (FIPS PUB 180-4)
// ============================================================================

void SHA256::process_block(const uint8_t* block, uint32_t state[8]) {
    // 1. Preparar el message schedule W[0..63]
    uint32_t W[64];
    
    // W[0..15] = palabras del bloque (big-endian)
    for (int t = 0; t < 16; ++t) {
        W[t] = (static_cast<uint32_t>(block[t * 4]) << 24) |
               (static_cast<uint32_t>(block[t * 4 + 1]) << 16) |
               (static_cast<uint32_t>(block[t * 4 + 2]) << 8) |
               (static_cast<uint32_t>(block[t * 4 + 3]));
    }
    
    // W[16..63] = σ1(W[t-2]) + W[t-7] + σ0(W[t-15]) + W[t-16]
    for (int t = 16; t < 64; ++t) {
        W[t] = lsigma1(W[t - 2]) + W[t - 7] + lsigma0(W[t - 15]) + W[t - 16];
    }
    
    // 2. Inicializar variables de trabajo
    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t e = state[4];
    uint32_t f = state[5];
    uint32_t g = state[6];
    uint32_t h = state[7];
    
    // 3. 64 rondas de compresión
    for (int t = 0; t < 64; ++t) {
        uint32_t T1 = h + sigma1(e) + ch(e, f, g) + SHA256_K[t] + W[t];
        uint32_t T2 = sigma0(a) + maj(a, b, c);
        
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
    }
    
    // 4. Calcular el nuevo valor intermedio del hash
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

// ============================================================================
// SHA-256 - FUNCIONES DE HASH PRINCIPALES
// ============================================================================

SHA256Digest SHA256::hash(const uint8_t* data, size_t length) {
    // 1. Preprocesamiento: padding del mensaje
    std::vector<uint8_t> padded = pad_message(data, length);
    
    // 2. Inicializar estado del hash (FIPS PUB 180-4, Sección 5.3.3)
    uint32_t state[8];
    for (int i = 0; i < 8; ++i) {
        state[i] = SHA256_H0[i];
    }
    
    // 3. Procesar cada bloque de 512 bits (64 bytes)
    size_t num_blocks = padded.size() / 64;
    
    for (size_t i = 0; i < num_blocks; ++i) {
        process_block(padded.data() + i * 64, state);
    }
    
    // 4. Producir el digest final (big-endian)
    SHA256Digest digest;
    
    for (int i = 0; i < 8; ++i) {
        digest.bytes[i * 4]     = static_cast<uint8_t>((state[i] >> 24) & 0xFF);
        digest.bytes[i * 4 + 1] = static_cast<uint8_t>((state[i] >> 16) & 0xFF);
        digest.bytes[i * 4 + 2] = static_cast<uint8_t>((state[i] >> 8) & 0xFF);
        digest.bytes[i * 4 + 3] = static_cast<uint8_t>(state[i] & 0xFF);
    }
    
    return digest;
}

SHA256Digest SHA256::hash(const std::string& message) {
    return hash(reinterpret_cast<const uint8_t*>(message.data()), message.size());
}

SHA256Digest SHA256::hash(const std::vector<uint8_t>& data) {
    return hash(data.data(), data.size());
}

BigInt SHA256::hash_to_bigint(const std::string& message) {
    return hash(message).to_bigint();
}

} // namespace crypto