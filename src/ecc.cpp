// ecc.cpp
// ImplementaciÃ³n de Elliptic Curve Cryptography (ECC)
// 
// Autor: Leon Elliott Fuller
// Fecha: 2026-01-04

#include "ecc.hpp"
#include "sha256.hpp"
#include <iostream>
#include <iomanip>
#include <stdexcept>

namespace crypto {

// ============================================================================
// PARÃMETROS DE CURVAS ESTÃNDAR
// ============================================================================

/**
 * @brief Obtiene parÃ¡metros de curvas elÃ­pticas estÃ¡ndar
 * 
 * Fuentes:
 * - NIST P-256: FIPS 186-4
 * - NIST P-384: FIPS 186-4
 * - secp256k1: SEC 2
 */
CurveParams get_curve_params(CurveType type) {
    CurveParams params;
    
    switch (type) {
        case CurveType::NIST_P256: {
            // NIST P-256 (secp256r1)
            // Curva: yÂ² = xÂ³ - 3x + b (mod p)
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
            
            // Cofactor
            params.h = to_ZZ(1);
            
            break;
        }
        
        case CurveType::NIST_P384: {
            // NIST P-384
            // Curva: yÂ² = xÂ³ - 3x + b (mod p)
            // Seguridad equivalente: RSA-7680
            
            params.name = "NIST P-384";
            params.bits = 384;
            
            // p = 2^384 - 2^128 - 2^96 + 2^32 - 1
            conv(params.p, "39402006196394479212279040100143613805079739270465446667948293404245721771496870329047266088258938001861606973112319");
            
            // a = -3 (mod p)
            conv(params.a, "39402006196394479212279040100143613805079739270465446667948293404245721771496870329047266088258938001861606973112316");
            
            // b
            conv(params.b, "27580193559959705877849011840389048093056905856361568521428707301988689241309860865136260764883745107765439761230575");
            
            // Generator G
            conv(params.Gx, "26247035095799689268623156744566981891852923491109213387815615900925518854738050089022388053975719786650872476732087");
            conv(params.Gy, "8325710961489029985546751289520108179287853048861315594709205902480503199884419224438643760392947333078086511627871");
            
            // Order n
            conv(params.n, "39402006196394479212279040100143613805079739270465446667946905279627659399113263569398956308152294913554433653942643");
            
            // Cofactor
            params.h = to_ZZ(1);
            
            break;
        }
        
        case CurveType::SECP256K1: {
            // secp256k1 (Bitcoin/Ethereum)
            // Curva: yÂ² = xÂ³ + 7 (mod p)
            // a = 0, b = 7
            // Seguridad equivalente: RSA-3072
            
            params.name = "secp256k1 (Bitcoin)";
            params.bits = 256;
            
            // p = 2^256 - 2^32 - 977
            conv(params.p, "115792089237316195423570985008687907853269984665640564039457584007908834671663");
            
            // a = 0
            params.a = to_ZZ(0);
            
            // b = 7
            params.b = to_ZZ(7);
            
            // Generator G
            conv(params.Gx, "55066263022277343669578718895168534326250603453777594175500187360389116729240");
            conv(params.Gy, "32670510020758816978083085130507043184471273380659243275938904335757337482424");
            
            // Order n
            conv(params.n, "115792089237316195423570985008687907852837564279074904382605163141518161494337");
            
            // Cofactor
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
    // Validaciones bÃ¡sicas
    
    // 1. p debe ser primo (simplificado: solo verificamos que sea > 3)
    if (p <= 3) return false;
    
    // 2. Verificar discriminante: 4aÂ³ + 27bÂ² â‰  0 (mod p)
    ZZ_p::init(p);
    ZZ_p a_p = conv<ZZ_p>(a);
    ZZ_p b_p = conv<ZZ_p>(b);
    
    ZZ_p discriminant = 4 * power(a_p, 3) + 27 * power(b_p, 2);
    if (IsZero(discriminant)) return false;
    
    // 3. Verificar que G estÃ¡ en la curva
    ZZ_p Gx_p = conv<ZZ_p>(Gx);
    ZZ_p Gy_p = conv<ZZ_p>(Gy);
    
    ZZ_p lhs = power(Gy_p, 2);
    ZZ_p rhs = power(Gx_p, 3) + a_p * Gx_p + b_p;
    
    if (lhs != rhs) return false;
    
    // 4. n debe ser > 0
    if (n <= 0) return false;
    
    // 5. h debe ser > 0
    if (h <= 0) return false;
    
    return true;
}

void CurveParams::print() const {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "CURVA: " << name << "\n";
    std::cout << std::string(70, '=') << "\n";
    std::cout << "Bits:       " << bits << "\n";
    std::cout << "p (mÃ³dulo): " << p << "\n";
    std::cout << "a:          " << a << "\n";
    std::cout << "b:          " << b << "\n";
    std::cout << "Gx:         " << Gx << "\n";
    std::cout << "Gy:         " << Gy << "\n";
    std::cout << "n (orden):  " << n << "\n";
    std::cout << "h (cofact): " << h << "\n";
    std::cout << std::string(70, '=') << "\n";
}

// ============================================================================
// ECPoint - IMPLEMENTACIÃ“N
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
    
    // Verificar: yÂ² = xÂ³ + ax + b (mod p)
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
    // Verificar que estÃ¡n en la misma curva
    if (curve_ != other.curve_) return false;
    
    // Ambos infinito
    if (is_infinity_ && other.is_infinity_) return true;
    
    // Uno infinito, otro no
    if (is_infinity_ != other.is_infinity_) return false;
    
    // Comparar coordenadas
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
// OPERACIONES
// ============================================================================

ECPoint ec_add(const ECPoint& P, const ECPoint& Q) {
    // Verificar misma curva
    if (P.curve() != Q.curve()) {
        throw std::invalid_argument("Points must be on the same curve");
    }
    
    const CurveParams* curve = P.curve();
    
    // Caso 1: P es infinito â†’ return Q
    if (P.is_infinity()) return Q;
    
    // Caso 2: Q es infinito â†’ return P
    if (Q.is_infinity()) return P;
    
    // Caso 3: P = -Q â†’ return infinito
    if (P.x() == Q.x() && P.y() != Q.y()) {
        return ECPoint(curve);  // Punto infinito
    }
    
    // Caso 4: P = Q â†’ duplicaciÃ³n
    if (P == Q) {
        return ec_double(P);
    }
    
    // Caso 5: P â‰  Q, suma general
    ZZ_p::init(curve->p);
    
    ZZ_p x1 = conv<ZZ_p>(P.x());
    ZZ_p y1 = conv<ZZ_p>(P.y());
    ZZ_p x2 = conv<ZZ_p>(Q.x());
    ZZ_p y2 = conv<ZZ_p>(Q.y());
    
    // Î» = (y2 - y1) / (x2 - x1)
    ZZ_p lambda = (y2 - y1) / (x2 - x1);
    
    // x3 = Î»Â² - x1 - x2
    ZZ_p x3 = power(lambda, 2) - x1 - x2;
    
    // y3 = Î»(x1 - x3) - y1
    ZZ_p y3 = lambda * (x1 - x3) - y1;
    
    return ECPoint(conv<BigInt>(x3), conv<BigInt>(y3), curve);
}

ECPoint ec_double(const ECPoint& P) {
    if (P.is_infinity()) return P;
    
    const CurveParams* curve = P.curve();
    
    // Si y = 0, retornar infinito
    if (P.y() == 0) {
        return ECPoint(curve);
    }
    
    ZZ_p::init(curve->p);
    
    ZZ_p x = conv<ZZ_p>(P.x());
    ZZ_p y = conv<ZZ_p>(P.y());
    ZZ_p a = conv<ZZ_p>(curve->a);
    
    // Î» = (3xÂ² + a) / (2y)
    ZZ_p lambda = (3 * power(x, 2) + a) / (2 * y);
    
    // x3 = Î»Â² - 2x
    ZZ_p x3 = power(lambda, 2) - 2 * x;
    
    // y3 = Î»(x - x3) - y
    ZZ_p y3 = lambda * (x - x3) - y;
    
    return ECPoint(conv<BigInt>(x3), conv<BigInt>(y3), curve);
}

ECPoint ec_negate(const ECPoint& P) {
    if (P.is_infinity()) return P;
    
    const CurveParams* curve = P.curve();
    
    // -P = (x, -y mod p)
    BigInt neg_y = (curve->p - P.y()) % curve->p;
    
    return ECPoint(P.x(), neg_y, curve);
}

ECPoint ec_scalar_mult(const BigInt& k, const ECPoint& P) {
    if (k == 0 || P.is_infinity()) {
        return ECPoint(P.curve());  // Infinito
    }
    
    const CurveParams* curve = P.curve();
    
    // Algoritmo double-and-add
    ECPoint result(curve);  // Empieza en infinito
    ECPoint addend = P;
    
    BigInt k_copy = k % curve->n;  // Reducir mÃ³dulo n
    
    // Recorrer bits de k de derecha a izquierda
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
// GENERACIÃ“N DE CLAVES
// ============================================================================

ECKeyPair generate_keypair(const CurveParams& curve, RNG& rng) {
    // 1. Generar clave privada: d âˆˆ [1, n-1]
    BigInt private_key = rng.random_range(to_ZZ(1), curve.n - 1);
    
    // 2. Calcular clave pÃºblica: Q = d*G
    ECPoint G(curve.Gx, curve.Gy, &curve);
    ECPoint public_key = ec_scalar_mult(private_key, G);
    
    // 3. Retornar usando brace initialization (aggregate initialization)
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
    
    std::cout << "\nClave pÃºblica (Q):\n";
    public_key.print();
    std::cout << std::string(70, '=') << "\n";
}

// ============================================================================
// ECDH
// ============================================================================

ECPoint ecdh_shared_secret(const BigInt& private_key, 
                           const ECPoint& public_key) {
    // S = private_key * public_key
    return ec_scalar_mult(private_key, public_key);
}

BigInt ecdh_derive_key(const ECPoint& shared_point, int key_bits) {
    if (shared_point.is_infinity()) {
        throw std::runtime_error("Cannot derive key from point at infinity");
    }
    
    // DerivaciÃ³n simple: usar coordenada x como clave
    // En producciÃ³n: usar KDF como HKDF o PBKDF2
    
    // Tomar solo los bits necesarios
    BigInt key = shared_point.x();
    
    if (key_bits < NumBits(key)) {
        // Truncar a los bits necesarios
        key = key % power2_ZZ(key_bits);
    }
    
    return key;
}

// ============================================================================
// ECDSA - FIRMA DIGITAL (FIPS 186-4)
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
    // Si el hash tiene más bits que n, truncar los más significativos
    long n_bits = NumBits(n);
    long hash_bits = NumBits(hash);
    
    if (hash_bits > n_bits) {
        // Desplazar a la derecha para truncar
        return hash >> (hash_bits - n_bits);
    }
    
    return hash;
}

ECDSASignature ecdsa_sign_hash(const BigInt& hash_value,
                               const BigInt& private_key,
                               const CurveParams& curve,
                               RNG& rng) {
    // Validar clave privada
    if (private_key <= 0 || private_key >= curve.n) {
        throw std::invalid_argument("Private key must be in range [1, n-1]");
    }
    
    // Punto generador
    ECPoint G(curve.Gx, curve.Gy, &curve);
    
    // Truncar hash al tamaño del orden de la curva
    BigInt z = truncate_hash(hash_value, curve.n);
    
    ECDSASignature sig;
    
    // Bucle hasta encontrar firma valida (r != 0 && s != 0)
    while (true) {
        // 1. Seleccionar k aleatorio en [1, n-1]
        BigInt k = rng.random_range(to_ZZ(1), curve.n - 1);
        
        // 2. Calcular (x1, y1) = k * G
        ECPoint kG = ec_scalar_mult(k, G);
        
        if (kG.is_infinity()) continue;
        
        // 3. r = x1 mod n
        sig.r = kG.x() % curve.n;
        if (sig.r == 0) continue;
        
        // 4. s = k⁻¹ · (z + r·d) mod n
        BigInt k_inv = InvMod(k, curve.n);
        sig.s = (k_inv * ((z + sig.r * private_key) % curve.n)) % curve.n;
        
        if (sig.s == 0) continue;
        
        // Firma valida encontrada
        break;
    }
    
    return sig;
}

ECDSASignature ecdsa_sign(const std::string& message,
                          const BigInt& private_key,
                          const CurveParams& curve,
                          RNG& rng) {
    // 1. Calcular hash SHA-256 del mensaje
    BigInt hash_value = SHA256::hash_to_bigint(message);
    
    // 2. Firmar el hash
    return ecdsa_sign_hash(hash_value, private_key, curve, rng);
}

bool ecdsa_verify_hash(const BigInt& hash_value,
                       const ECDSASignature& signature,
                       const ECPoint& public_key,
                       const CurveParams& curve) {
    // 1. Verificar formato de la firma
    if (!signature.is_valid_format(curve.n)) {
        return false;
    }
    
    // 2. Verificar que la clave pública está en la curva y no es infinito
    if (public_key.is_infinity() || !public_key.is_on_curve()) {
        return false;
    }
    
    // 3. Truncar hash al tamaño del orden
    BigInt z = truncate_hash(hash_value, curve.n);
    
    // 4. w = s⁻¹ mod n
    BigInt w = InvMod(signature.s, curve.n);
    
    // 5. u1 = z·w mod n
    BigInt u1 = (z * w) % curve.n;
    
    // 6. u2 = r·w mod n
    BigInt u2 = (signature.r * w) % curve.n;
    
    // 7. (x1, y1) = u1·G + u2·Q
    ECPoint G(curve.Gx, curve.Gy, &curve);
    
    ECPoint u1G = ec_scalar_mult(u1, G);
    ECPoint u2Q = ec_scalar_mult(u2, public_key);
    ECPoint point = ec_add(u1G, u2Q);
    
    // 8. Si el punto resultante es infinito, firma invalida
    if (point.is_infinity()) {
        return false;
    }
    
    // 9. Firma valida si r ≡ x1 (mod n)
    BigInt v = point.x() % curve.n;
    
    return v == signature.r;
}

bool ecdsa_verify(const std::string& message,
                  const ECDSASignature& signature,
                  const ECPoint& public_key,
                  const CurveParams& curve) {
    // 1. Calcular hash SHA-256 del mensaje
    BigInt hash_value = SHA256::hash_to_bigint(message);
    
    // 2. Verificar la firma
    return ecdsa_verify_hash(hash_value, signature, public_key, curve);
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
    // Mapeo de seguridad ECC â†’ RSA
    // Fuente: NIST SP 800-57
    
    if (curve_bits <= 160) return 1024;   // DÃ©bil
    if (curve_bits <= 224) return 2048;   // MÃ­nimo actual
    if (curve_bits <= 256) return 3072;   // EstÃ¡ndar
    if (curve_bits <= 384) return 7680;   // Alta seguridad
    if (curve_bits <= 521) return 15360;  // Muy alta
    
    return 15360;  // MÃ¡ximo
}

CurveType rsa_to_ecc_curve(int rsa_bits) {
    // Mapeo RSA â†’ Curva ECC recomendada
    
    if (rsa_bits <= 1024) return CurveType::NIST_P256;  // MÃ­nimo
    if (rsa_bits <= 2048) return CurveType::NIST_P256;
    if (rsa_bits <= 3072) return CurveType::NIST_P256;
    if (rsa_bits <= 4096) return CurveType::NIST_P384;
    
    return CurveType::NIST_P384;  // MÃ¡ximo soportado
}

} // namespace crypto