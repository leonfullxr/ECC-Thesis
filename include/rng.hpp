// rng.hpp
// Sistema de generación de números aleatorios con soporte para semillas
// Permite reproducibilidad en benchmarks mediante semillas fijas
// Autor: Leon Elliott Fuller
// Fecha: 2025-01-06

#ifndef RNG_HPP
#define RNG_HPP

#include "common.hpp"
#include <NTL/ZZ.h>
#include <memory>

namespace crypto {

// ============================================================================
// INTERFAZ ABSTRACTA RNG
// ============================================================================

/**
 * @brief Interfaz abstracta para generadores de números aleatorios
 * 
 * Esta clase define la interfaz común para todos los generadores de números
 * aleatorios. Permite usar diferentes implementaciones (NTL, OpenSSL, etc.)
 * de forma intercambiable.
 */
class RNG {
public:
    virtual ~RNG() = default;

    /**
     * @brief Genera un número aleatorio en el rango [0, n-1]
     * @param n Límite superior (exclusivo)
     * @return Número aleatorio en [0, n-1]
     */
    virtual BigInt random_bnd(const BigInt& n) = 0;

    /**
     * @brief Genera un número aleatorio en el rango [min, max]
     * @param min Límite inferior (inclusive)
     * @param max Límite superior (inclusive)
     * @return Número aleatorio en [min, max]
     */
    virtual BigInt random_range(const BigInt& min, const BigInt& max) = 0;

    /**
     * @brief Genera un número aleatorio de exactamente l bits
     * @param l Número de bits
     * @return Número aleatorio de l bits (el bit más significativo es 1)
     */
    virtual BigInt random_len(long l) = 0;

    /**
     * @brief Genera un número aleatorio de hasta l bits
     * @param l Número máximo de bits
     * @return Número aleatorio en [0, 2^l - 1]
     */
    virtual BigInt random_bits(long l) = 0;

    /**
     * @brief Obtiene la semilla actual (para debugging/logging)
     * @return Semilla como BigInt
     */
    virtual BigInt get_seed() const = 0;

    /**
     * @brief Genera un primo aleatorio de exactamente l bits
     * @param l Número de bits del primo
     * @param iterations Número de iteraciones Miller-Rabin (default: 40)
     * @return Número primo de l bits
     * 
     * Usa el test de primalidad de Miller-Rabin.
     * Con 40 iteraciones, la probabilidad de error es < 2^-80
     */
    virtual BigInt random_prime(long l, long iterations = MILLER_RABIN_ITERATIONS) = 0;
};

// ============================================================================
// IMPLEMENTACIÓN CON NTL
// ============================================================================

/**
 * @brief Generador de números aleatorios usando NTL
 * 
 * Esta implementación usa las funciones criptográficamente seguras de NTL
 * para generar números aleatorios. Soporta semillas fijas para permitir
 * reproducibilidad en los benchmarks.
 * 
 * Características:
 * - Criptográficamente seguro
 * - Reproducible con semillas fijas
 * - Independiente de la arquitectura hardware
 * 
 * Uso típico:
 * @code
 *   // Semilla fija para reproducibilidad
 *   NTLRNG rng(BigInt(12345));
 *   
 *   // Generar un número aleatorio de 2048 bits
 *   BigInt random_num = rng.random_len(2048);
 * @endcode
 */
class NTLRNG : public RNG {
private:
    BigInt seed_;
    bool initialized_;

    /**
     * @brief Inicializa el generador con la semilla
     */
    void initialize_seed();

public:
    /**
     * @brief Constructor con semilla
     * @param seed Semilla para el generador (BigInt para mayor flexibilidad)
     */
    explicit NTLRNG(const BigInt& seed);

    /**
     * @brief Constructor por defecto (usa semilla basada en tiempo)
     */
    NTLRNG();

    /**
     * @brief Reinicializa el generador con una nueva semilla
     * @param new_seed Nueva semilla
     * 
     * Útil para cambiar la semilla sin crear un nuevo objeto RNG
     */
    void set_seed(const BigInt& new_seed);

    // Implementación de la interfaz RNG
    BigInt random_bnd(const BigInt& n) override;
    BigInt random_range(const BigInt& min, const BigInt& max) override;
    BigInt random_len(long l) override;
    BigInt random_bits(long l) override;
    BigInt get_seed() const override;
    BigInt random_prime(long l, long iterations = MILLER_RABIN_ITERATIONS) override;
};

// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================

/**
 * @brief Crea un RNG basado en el modo de semilla especificado
 * @param seed_mode "fixed" para semilla 0, "random" para semilla basada en tiempo
 * @param fixed_value Valor de semilla fija si seed_mode == "fixed" (default: 0)
 * @return Puntero único al RNG creado
 */
std::unique_ptr<RNG> create_rng(const std::string& seed_mode, long fixed_value = 0);

/**
 * @brief Convierte un timestamp a una semilla válida
 * @param timestamp Timestamp Unix
 * @return Semilla basada en el timestamp
 */
inline BigInt timestamp_to_seed(long timestamp) {
    return BigInt(timestamp);
}

} // namespace crypto

#endif // RNG_HPP