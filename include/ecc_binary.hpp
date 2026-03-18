// ecc_binary.hpp
// Elliptic Curve Cryptography over Binary Fields GF(2^m)
// Implementacion educativa con curvas estandar SEC 2
//
// Autor: Leon Elliott Fuller
// Fecha: 2026-03-14

#ifndef ECC_BINARY_HPP
#define ECC_BINARY_HPP

#include "common.hpp"
#include "rng.hpp"
#include <NTL/ZZ.h>
#include <NTL/GF2X.h>
#include <NTL/GF2E.h>
#include <NTL/GF2XFactoring.h>
#include <string>
#include <vector>

namespace crypto {

using namespace NTL;

// ============================================================================
// CURVAS EN CAMPOS BINARIOS GF(2^m)
// ============================================================================

/**
 * Diferencias fundamentales: campos primos Fp vs campos binarios GF(2^m)
 * 
 * En Fp (lo que ya tenemos):
 * - Ecuacion: y^2 = x^3 + ax + b (mod p), p primo
 * - Suma en el campo: suma modular (a + b mod p)
 * - Multiplicacion: multiplicacion modular
 * - Inversion: algoritmo extendido de Euclides o Fermat
 * 
 * En GF(2^m):
 * - Ecuacion: y^2 + xy = x^3 + ax^2 + b (forma no supersingular)
 * - Los elementos son polinomios de grado < m con coeficientes en {0, 1}
 * - Suma en el campo: XOR de coeficientes (sin carries, muy rapido)
 * - Multiplicacion: multiplicacion de polinomios mod polinomio irreducible
 * - Inversion: Euclides extendido para polinomios
 * 
 * Ventajas de GF(2^m):
 * - Suma = XOR (operacion nativa del hardware, extremadamente rapida)
 * - No hay carries en sumas (paralelismo natural)
 * - Cuadrado eficiente: insertar ceros entre bits, luego reducir
 * - Implementacion natural en hardware (FPGAs, ASICs)
 * 
 * Desventajas:
 * - Multiplicacion de polinomios es mas compleja en software
 * - Menos optimizaciones en librerias genericas de CPU
 * - Preocupaciones de seguridad por ataques de Weil descent (campos peque.)
 * - NIST ha ido retirando curvas binarias en favor de primas
 * 
 * Por que incluirlo en el TFG:
 * - Completitud teorica: dos familias principales de curvas elipticas
 * - Comparacion de rendimiento: campo primo vs campo binario
 * - Comprension de la aritmetica de polinomios sobre GF(2)
 * - Aplicaciones en IoT y hardware embebido
 * - Valor educativo de ver dos enfoques diferentes al mismo problema
 */

/**
 * @brief Tipo de curva binaria
 * 
 * Curvas recomendadas por SEC 2 (Standards for Efficient Cryptography):
 * - sect163k1: Curva Koblitz de 163 bits (a=1, eficiencias especiales)
 * - sect233k1: Curva Koblitz de 233 bits (~112 bits de seguridad)
 * - sect283k1: Curva Koblitz de 283 bits (~128 bits de seguridad)
 * - sect233r1: Curva aleatoria de 233 bits
 * - sect283r1: Curva aleatoria de 283 bits
 * 
 * Las curvas Koblitz (a in {0,1}) permiten optimizaciones especiales
 * usando el endomorfismo de Frobenius.
 */
enum class BinaryCurveType {
    SECT163K1,      // Koblitz, 163 bits, ~80 bits seguridad
    SECT233K1,      // Koblitz, 233 bits, ~112 bits seguridad
    SECT283K1,      // Koblitz, 283 bits, ~128 bits seguridad
    SECT233R1,      // Aleatoria, 233 bits, ~112 bits seguridad
    SECT283R1,      // Aleatoria, 283 bits, ~128 bits seguridad
    CUSTOM_BINARY   // Definida por usuario
};

/**
 * @brief Parametros de una curva eliptica sobre GF(2^m)
 * 
 * Ecuacion: y^2 + xy = x^3 + ax^2 + b  en GF(2^m)
 * 
 * El campo GF(2^m) se construye como GF(2)[x] / f(x) donde f(x) es
 * un polinomio irreducible de grado m. Los elementos del campo son
 * polinomios de grado < m con coeficientes en {0, 1}.
 * 
 * Para eficiencia, se usan polinomios irreducibles trinomiales
 * (x^m + x^k + 1) o pentanomiales (x^m + x^k3 + x^k2 + x^k1 + 1).
 */
struct BinaryCurveParams {
    int m;                      // Grado de la extension GF(2^m)
    GF2X reduction_poly;        // Polinomio irreducible f(x) de grado m
    
    // Coeficientes a, b de la curva (como elementos de GF(2^m))
    // Se almacenan como representacion hexadecimal y se convierten al inicializar
    std::string a_hex;          // Coeficiente a (como hex string)
    std::string b_hex;          // Coeficiente b (como hex string)
    std::string Gx_hex;         // Coordenada x del generador
    std::string Gy_hex;         // Coordenada y del generador
    BigInt n;                   // Orden del punto generador
    BigInt h;                   // Cofactor
    
    std::string name;           // Nombre de la curva
    int security_bits;          // Bits de seguridad equivalentes
    
    BinaryCurveParams() : m(0), security_bits(0) {}
    
