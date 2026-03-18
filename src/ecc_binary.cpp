// ecc_binary.cpp
// Implementacion de curvas elipticas sobre campos binarios GF(2^m)
// Usando NTL para aritmetica de polinomios sobre GF(2)
//
// Autor: Leon Elliott Fuller
// Fecha: 2026-03-14

#include "ecc_binary.hpp"
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <sstream>

namespace crypto {

// ============================================================================
// UTILIDADES DE CONVERSION
// ============================================================================

/**
 * Convierte un BigInt a un polinomio GF2X
 * 
 * Cada bit del BigInt se convierte en un coeficiente del polinomio.
 * Bit i del BigInt -> coeficiente de x^i en el polinomio.
 * 
 * Ejemplo: BigInt 13 = 1101 en binario -> x^3 + x^2 + 1
 */
GF2X bigint_to_gf2x(const BigInt& n) {
    GF2X result;
    BigInt temp = n;
    long i = 0;
    
    while (temp > 0) {
        if (IsOdd(temp)) {
            SetCoeff(result, i, 1);
        }
        temp >>= 1;
        i++;
    }
    
    return result;
}

/**
 * Convierte string hexadecimal a polinomio GF2X
 * 
 * Cada digito hex (4 bits) se expande a 4 coeficientes del polinomio.
 * El string se procesa de derecha a izquierda (LSB first en posiciones).
 */
GF2X hex_to_gf2x(const std::string& hex_str) {
    GF2X result;
    
    // Eliminar prefijo "0x" si existe
    std::string hex = hex_str;
    if (hex.size() >= 2 && hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')) {
        hex = hex.substr(2);
    }
    
    long bit_pos = 0;
    // Procesar de derecha a izquierda
    for (int i = hex.size() - 1; i >= 0; i--) {
        char c = hex[i];
        int val;
        if (c >= '0' && c <= '9') val = c - '0';
        else if (c >= 'a' && c <= 'f') val = 10 + c - 'a';
        else if (c >= 'A' && c <= 'F') val = 10 + c - 'A';
        else continue;  // ignorar caracteres no-hex
        
        for (int b = 0; b < 4; b++) {
            if (val & (1 << b)) {
                SetCoeff(result, bit_pos, 1);
            }
            bit_pos++;
        }
    }
    
    return result;
}

// ============================================================================
// PARAMETROS DE CURVAS BINARIAS ESTANDAR
// ============================================================================

void BinaryCurveParams::init_field() const {
    // Inicializar GF(2^m) con el polinomio irreducible
    GF2E::init(reduction_poly);
}

GF2E BinaryCurveParams::hex_to_gf2e(const std::string& hex) const {
    GF2X poly = hex_to_gf2x(hex);
    return conv<GF2E>(poly);
}

bool BinaryCurveParams::validate() const {
    if (m <= 0) return false;
    if (deg(reduction_poly) != m) return false;
    
    // Verificar que b != 0 (requerido para curva no singular)
    init_field();
    GF2E b = hex_to_gf2e(b_hex);
    if (IsZero(b)) return false;
    
    if (n <= 0) return false;
    if (h <= 0) return false;
    
    return true;
}

void BinaryCurveParams::print() const {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "CURVA BINARIA: " << name << "\n";
    std::cout << std::string(70, '=') << "\n";
    std::cout << "Campo:     GF(2^" << m << ")\n";
    std::cout << "Seguridad: ~" << security_bits << " bits\n";
    std::cout << "a:         " << a_hex << "\n";
    std::cout << "b:         " << b_hex << "\n";
    std::cout << "Gx:        " << Gx_hex << "\n";
    std::cout << "Gy:        " << Gy_hex << "\n";
    std::cout << "n (orden): " << n << "\n";
    std::cout << "h (cofact):" << h << "\n";
    std::cout << std::string(70, '=') << "\n";
}

/**
 * Parametros de curvas binarias estandar
 * 
 * Fuente: SEC 2 v2 (https://www.secg.org/sec2-v2.pdf)
 * 
 * Cada curva define:
 * - m: grado de la extension
 * - f(x): polinomio irreducible (trinomial o pentanomial)
 * - a, b: coeficientes de y^2 + xy = x^3 + ax^2 + b
 * - G = (Gx, Gy): punto generador
 * - n: orden del generador, h: cofactor
 * 
 * Las curvas Koblitz (sufijo 'k') tienen a in {0, 1},
 * lo que permite optimizaciones con el mapa de Frobenius.
 */
BinaryCurveParams get_binary_curve_params(BinaryCurveType type) {
    BinaryCurveParams params;
    
    switch (type) {
        case BinaryCurveType::SECT163K1: {
            // sect163k1 - Curva Koblitz sobre GF(2^163)
            // Polinomio irreducible: x^163 + x^7 + x^6 + x^3 + 1
            // Seguridad: ~80 bits (equivalente a RSA-1024)
            
            params.name = "sect163k1 (Koblitz)";
            params.m = 163;
            params.security_bits = 80;
            
            // f(x) = x^163 + x^7 + x^6 + x^3 + 1
            GF2X f;
            SetCoeff(f, 163, 1);
            SetCoeff(f, 7, 1);
            SetCoeff(f, 6, 1);
            SetCoeff(f, 3, 1);
            SetCoeff(f, 0, 1);
            params.reduction_poly = f;
            
            // a = 1 (Koblitz), b = 1
            params.a_hex = "1";
            params.b_hex = "1";
            
            // Generador (SEC 2 v2, seccion 3.2.1)
            params.Gx_hex = "02FE13C0537BBC11ACAA07D793DE4E6D5E5C94EEE8";
            params.Gy_hex = "0289070FB05D38FF58321F2E800536D538CCDAA3D9";
            
            // Orden y cofactor
            conv(params.n, "5846006549323611672814741753598448348329118574063");
            params.h = to_ZZ(2);
            
            break;
        }
        
        case BinaryCurveType::SECT233K1: {
            // sect233k1 - Curva Koblitz sobre GF(2^233)
            // Polinomio irreducible: x^233 + x^74 + 1 (trinomial)
            // Seguridad: ~112 bits (equivalente a RSA-2048)
            
            params.name = "sect233k1 (Koblitz)";
            params.m = 233;
            params.security_bits = 112;
            
            // f(x) = x^233 + x^74 + 1
            GF2X f;
            SetCoeff(f, 233, 1);
            SetCoeff(f, 74, 1);
            SetCoeff(f, 0, 1);
            params.reduction_poly = f;
            
            // a = 0 (Koblitz), b = 1
            params.a_hex = "0";
            params.b_hex = "1";
            
            // Generador
            params.Gx_hex = "017232BA853A7E731AF129F22FF4149563A419C26BF50A4C9D6EEFAD6126";
            params.Gy_hex = "01DB537DECE819B7F70F555A67C427A8CD9BF18AEB9B56E0C11056FAE6A3";
            
            conv(params.n, "3450873173395281893717377931138512760570940988862252126328087024741343");
            params.h = to_ZZ(4);
            
            break;
        }
        
        case BinaryCurveType::SECT283K1: {
            // sect283k1 - Curva Koblitz sobre GF(2^283)
            // Polinomio irreducible: x^283 + x^12 + x^7 + x^5 + 1
            // Seguridad: ~128 bits (equivalente a RSA-3072)
            
            params.name = "sect283k1 (Koblitz)";
            params.m = 283;
            params.security_bits = 128;
            
            // f(x) = x^283 + x^12 + x^7 + x^5 + 1 (pentanomial)
            GF2X f;
            SetCoeff(f, 283, 1);
            SetCoeff(f, 12, 1);
            SetCoeff(f, 7, 1);
            SetCoeff(f, 5, 1);
            SetCoeff(f, 0, 1);
            params.reduction_poly = f;
            
            // a = 0 (Koblitz), b = 1
            params.a_hex = "0";
            params.b_hex = "1";
            
            // Generador
            params.Gx_hex = "0503213F78CA44883F1A3B8162F188E553CD265F23C1567A16876913B0C2AC2458492836";
            params.Gy_hex = "01CCDA380F1C9E318D90F95D07E5426FE87E45C0E8184698E45962364E34116177DD2259";
            
            conv(params.n, "3885337784451458141838923813647037813284811733793061324295874997529815829704422603873");
            params.h = to_ZZ(4);
            
            break;
        }
        
        case BinaryCurveType::SECT233R1: {
            // sect233r1 - Curva aleatoria sobre GF(2^233)
            // Mismo campo que sect233k1, pero con coeficientes aleatorios
            // Seguridad: ~112 bits
            
            params.name = "sect233r1 (random)";
            params.m = 233;
            params.security_bits = 112;
            
            // f(x) = x^233 + x^74 + 1 (mismo trinomial)
            GF2X f;
            SetCoeff(f, 233, 1);
            SetCoeff(f, 74, 1);
            SetCoeff(f, 0, 1);
            params.reduction_poly = f;
            
            // Coeficientes aleatorios (verificablemente generados)
            params.a_hex = "1";
            params.b_hex = "0066647EDE6C332C7F8C0923BB58213B333B20E9CE4281FE115F7D8F90AD";
            
            // Generador
            params.Gx_hex = "00FAC9DFCBAC8313BB2139F1BB755FEF65BC391F8B36F8F8EB7371FD558B";
            params.Gy_hex = "01006A08A41903350678E58528BEBF8A0BEFF867A7CA36716F7E01F81052";
            
            conv(params.n, "6901746346790563787434755862277025555839812737345013555379383634485463");
            params.h = to_ZZ(2);
            
            break;
        }
        
        case BinaryCurveType::SECT283R1: {
            // sect283r1 - Curva aleatoria sobre GF(2^283)
            // Seguridad: ~128 bits
            
            params.name = "sect283r1 (random)";
            params.m = 283;
            params.security_bits = 128;
            
            // f(x) = x^283 + x^12 + x^7 + x^5 + 1
            GF2X f;
            SetCoeff(f, 283, 1);
            SetCoeff(f, 12, 1);
            SetCoeff(f, 7, 1);
            SetCoeff(f, 5, 1);
            SetCoeff(f, 0, 1);
            params.reduction_poly = f;
            
            params.a_hex = "1";
            params.b_hex = "027B680AC8B8596DA5A4AF8A19A0303FCA97FD7645309FA2A581485AF6263E313B79A2F5";
            
            params.Gx_hex = "05F939258DB7DD90E1934F8C70B0DFEC2EED25B8557EAC9C80E2E198F8CDBECD86B12053";
            params.Gy_hex = "03676854FE24141CB98FE6D4B20D02B4516FF702350EDDB0826779C813F0DF45BE8112F4";
            
            conv(params.n, "7770675568902916283677847627294075626569625924376904889109196526770044277787378692871");
            params.h = to_ZZ(2);
            
            break;
        }
        
        case BinaryCurveType::CUSTOM_BINARY:
            throw std::runtime_error("CUSTOM_BINARY requires manual parameter setting");
        
        default:
            throw std::runtime_error("Unknown binary curve type");
    }
    
    return params;
}

// ============================================================================
// BinaryECPoint - IMPLEMENTACION
// ============================================================================

BinaryECPoint::BinaryECPoint(const BinaryCurveParams* curve)
    : is_infinity_(true), curve_(curve) {
    if (!curve_) {
        throw std::invalid_argument("Curve parameters cannot be null");
    }
    // Inicializar campo
    curve_->init_field();
    clear(x_);
    clear(y_);
}

BinaryECPoint::BinaryECPoint(const GF2E& x, const GF2E& y, 
                             const BinaryCurveParams* curve)
    : x_(x), y_(y), is_infinity_(false), curve_(curve) {
    if (!curve_) {
        throw std::invalid_argument("Curve parameters cannot be null");
    }
}

/**
 * Verificacion de punto en curva binaria
 * 
 * Para y^2 + xy = x^3 + ax^2 + b:
 *   LHS = y^2 + xy
 *   RHS = x^3 + ax^2 + b
 *   Punto valido si LHS == RHS
 * 
 * En GF(2^m), y^2 NO es simplemente cuadrado: es cuadrado en el campo.
 * La suma (+) es XOR, la multiplicacion es producto de polinomios mod f(x).
 */
bool BinaryECPoint::is_on_curve() const {
    if (is_infinity_) return true;
    
    curve_->init_field();
    
    GF2E a = curve_->hex_to_gf2e(curve_->a_hex);
    GF2E b = curve_->hex_to_gf2e(curve_->b_hex);
    
    // LHS = y^2 + x*y
    GF2E lhs = sqr(y_) + x_ * y_;
    
    // RHS = x^3 + a*x^2 + b
    GF2E x_sq = sqr(x_);
    GF2E rhs = x_sq * x_ + a * x_sq + b;
    
    return lhs == rhs;
}

bool BinaryECPoint::operator==(const BinaryECPoint& other) const {
    if (curve_ != other.curve_) return false;
    if (is_infinity_ && other.is_infinity_) return true;
    if (is_infinity_ != other.is_infinity_) return false;
    return (x_ == other.x_) && (y_ == other.y_);
}

void BinaryECPoint::print() const {
    if (is_infinity_) {
        std::cout << "Binary EC Point at infinity (O)\n";
    } else {
        std::cout << "Binary EC Point on " << curve_->name << ":\n";
        std::cout << "  x = " << x_ << "\n";
        std::cout << "  y = " << y_ << "\n";
    }
}

// ============================================================================
// OPERACIONES EN CURVA BINARIA
// ============================================================================

/**
 * Suma de puntos en curva binaria: P + Q
 * 
 * Formulas (para P = (x1,y1), Q = (x2,y2), P != Q):
 * 
 *   lambda = (y1 + y2) / (x1 + x2)
 *   x3 = lambda^2 + lambda + x1 + x2 + a
 *   y3 = lambda * (x1 + x3) + x3 + y1
 * 
 * Notas sobre aritmetica en GF(2^m):
 * - "+" es XOR (no hay carries, no hay problemas de overflow)
 * - "-" es identico a "+" (en caracteristica 2, -x = x)
 * - "*" es multiplicacion de polinomios mod f(x)
 * - "/" requiere inversion: a/b = a * b^(-1)
 */
BinaryECPoint binary_ec_add(const BinaryECPoint& P, const BinaryECPoint& Q) {
    if (P.curve() != Q.curve()) {
        throw std::invalid_argument("Points must be on the same curve");
    }
    
    const BinaryCurveParams* curve = P.curve();
    curve->init_field();
    
    // Casos triviales
    if (P.is_infinity()) return Q;
    if (Q.is_infinity()) return P;
    
    // Caso P + (-P) = O
    // En GF(2^m), -P = (x, x + y), asi que P + (-P) si x1 == x2 y y1 == x2 + y2
    // Es decir, x1 == x2 y y1 + y2 == x1
    if (P.x() == Q.x()) {
        if (P.y() == Q.y()) {
            // P == Q: usar doblado
            return binary_ec_double(P);
        }
        // P.x == Q.x pero P != Q: deben ser inversos (P = -Q)
        return BinaryECPoint(curve);
    }
    
    GF2E a = curve->hex_to_gf2e(curve->a_hex);
    
    // lambda = (y1 + y2) / (x1 + x2)
    GF2E lambda = (P.y() + Q.y()) / (P.x() + Q.x());
    
    // x3 = lambda^2 + lambda + x1 + x2 + a
    GF2E x3 = sqr(lambda) + lambda + P.x() + Q.x() + a;
    
    // y3 = lambda*(x1 + x3) + x3 + y1
    GF2E y3 = lambda * (P.x() + x3) + x3 + P.y();
    
    return BinaryECPoint(x3, y3, curve);
}

/**
 * Doblado de punto en curva binaria: 2P
 * 
 * Para P = (x1, y1), x1 != 0:
 *   lambda = x1 + y1/x1
 *   x3 = lambda^2 + lambda + a
 *   y3 = x1^2 + (lambda + 1) * x3
 * 
 * Si x1 = 0, entonces 2P = O (punto en el infinito)
 * 
 * Nota: el cuadrado en GF(2^m) es una operacion lineal (Frobenius).
 * Para un elemento a = sum(a_i * x^i), tenemos a^2 = sum(a_i * x^(2i)).
 * Es decir, solo hay que insertar ceros entre los coeficientes y reducir.
 * NTL maneja esto automaticamente con sqr().
 */
BinaryECPoint binary_ec_double(const BinaryECPoint& P) {
    if (P.is_infinity()) return P;
    
    const BinaryCurveParams* curve = P.curve();
    curve->init_field();
    
    // Si x = 0, resultado es infinito
    if (IsZero(P.x())) {
        return BinaryECPoint(curve);
    }
    
    GF2E a = curve->hex_to_gf2e(curve->a_hex);
    
    // lambda = x1 + y1/x1
    GF2E lambda = P.x() + P.y() / P.x();
    
    // x3 = lambda^2 + lambda + a
    GF2E x3 = sqr(lambda) + lambda + a;
    
    // y3 = x1^2 + (lambda + 1) * x3
    GF2E one;
    set(one);  // one = 1 en GF(2^m)
    GF2E y3 = sqr(P.x()) + (lambda + one) * x3;
    
    return BinaryECPoint(x3, y3, curve);
}

/**
 * Negacion en curva binaria
 * 
 * En GF(2^m), la negacion de P = (x, y) es:
 *   -P = (x, x + y)
 * 
 * Esto es diferente de campos primos donde -P = (x, -y mod p).
 * La razon: si P = (x, y) esta en y^2 + xy = x^3 + ax^2 + b,
 * entonces (x, x+y) tambien lo esta (sustituir y' = x+y y verificar).
 * Ademas P + (-P) debe dar O, lo cual se puede comprobar con las
 * formulas de suma: si x1 = x2, se verifica y1 + y2 = x1,
 * es decir, y + (x + y) = x. Correcto.
 */
BinaryECPoint binary_ec_negate(const BinaryECPoint& P) {
    if (P.is_infinity()) return P;
    
    return BinaryECPoint(P.x(), P.x() + P.y(), P.curve());
}

/**
 * Multiplicacion escalar en curva binaria: k * P
 * 
 * Identico algoritmicamente al double-and-add de campos primos.
 * La estructura es la misma; solo la aritmetica subyacente cambia.
 * 
 * Para k de m bits: O(m) doblados + O(m/2) sumas en promedio.
 */
BinaryECPoint binary_ec_scalar_mult(const BigInt& k, const BinaryECPoint& P,
                                    const BigInt& order) {
    if (k == 0 || P.is_infinity()) {
        return BinaryECPoint(P.curve());
    }
    
    BinaryECPoint result(P.curve());  // Infinito
    BinaryECPoint addend = P;
    
    BigInt k_copy = k % order;
    
    while (k_copy > 0) {
        if (IsOdd(k_copy)) {
            result = binary_ec_add(result, addend);
        }
        addend = binary_ec_double(addend);
        k_copy >>= 1;
    }
    
    return result;
}

// ============================================================================
// CLAVES Y OPERACIONES CRIPTOGRAFICAS
// ============================================================================

BinaryECKeyPair binary_generate_keypair(const BinaryCurveParams& curve, RNG& rng) {
    // Inicializar campo
    curve.init_field();
    
    // 1. Generar clave privada: d in [1, n-1]
    BigInt private_key = rng.random_range(to_ZZ(1), curve.n - 1);
    
    // 2. Calcular clave publica: Q = d * G
    GF2E Gx = curve.hex_to_gf2e(curve.Gx_hex);
    GF2E Gy = curve.hex_to_gf2e(curve.Gy_hex);
    BinaryECPoint G(Gx, Gy, &curve);
    
    BinaryECPoint public_key = binary_ec_scalar_mult(private_key, G, curve.n);
    
    return BinaryECKeyPair{private_key, public_key, &curve};
}

void BinaryECKeyPair::print(bool show_private) const {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "PAR DE CLAVES ECC (Campo Binario)\n";
    std::cout << std::string(70, '=') << "\n";
    std::cout << "Curva: " << curve->name << "\n";
    std::cout << "Campo: GF(2^" << curve->m << ")\n";
    
    if (show_private) {
        std::cout << "Clave privada (d): " << private_key << "\n";
    } else {
        std::cout << "Clave privada: [OCULTA]\n";
    }
    
    std::cout << "\nClave publica (Q):\n";
    public_key.print();
    std::cout << std::string(70, '=') << "\n";
}

BinaryECPoint binary_ecdh_shared_secret(const BigInt& private_key,
                                        const BinaryECPoint& public_key,
                                        const BigInt& order) {
    return binary_ec_scalar_mult(private_key, public_key, order);
}

// ============================================================================
// UTILIDADES
// ============================================================================

std::string binary_curve_type_to_string(BinaryCurveType type) {
    switch (type) {
        case BinaryCurveType::SECT163K1:     return "sect163k1";
        case BinaryCurveType::SECT233K1:     return "sect233k1";
        case BinaryCurveType::SECT283K1:     return "sect283k1";
        case BinaryCurveType::SECT233R1:     return "sect233r1";
        case BinaryCurveType::SECT283R1:     return "sect283r1";
        case BinaryCurveType::CUSTOM_BINARY: return "Custom Binary";
        default:                             return "Unknown";
    }
}

} // namespace crypto