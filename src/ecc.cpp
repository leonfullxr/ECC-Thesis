// ecc.cpp
// Implementacion de Elliptic Curve Cryptography (ECC)
// Coordenadas afines + Jacobianas para curvas sobre campos primos
// 
// Autor: Leon Elliott Fuller
// Fecha: 2026-03-02

#include "ecc.hpp"
#include "sha256.hpp"
#include <iostream>
#include <iomanip>
#include <stdexcept>

namespace crypto {

// ============================================================================
// PARAMETROS DE CURVAS ESTANDAR
// ============================================================================

CurveParams get_curve_params(CurveType type) {
    CurveParams params;
    
    switch (type) {
        case CurveType::NIST_P256: {
            // NIST P-256 (secp256r1)
            // Curva: y^2 = x^3 - 3x + b (mod p)
            // Seguridad equivalente: RSA-3072
            
            params.name = "NIST P-256 (secp256r1)";
            params.bits = 256;
            
            // p = 2^256 - 2^224 + 2^192 + 2^96 - 1
            conv(params.p, "115792089210356248762697446949407573530086143415290314195533631308867097853951");
            
            // a = -3 (mod p)
            conv(params.a, "115792089210356248762697446949407573530086143415290314195533631308867097853948");
            
            // b
            conv(params.b, "41058363725152142129326129780047268409114441015993725554835256314039467401291");
            
            // Generator G
            conv(params.Gx, "48439561293906451759052585252797914202762949526041747995844080717082404635286");
            conv(params.Gy, "36134250956749795798585127919587881956611106672985015071877198253568414405109");
            
            // Order n (prime)
            conv(params.n, "115792089210356248762697446949407573529996955224135760342422259061068512044369");
            
            params.h = to_ZZ(1);
            break;
        }
        
        case CurveType::NIST_P384: {
            // NIST P-384
            // Seguridad equivalente: RSA-7680
            
            params.name = "NIST P-384";
            params.bits = 384;
            
            conv(params.p, "39402006196394479212279040100143613805079739270465446667948293404245721771496870329047266088258938001861606973112319");
            conv(params.a, "39402006196394479212279040100143613805079739270465446667948293404245721771496870329047266088258938001861606973112316");
            conv(params.b, "27580193559959705877849011840389048093056905856361568521428707301988689241309860865136260764883745107765439761230575");
            conv(params.Gx, "26247035095799689268623156744566981891852923491109213387815615900925518854738050089022388053975719786650872476732087");
            conv(params.Gy, "8325710961489029985546751289520108179287853048861315594709205902480503199884419224438643760392947333078086511627871");
            conv(params.n, "39402006196394479212279040100143613805079739270465446667946905279627659399113263569398956308152294913554433653942643");
            
            params.h = to_ZZ(1);
            break;
        }
        
        case CurveType::SECP256K1: {
            // secp256k1 (Bitcoin/Ethereum)
            // Curva: y^2 = x^3 + 7 (mod p), a = 0, b = 7
            
            params.name = "secp256k1 (Bitcoin)";
            params.bits = 256;
            
            conv(params.p, "115792089237316195423570985008687907853269984665640564039457584007908834671663");
            params.a = to_ZZ(0);
            params.b = to_ZZ(7);
            conv(params.Gx, "55066263022277343669578718895168534326250603453777594175500187360389116729240");
            conv(params.Gy, "32670510020758816978083085130507043184471273380659243275938904335757337482424");
            conv(params.n, "115792089237316195423570985008687907852837564279074904382605163141518161494337");
            
            params.h = to_ZZ(1);
            break;
        }
        
        case CurveType::CUSTOM:
            throw std::runtime_error("CUSTOM curve type requires manual parameter setting");
        
        default:
            throw std::runtime_error("Unknown curve type");
    }
    
    return params;
}

bool CurveParams::validate() const {
    if (p <= 3) return false;
    
    // Verificar discriminante: 4a^3 + 27b^2 != 0 (mod p)
    ZZ_p::init(p);
    ZZ_p a_p = conv<ZZ_p>(a);
    ZZ_p b_p = conv<ZZ_p>(b);
    
    ZZ_p discriminant = 4 * power(a_p, 3) + 27 * power(b_p, 2);
    if (IsZero(discriminant)) return false;
    
    // Verificar que G esta en la curva
    ZZ_p Gx_p = conv<ZZ_p>(Gx);
    ZZ_p Gy_p = conv<ZZ_p>(Gy);
    
    ZZ_p lhs = power(Gy_p, 2);
    ZZ_p rhs = power(Gx_p, 3) + a_p * Gx_p + b_p;
    
    if (lhs != rhs) return false;
    
    if (n <= 0) return false;
    if (h <= 0) return false;
    
    return true;
}

void CurveParams::print() const {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "CURVA: " << name << "\n";
    std::cout << std::string(70, '=') << "\n";
    std::cout << "Bits:       " << bits << "\n";
    std::cout << "p (modulo): " << p << "\n";
    std::cout << "a:          " << a << "\n";
    std::cout << "b:          " << b << "\n";
    std::cout << "Gx:         " << Gx << "\n";
    std::cout << "Gy:         " << Gy << "\n";
    std::cout << "n (orden):  " << n << "\n";
    std::cout << "h (cofact): " << h << "\n";
    std::cout << std::string(70, '=') << "\n";
}

// ============================================================================
// ECPoint (AFIN) - IMPLEMENTACION
// ============================================================================

ECPoint::ECPoint(const CurveParams* curve)
    : x_(0), y_(0), is_infinity_(true), curve_(curve) {
    if (!curve_) {
        throw std::invalid_argument("Curve parameters cannot be null");
    }
}

ECPoint::ECPoint(const BigInt& x, const BigInt& y, const CurveParams* curve)
    : x_(x), y_(y), is_infinity_(false), curve_(curve) {
    if (!curve_) {
        throw std::invalid_argument("Curve parameters cannot be null");
    }
    
    if (!is_on_curve()) {
        throw std::invalid_argument("Point is not on the curve");
    }
}

bool ECPoint::is_on_curve() const {
    if (is_infinity_) return true;
    
    // Verificar: y^2 = x^3 + ax + b (mod p)
    ZZ_p::init(curve_->p);
    
    ZZ_p x_p = conv<ZZ_p>(x_);
    ZZ_p y_p = conv<ZZ_p>(y_);
    ZZ_p a_p = conv<ZZ_p>(curve_->a);
    ZZ_p b_p = conv<ZZ_p>(curve_->b);
    
    ZZ_p lhs = power(y_p, 2);
    ZZ_p rhs = power(x_p, 3) + a_p * x_p + b_p;
    
    return lhs == rhs;
}

bool ECPoint::operator==(const ECPoint& other) const {
    if (curve_ != other.curve_) return false;
    if (is_infinity_ && other.is_infinity_) return true;
    if (is_infinity_ != other.is_infinity_) return false;
    return (x_ == other.x_) && (y_ == other.y_);
}

void ECPoint::print() const {
    if (is_infinity_) {
        std::cout << "Point at infinity (O)\n";
    } else {
        std::cout << "Point on " << curve_->name << ":\n";
        std::cout << "  x = " << x_ << "\n";
        std::cout << "  y = " << y_ << "\n";
    }
}

// ============================================================================
// JacobianPoint - IMPLEMENTACION
// ============================================================================

JacobianPoint::JacobianPoint(const CurveParams* curve)
    : X_(to_ZZ(1)), Y_(to_ZZ(1)), Z_(to_ZZ(0)), curve_(curve) {
    if (!curve_) {
        throw std::invalid_argument("Curve parameters cannot be null");
    }
}

JacobianPoint::JacobianPoint(const BigInt& X, const BigInt& Y, const BigInt& Z,
                             const CurveParams* curve)
    : X_(X), Y_(Y), Z_(Z), curve_(curve) {
    if (!curve_) {
        throw std::invalid_argument("Curve parameters cannot be null");
    }
}

void JacobianPoint::print() const {
    if (is_infinity()) {
        std::cout << "Jacobian Point at infinity (Z=0)\n";
    } else {
        std::cout << "Jacobian Point on " << curve_->name << ":\n";
        std::cout << "  X = " << X_ << "\n";
        std::cout << "  Y = " << Y_ << "\n";
        std::cout << "  Z = " << Z_ << "\n";
    }
}

// ============================================================================
// OPERACIONES AFINES
// ============================================================================

ECPoint ec_add(const ECPoint& P, const ECPoint& Q) {
    if (P.curve() != Q.curve()) {
        throw std::invalid_argument("Points must be on the same curve");
    }
    
    const CurveParams* curve = P.curve();
    
    if (P.is_infinity()) return Q;
    if (Q.is_infinity()) return P;
    
    if (P.x() == Q.x() && P.y() != Q.y()) {
        return ECPoint(curve);
    }
    
    if (P == Q) {
        return ec_double(P);
    }
    
    ZZ_p::init(curve->p);
    
    ZZ_p x1 = conv<ZZ_p>(P.x());
    ZZ_p y1 = conv<ZZ_p>(P.y());
    ZZ_p x2 = conv<ZZ_p>(Q.x());
    ZZ_p y2 = conv<ZZ_p>(Q.y());
    
    // lambda = (y2 - y1) / (x2 - x1)  <-- division = inversion + multiplicacion
    ZZ_p lambda = (y2 - y1) / (x2 - x1);
    
    ZZ_p x3 = power(lambda, 2) - x1 - x2;
    ZZ_p y3 = lambda * (x1 - x3) - y1;
    
    return ECPoint(conv<BigInt>(x3), conv<BigInt>(y3), curve);
}

ECPoint ec_double(const ECPoint& P) {
    if (P.is_infinity()) return P;
    
    const CurveParams* curve = P.curve();
    
    if (P.y() == 0) {
        return ECPoint(curve);
    }
    
    ZZ_p::init(curve->p);
    
    ZZ_p x = conv<ZZ_p>(P.x());
    ZZ_p y = conv<ZZ_p>(P.y());
    ZZ_p a = conv<ZZ_p>(curve->a);
    
    // lambda = (3x^2 + a) / (2y)  <-- otra inversion
    ZZ_p lambda = (3 * power(x, 2) + a) / (2 * y);
    
    ZZ_p x3 = power(lambda, 2) - 2 * x;
    ZZ_p y3 = lambda * (x - x3) - y;
    
    return ECPoint(conv<BigInt>(x3), conv<BigInt>(y3), curve);
}

ECPoint ec_negate(const ECPoint& P) {
    if (P.is_infinity()) return P;
    
    const CurveParams* curve = P.curve();
    BigInt neg_y = (curve->p - P.y()) % curve->p;
    
    return ECPoint(P.x(), neg_y, curve);
}

ECPoint ec_scalar_mult(const BigInt& k, const ECPoint& P) {
    if (k == 0 || P.is_infinity()) {
        return ECPoint(P.curve());
    }
    
    const CurveParams* curve = P.curve();
    
    ECPoint result(curve);
    ECPoint addend = P;
    
    BigInt k_copy = k % curve->n;
    
    while (k_copy > 0) {
        if (k_copy % 2 == 1) {
            result = ec_add(result, addend);
        }
        addend = ec_double(addend);
        k_copy /= 2;
    }
    
    return result;
}

// ============================================================================
// OPERACIONES EN COORDENADAS JACOBIANAS
// ============================================================================

JacobianPoint to_jacobian(const ECPoint& P) {
    if (P.is_infinity()) {
        return JacobianPoint(P.curve());
    }
    // Conversion trivial: (x, y) -> (x, y, 1)
    return JacobianPoint(P.x(), P.y(), to_ZZ(1), P.curve());
}

ECPoint to_affine(const JacobianPoint& J) {
    if (J.is_infinity()) {
        return ECPoint(J.curve());
    }
    
    const CurveParams* curve = J.curve();
    
    // Esta es la UNICA inversion necesaria en toda la multiplicacion escalar
    // x = X * Z^(-2), y = Y * Z^(-3)
    ZZ_p::init(curve->p);
    
    ZZ_p Z_inv = inv(conv<ZZ_p>(J.Z()));
    ZZ_p Z_inv2 = power(Z_inv, 2);    // Z^(-2)
    ZZ_p Z_inv3 = Z_inv2 * Z_inv;     // Z^(-3)
    
    ZZ_p x = conv<ZZ_p>(J.X()) * Z_inv2;
    ZZ_p y = conv<ZZ_p>(J.Y()) * Z_inv3;
    
    return ECPoint(conv<BigInt>(x), conv<BigInt>(y), curve);
}

/**
 * Suma en coordenadas Jacobianas
 * 
 * Entrada: P1 = (X1, Y1, Z1), P2 = (X2, Y2, Z2)
 * Salida:  P3 = (X3, Y3, Z3) = P1 + P2
 * 
 * Formulas (fuente: "Guide to Elliptic Curve Cryptography", Hankerson et al.):
 *   U1 = X1 * Z2^2
 *   U2 = X2 * Z1^2
 *   S1 = Y1 * Z2^3
 *   S2 = Y2 * Z1^3
 *   H  = U2 - U1
 *   R  = S2 - S1
 * 
 *   Si H == 0 y R == 0: es doblado (P1 == P2)
 *   Si H == 0 y R != 0: resultado es infinito (P1 == -P2)
 * 
 *   X3 = R^2 - H^3 - 2*U1*H^2
 *   Y3 = R*(U1*H^2 - X3) - S1*H^3
 *   Z3 = H * Z1 * Z2
 * 
 * Coste total: 12M + 4S (0 inversiones)
 */
JacobianPoint jacobian_add(const JacobianPoint& P, const JacobianPoint& Q) {
    if (P.curve() != Q.curve()) {
        throw std::invalid_argument("Points must be on the same curve");
    }
    
    const CurveParams* curve = P.curve();
    
    // Casos triviales
    if (P.is_infinity()) return Q;
    if (Q.is_infinity()) return P;
    
    ZZ_p::init(curve->p);
    
    ZZ_p X1 = conv<ZZ_p>(P.X());
    ZZ_p Y1 = conv<ZZ_p>(P.Y());
    ZZ_p Z1 = conv<ZZ_p>(P.Z());
    ZZ_p X2 = conv<ZZ_p>(Q.X());
    ZZ_p Y2 = conv<ZZ_p>(Q.Y());
    ZZ_p Z2 = conv<ZZ_p>(Q.Z());
    
    ZZ_p Z1_sq = sqr(Z1);       // Z1^2
    ZZ_p Z2_sq = sqr(Z2);       // Z2^2
    
    ZZ_p U1 = X1 * Z2_sq;       // U1 = X1 * Z2^2
    ZZ_p U2 = X2 * Z1_sq;       // U2 = X2 * Z1^2
    ZZ_p S1 = Y1 * Z2_sq * Z2;  // S1 = Y1 * Z2^3
    ZZ_p S2 = Y2 * Z1_sq * Z1;  // S2 = Y2 * Z1^3
    
    ZZ_p H = U2 - U1;           // H = U2 - U1
    ZZ_p R = S2 - S1;           // R = S2 - S1
    
    // Comprobar casos especiales
    if (IsZero(H)) {
        if (IsZero(R)) {
            // P == Q: usar doblado
            return jacobian_double(P);
        }
        // P == -Q: resultado es infinito
        return JacobianPoint(curve);
    }
    
    ZZ_p H_sq = sqr(H);         // H^2
    ZZ_p H_cu = H_sq * H;       // H^3
    ZZ_p U1H2 = U1 * H_sq;     // U1 * H^2
    
    // X3 = R^2 - H^3 - 2*U1*H^2
    ZZ_p X3 = sqr(R) - H_cu - 2 * U1H2;
    
    // Y3 = R*(U1*H^2 - X3) - S1*H^3
    ZZ_p Y3 = R * (U1H2 - X3) - S1 * H_cu;
    
    // Z3 = H * Z1 * Z2
    ZZ_p Z3 = H * Z1 * Z2;
    
    return JacobianPoint(conv<BigInt>(X3), conv<BigInt>(Y3), 
                         conv<BigInt>(Z3), curve);
}

/**
 * Doblado en coordenadas Jacobianas
 * 
 * Entrada: P = (X1, Y1, Z1)
 * Salida:  2P = (X3, Y3, Z3)
 * 
 * Formulas:
 *   A = Y1^2
 *   B = 4 * X1 * A
 *   C = 8 * A^2
 *   D = 3 * X1^2 + a * Z1^4
 *   X3 = D^2 - 2*B
 *   Y3 = D*(B - X3) - C
 *   Z3 = 2 * Y1 * Z1
 * 
 * Coste total: 4M + 4S (0 inversiones)
 * 
 * Nota: para secp256k1 donde a=0, D se simplifica a 3*X1^2 (ahorro extra).
 * Para NIST P-256 donde a=-3, D = 3*(X1-Z1^2)*(X1+Z1^2) (optimizable).
 */
JacobianPoint jacobian_double(const JacobianPoint& P) {
    if (P.is_infinity()) return P;
    
    const CurveParams* curve = P.curve();
    
    ZZ_p::init(curve->p);
    
    ZZ_p X1 = conv<ZZ_p>(P.X());
    ZZ_p Y1 = conv<ZZ_p>(P.Y());
    ZZ_p Z1 = conv<ZZ_p>(P.Z());
    
    // Si Y1 = 0, resultado es infinito (tangente vertical)
    if (IsZero(Y1)) {
        return JacobianPoint(curve);
    }
    
    ZZ_p a_p = conv<ZZ_p>(curve->a);
    
    ZZ_p A = sqr(Y1);                  // A = Y1^2
    ZZ_p B = 4 * X1 * A;              // B = 4*X1*A
    ZZ_p C = 8 * sqr(A);              // C = 8*A^2
    
    // D = 3*X1^2 + a*Z1^4
    ZZ_p Z1_sq = sqr(Z1);
    ZZ_p D = 3 * sqr(X1) + a_p * sqr(Z1_sq);
    
    // X3 = D^2 - 2*B
    ZZ_p X3 = sqr(D) - 2 * B;
    
    // Y3 = D*(B - X3) - C
    ZZ_p Y3 = D * (B - X3) - C;
    
    // Z3 = 2*Y1*Z1
    ZZ_p Z3 = 2 * Y1 * Z1;
    
    return JacobianPoint(conv<BigInt>(X3), conv<BigInt>(Y3),
                         conv<BigInt>(Z3), curve);
}

/**
 * Multiplicacion escalar usando coordenadas Jacobianas
 * 
 * Flujo:
 * 1. Convertir punto afin P a Jacobiano J = (Px, Py, 1)
 * 2. Ejecutar double-and-add enteramente en Jacobianas
 * 3. Convertir resultado final a afin (UNA sola inversion)
 * 
 * Para k de 256 bits:
 * - ~256 doblados Jacobianos (4M + 4S cada uno)
 * - ~128 sumas Jacobianas (12M + 4S cada una)
 * - 1 inversion final
 * Total: ~2560M + ~1536S + 1I
 * 
 * Comparado con afin:
 * - ~256 doblados (1I + 2M + 2S) + ~128 sumas (1I + 2M + 1S)
 * Total: ~384I + ~768M + ~640S
 * Con I ~ 80M: ~31,488M equivalentes (afin) vs ~4,096M (Jacobiano)
 * Factor teorico: ~7.7x mas rapido (en practica 3-5x por overheads)
 */
ECPoint ec_scalar_mult_jacobian(const BigInt& k, const ECPoint& P) {
    if (k == 0 || P.is_infinity()) {
        return ECPoint(P.curve());
    }
    
    const CurveParams* curve = P.curve();
    
    // Paso 1: Convertir a Jacobiano
    JacobianPoint result(curve);  // Infinito
    JacobianPoint addend = to_jacobian(P);
    
    BigInt k_copy = k % curve->n;
    
    // Paso 2: Double-and-add en Jacobiano (sin inversiones)
    while (k_copy > 0) {
        if (k_copy % 2 == 1) {
            result = jacobian_add(result, addend);
        }
        addend = jacobian_double(addend);
        k_copy /= 2;
    }
    
    // Paso 3: Convertir resultado a afin (UNA inversion)
    return to_affine(result);
}

// ============================================================================
// GENERACION DE CLAVES
// ============================================================================

ECKeyPair generate_keypair(const CurveParams& curve, RNG& rng,
                           bool use_jacobian) {
    BigInt private_key = rng.random_range(to_ZZ(1), curve.n - 1);
    
    ECPoint G(curve.Gx, curve.Gy, &curve);
    ECPoint public_key = use_jacobian 
        ? ec_scalar_mult_jacobian(private_key, G)
        : ec_scalar_mult(private_key, G);
    
    return ECKeyPair{private_key, public_key, &curve};
}

void ECKeyPair::print(bool show_private) const {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "PAR DE CLAVES ECC\n";
    std::cout << std::string(70, '=') << "\n";
    std::cout << "Curva: " << curve->name << "\n";
    
    if (show_private) {
        std::cout << "Clave privada (d): " << private_key << "\n";
    } else {
        std::cout << "Clave privada: [OCULTA]\n";
    }
    
    std::cout << "\nClave publica (Q):\n";
    public_key.print();
    std::cout << std::string(70, '=') << "\n";
}

// ============================================================================
// ECDH
// ============================================================================

ECPoint ecdh_shared_secret(const BigInt& private_key, 
                           const ECPoint& public_key,
                           bool use_jacobian) {
    return use_jacobian
        ? ec_scalar_mult_jacobian(private_key, public_key)
        : ec_scalar_mult(private_key, public_key);
}

BigInt ecdh_derive_key(const ECPoint& shared_point, int key_bits) {
    if (shared_point.is_infinity()) {
        throw std::runtime_error("Cannot derive key from point at infinity");
    }
    
    BigInt key = shared_point.x();
    
    if (key_bits < NumBits(key)) {
        key = key % power2_ZZ(key_bits);
    }
    
    return key;
}

// ============================================================================
// ECDSA - FIRMA DIGITAL
// ============================================================================

bool ECDSASignature::is_valid_format(const BigInt& n) const {
    return (r > 0 && r < n) && (s > 0 && s < n);
}

void ECDSASignature::print() const {
    std::cout << "Firma ECDSA:\n";
    std::cout << "  r = " << r << "\n";
    std::cout << "  s = " << s << "\n";
}

BigInt truncate_hash(const BigInt& hash, const BigInt& n) {
    long n_bits = NumBits(n);
    long hash_bits = NumBits(hash);
    
    if (hash_bits > n_bits) {
        return hash >> (hash_bits - n_bits);
    }
    
    return hash;
}

ECDSASignature ecdsa_sign_hash(const BigInt& hash_value,
                               const BigInt& private_key,
                               const CurveParams& curve,
                               RNG& rng,
                               bool use_jacobian) {
    if (private_key <= 0 || private_key >= curve.n) {
        throw std::invalid_argument("Private key must be in range [1, n-1]");
    }
    
    ECPoint G(curve.Gx, curve.Gy, &curve);
    BigInt z = truncate_hash(hash_value, curve.n);
    
    ECDSASignature sig;
    
    while (true) {
        BigInt k = rng.random_range(to_ZZ(1), curve.n - 1);
        
        ECPoint kG = use_jacobian
            ? ec_scalar_mult_jacobian(k, G)
            : ec_scalar_mult(k, G);
        
        if (kG.is_infinity()) continue;
        
        sig.r = kG.x() % curve.n;
        if (sig.r == 0) continue;
        
        BigInt k_inv = InvMod(k, curve.n);
        sig.s = (k_inv * ((z + sig.r * private_key) % curve.n)) % curve.n;
        
        if (sig.s == 0) continue;
        break;
    }
    
    return sig;
}

ECDSASignature ecdsa_sign(const std::string& message,
                          const BigInt& private_key,
                          const CurveParams& curve,
                          RNG& rng,
                          bool use_jacobian) {
    BigInt hash_value = SHA256::hash_to_bigint(message);
    return ecdsa_sign_hash(hash_value, private_key, curve, rng, use_jacobian);
}

bool ecdsa_verify_hash(const BigInt& hash_value,
                       const ECDSASignature& signature,
                       const ECPoint& public_key,
                       const CurveParams& curve,
                       bool use_jacobian) {
    if (!signature.is_valid_format(curve.n)) return false;
    if (public_key.is_infinity() || !public_key.is_on_curve()) return false;
    
    BigInt z = truncate_hash(hash_value, curve.n);
    BigInt w = InvMod(signature.s, curve.n);
    BigInt u1 = (z * w) % curve.n;
    BigInt u2 = (signature.r * w) % curve.n;
    
    ECPoint G(curve.Gx, curve.Gy, &curve);
    
    ECPoint u1G = use_jacobian ? ec_scalar_mult_jacobian(u1, G) : ec_scalar_mult(u1, G);
    ECPoint u2Q = use_jacobian ? ec_scalar_mult_jacobian(u2, public_key) : ec_scalar_mult(u2, public_key);
    
    ECPoint point = ec_add(u1G, u2Q);
    
    if (point.is_infinity()) return false;
    
    BigInt v = point.x() % curve.n;
    return v == signature.r;
}

bool ecdsa_verify(const std::string& message,
                  const ECDSASignature& signature,
                  const ECPoint& public_key,
                  const CurveParams& curve,
                  bool use_jacobian) {
    BigInt hash_value = SHA256::hash_to_bigint(message);
    return ecdsa_verify_hash(hash_value, signature, public_key, curve, use_jacobian);
}

// ============================================================================
// UTILIDADES
// ============================================================================

std::string curve_type_to_string(CurveType type) {
    switch (type) {
        case CurveType::NIST_P256:  return "NIST P-256";
        case CurveType::NIST_P384:  return "NIST P-384";
        case CurveType::SECP256K1:  return "secp256k1";
        case CurveType::CUSTOM:     return "Custom";
        default:                    return "Unknown";
    }
}

int ecc_to_rsa_security(int curve_bits) {
    if (curve_bits <= 160) return 1024;
    if (curve_bits <= 224) return 2048;
    if (curve_bits <= 256) return 3072;
    if (curve_bits <= 384) return 7680;
    if (curve_bits <= 521) return 15360;
    return 15360;
}

CurveType rsa_to_ecc_curve(int rsa_bits) {
    if (rsa_bits <= 3072) return CurveType::NIST_P256;
    if (rsa_bits <= 4096) return CurveType::NIST_P384;
    return CurveType::NIST_P384;
}

} // namespace crypto