    /**
     * @brief Inicializa el campo GF(2^m) de NTL con el polinomio reductor
     * Debe llamarse antes de cualquier operacion aritmetica
     */
    void init_field() const;
    
    /**
     * @brief Convierte string hexadecimal a elemento GF2E
     */
    GF2E hex_to_gf2e(const std::string& hex) const;
    
    /**
     * @brief Valida los parametros de la curva
     */
    bool validate() const;
    
    void print() const;
};

/**
 * @brief Obtiene parametros de una curva binaria estandar
 */
BinaryCurveParams get_binary_curve_params(BinaryCurveType type);

// ============================================================================
// PUNTO EN CURVA BINARIA
// ============================================================================

/**
 * @brief Punto en una curva eliptica sobre GF(2^m)
 * 
 * Los puntos se representan en coordenadas afines (x, y) donde
 * x, y son elementos de GF(2^m), es decir, polinomios de grado < m.
 * 
 * La ley de grupo es diferente a la de campos primos:
 * - Suma P + Q (P != Q):
 *   lambda = (y1 + y2) / (x1 + x2)   [suma = XOR en GF(2^m)]
 *   x3 = lambda^2 + lambda + x1 + x2 + a
 *   y3 = lambda*(x1 + x3) + x3 + y1
 * 
 * - Doblado 2P:
 *   lambda = x + y/x
 *   x3 = lambda^2 + lambda + a
 *   y3 = x^2 + (lambda + 1)*x3
 * 
 * Nota: la division en GF(2^m) es inversion de polinomios.
 */
class BinaryECPoint {
private:
    GF2E x_;
    GF2E y_;
    bool is_infinity_;
    const BinaryCurveParams* curve_;
    
public:
    /** @brief Constructor para punto en el infinito */
    BinaryECPoint(const BinaryCurveParams* curve);
    
    /** @brief Constructor con coordenadas */
    BinaryECPoint(const GF2E& x, const GF2E& y, const BinaryCurveParams* curve);
    
    const GF2E& x() const { return x_; }
    const GF2E& y() const { return y_; }
    bool is_infinity() const { return is_infinity_; }
    const BinaryCurveParams* curve() const { return curve_; }
    
    /** @brief Verifica si el punto satisface y^2 + xy = x^3 + ax^2 + b */
    bool is_on_curve() const;
    
    bool operator==(const BinaryECPoint& other) const;
    bool operator!=(const BinaryECPoint& other) const { return !(*this == other); }
    
    void print() const;
};

// ============================================================================
// OPERACIONES EN CURVA BINARIA
// ============================================================================

/**
 * @brief Suma de puntos en curva binaria
 * 
 * Para P = (x1,y1) y Q = (x2,y2) con P != Q:
 *   lambda = (y1 + y2) / (x1 + x2)
 *   x3 = lambda^2 + lambda + x1 + x2 + a
 *   y3 = lambda*(x1 + x3) + x3 + y1
 * 
 * Nota: + aqui es XOR (suma en GF(2^m)), / es inversion polinomial
 */
BinaryECPoint binary_ec_add(const BinaryECPoint& P, const BinaryECPoint& Q);

/**
 * @brief Doblado de punto en curva binaria
 * 
 * Para P = (x1, y1):
 *   lambda = x1 + y1/x1
 *   x3 = lambda^2 + lambda + a
 *   y3 = x1^2 + (lambda + 1)*x3
 * 
 * El cuadrado en GF(2^m) es especialmente eficiente: solo requiere
 * insertar ceros entre bits y reducir modulo el polinomio irreducible.
 * En NTL esto se hace automaticamente con sqr().
 */
BinaryECPoint binary_ec_double(const BinaryECPoint& P);

/**
 * @brief Negacion de punto en curva binaria
 * -P = (x, x + y) [en campos primos seria (x, -y)]
 */
BinaryECPoint binary_ec_negate(const BinaryECPoint& P);

/**
 * @brief Multiplicacion escalar en curva binaria
 * Algoritmo double-and-add, identico en estructura al de campos primos
 */
BinaryECPoint binary_ec_scalar_mult(const BigInt& k, const BinaryECPoint& P,
                                    const BigInt& order);

// ============================================================================
// CLAVES Y OPERACIONES CRIPTOGRAFICAS EN CURVAS BINARIAS
// ============================================================================

struct BinaryECKeyPair {
    BigInt private_key;
    BinaryECPoint public_key;
    const BinaryCurveParams* curve;
    void print(bool show_private = false) const;
};

/**
 * @brief Genera par de claves para curva binaria
 */
BinaryECKeyPair binary_generate_keypair(const BinaryCurveParams& curve, RNG& rng);

/**
 * @brief ECDH sobre curva binaria
 */
BinaryECPoint binary_ecdh_shared_secret(const BigInt& private_key,
                                        const BinaryECPoint& public_key,
                                        const BigInt& order);

// ============================================================================
// UTILIDADES
// ============================================================================

std::string binary_curve_type_to_string(BinaryCurveType type);

/**
 * @brief Convierte BigInt a polinomio GF2X (interpretando bits como coeficientes)
 */
GF2X bigint_to_gf2x(const BigInt& n);

/**
 * @brief Convierte string hexadecimal a GF2X
 */
GF2X hex_to_gf2x(const std::string& hex_str);

} // namespace crypto

#endif // ECC_BINARY_HPP