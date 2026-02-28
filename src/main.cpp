// main.cpp
// Programa principal para benchmarking de RSA y ECC
// Incluye: generacion de claves, cifrado/descifrado, firma/verificacion,
//          ECDH, ECDSA, y comparacion RSA vs ECC
//
// Autor: Leon Elliott Fuller
// Fecha: 2026-02-28

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <numeric>
#include <iomanip>
#include <unistd.h>
#include <memory>
#include <algorithm>
#include <cmath>

#include "common.hpp"
#include "rng.hpp"
#include "rsa.hpp"
#include "ecc.hpp"
#include "sha256.hpp"

using namespace crypto;
using namespace std;
using namespace std::chrono;

// ============================================================================
// UTILIDADES DE BENCHMARKING
// ============================================================================

/**
 * @brief Estructura para almacenar resultados de benchmark
 */
struct BenchmarkResult {
    string operation;
    long long avg_us;      // Tiempo promedio en microsegundos
    long long min_us;      // Tiempo minimo
    long long max_us;      // Tiempo maximo
    long long median_us;   // Mediana
    long long total_us;    // Tiempo total
    double stddev_us;      // Desviacion estandar
    int iterations;
    
    void print() const {
        cout << "\n=== " << operation << " ===\n"
             << "  Iterations:   " << iterations << "\n"
             << "  Average:      " << avg_us << " us\n"
             << "  Median:       " << median_us << " us\n"
             << "  Min:          " << min_us << " us\n"
             << "  Max:          " << max_us << " us\n"
             << "  Std Dev:      " << fixed << setprecision(1) << stddev_us << " us\n"
             << "  Total:        " << total_us << " us\n";
    }
    
    void print_compact() const {
        cout << "  " << left << setw(45) << operation 
             << right << setw(10) << avg_us << " us avg"
             << setw(10) << median_us << " us med"
             << setw(10) << min_us << " us min\n";
    }
};

/**
 * @brief Ejecuta un benchmark generico y retorna estadisticas
 */
template<typename Func>
BenchmarkResult run_benchmark(const string& label, Func operation, int iterations) {
    vector<long long> times;
    times.reserve(iterations);
    
    cout << "Running " << label << " (" << iterations << " iterations)..." << flush;
    
    for (int i = 0; i < iterations; ++i) {
        auto start = high_resolution_clock::now();
        operation();
        auto end = high_resolution_clock::now();
        
        long long elapsed = duration_cast<microseconds>(end - start).count();
        times.push_back(elapsed);
        
        // Mostrar progreso cada 10%
        if (iterations >= 10 && (i + 1) % (iterations / 10) == 0) {
            cout << "." << flush;
        }
    }
    
    cout << " Done!\n";
    
    // Calcular estadisticas
    long long total = accumulate(times.begin(), times.end(), 0LL);
    long long avg = total / iterations;
    long long min_time = *min_element(times.begin(), times.end());
    long long max_time = *max_element(times.begin(), times.end());
    
    // Mediana
    vector<long long> sorted_times = times;
    sort(sorted_times.begin(), sorted_times.end());
    long long median = sorted_times[iterations / 2];
    
    // Desviacion estandar
    double sum_sq = 0;
    for (auto t : times) {
        double diff = static_cast<double>(t - avg);
        sum_sq += diff * diff;
    }
    double stddev = sqrt(sum_sq / iterations);
    
    BenchmarkResult result;
    result.operation = label;
    result.avg_us = avg;
    result.min_us = min_time;
    result.max_us = max_time;
    result.median_us = median;
    result.total_us = total;
    result.stddev_us = stddev;
    result.iterations = iterations;
    
    return result;
}

// ============================================================================
// BENCHMARKS RSA
// ============================================================================

/**
 * @brief Benchmark de generacion de claves RSA
 */
BenchmarkResult benchmark_rsa_keygen(RNG& rng, int bits, int iterations) {
    string label = "RSA Key Generation (" + to_string(bits) + "-bit)";
    
    return run_benchmark(label, [&]() {
        RSA::generate_key(rng, bits);
    }, iterations);
}

/**
 * @brief Benchmark de cifrado/descifrado RSA
 */
vector<BenchmarkResult> benchmark_rsa_encrypt_decrypt(RNG& rng, int bits, int iterations) {
    vector<BenchmarkResult> results;
    
    // Generar un par de claves para las pruebas
    cout << "\nGenerating RSA keypair for encryption/decryption tests...\n";
    auto keypair = RSA::generate_key(rng, bits);
    
    // Generar mensaje aleatorio
    BigInt message = rng.random_bnd(keypair.public_key.n);
    
    // Benchmark de cifrado
    BigInt ciphertext;
    auto encrypt_result = run_benchmark("RSA Encryption (" + to_string(bits) + "-bit)", 
        [&]() {
            ciphertext = RSA::encrypt(message, keypair.public_key);
        }, iterations);
    results.push_back(encrypt_result);
    
    // Benchmark de descifrado (sin CRT)
    auto decrypt_result = run_benchmark("RSA Decryption (" + to_string(bits) + "-bit, no CRT)",
        [&]() {
            RSA::decrypt(ciphertext, keypair.private_key, false);
        }, iterations);
    results.push_back(decrypt_result);
    
    // Benchmark de descifrado (con CRT)
    auto decrypt_crt_result = run_benchmark("RSA Decryption (" + to_string(bits) + "-bit, with CRT)",
        [&]() {
            RSA::decrypt(ciphertext, keypair.private_key, true);
        }, iterations);
    results.push_back(decrypt_crt_result);
    
    return results;
}

/**
 * @brief Benchmark de firma/verificacion RSA
 */
vector<BenchmarkResult> benchmark_rsa_sign_verify(RNG& rng, int bits, int iterations) {
    vector<BenchmarkResult> results;
    
    cout << "\nGenerating RSA keypair for sign/verify tests...\n";
    auto keypair = RSA::generate_key(rng, bits);
    
    // Hash del mensaje usando SHA-256
    string test_message = "Benchmark test message for RSA signature";
    BigInt message_hash = SHA256::hash_to_bigint(test_message);
    
    // Asegurar que el hash es menor que n
    message_hash = message_hash % keypair.public_key.n;
    
    // Benchmark de firma RSA
    BigInt signature;
    auto sign_result = run_benchmark("RSA Sign (" + to_string(bits) + "-bit, CRT)",
        [&]() {
            signature = RSA::sign(message_hash, keypair.private_key, true);
        }, iterations);
    results.push_back(sign_result);
    
    // Benchmark de verificacion RSA
    auto verify_result = run_benchmark("RSA Verify (" + to_string(bits) + "-bit)",
        [&]() {
            RSA::verify(message_hash, signature, keypair.public_key);
        }, iterations);
    results.push_back(verify_result);
    
    return results;
}

/**
 * @brief Benchmark completo de RSA
 */
void benchmark_rsa(RNG& rng, int bits, int iterations) {
    cout << "\n" << string(80, '=') << "\n";
    cout << "RSA BENCHMARK - " << bits << " bits\n";
    cout << string(80, '=') << "\n";
    
    vector<BenchmarkResult> results;
    
    // Benchmark de generacion de claves
    results.push_back(benchmark_rsa_keygen(rng, bits, iterations));
    
    // Benchmark de cifrado/descifrado
    auto enc_dec_results = benchmark_rsa_encrypt_decrypt(rng, bits, iterations);
    results.insert(results.end(), enc_dec_results.begin(), enc_dec_results.end());
    
    // Benchmark de firma/verificacion
    auto sign_verify_results = benchmark_rsa_sign_verify(rng, bits, iterations);
    results.insert(results.end(), sign_verify_results.begin(), sign_verify_results.end());
    
    // Imprimir todos los resultados
    cout << "\n" << string(80, '-') << "\n";
    cout << "RESULTS SUMMARY - RSA " << bits << "-bit\n";
    cout << string(80, '-') << "\n";
    
    for (const auto& result : results) {
        result.print();
    }
    
    // Tabla compacta
    cout << "\n" << string(80, '-') << "\n";
    cout << "COMPACT SUMMARY\n";
    cout << string(80, '-') << "\n";
    for (const auto& result : results) {
        result.print_compact();
    }
    
    cout << "\n" << string(80, '=') << "\n";
}

// ============================================================================
// BENCHMARKS ECC
// ============================================================================

/**
 * @brief Benchmark de generacion de claves ECC
 */
BenchmarkResult benchmark_ecc_keygen(RNG& rng, CurveType curve_type, int iterations) {
    CurveParams curve = get_curve_params(curve_type);
    string label = "ECC Key Generation (" + curve.name + ")";
    
    return run_benchmark(label, [&]() {
        generate_keypair(curve, rng);
    }, iterations);
}

/**
 * @brief Benchmark de ECDH
 */
vector<BenchmarkResult> benchmark_ecc_ecdh(RNG& rng, CurveType curve_type, int iterations) {
    vector<BenchmarkResult> results;
    CurveParams curve = get_curve_params(curve_type);
    
    // Generar claves de Alice y Bob para las pruebas
    cout << "\nGenerating ECC keypairs for ECDH tests...\n";
    ECKeyPair alice = generate_keypair(curve, rng);
    ECKeyPair bob = generate_keypair(curve, rng);
    
    // Benchmark de calculo de secreto compartido
    auto ecdh_result = run_benchmark("ECDH Shared Secret (" + curve.name + ")",
        [&]() {
            ecdh_shared_secret(alice.private_key, bob.public_key);
        }, iterations);
    results.push_back(ecdh_result);
    
    // Benchmark de derivacion de clave
    ECPoint shared = ecdh_shared_secret(alice.private_key, bob.public_key);
    auto derive_result = run_benchmark("ECDH Key Derivation (" + curve.name + ")",
        [&]() {
            ecdh_derive_key(shared, 256);
        }, iterations);
    results.push_back(derive_result);
    
    return results;
}

/**
 * @brief Benchmark de ECDSA (firma digital)
 */
vector<BenchmarkResult> benchmark_ecc_ecdsa(RNG& rng, CurveType curve_type, int iterations) {
    vector<BenchmarkResult> results;
    CurveParams curve = get_curve_params(curve_type);
    
    cout << "\nGenerating ECC keypair for ECDSA tests...\n";
    ECKeyPair kp = generate_keypair(curve, rng);
    
    string test_message = "Benchmark test message for ECDSA signature verification";
    
    // Benchmark de hash SHA-256
    auto hash_result = run_benchmark("SHA-256 Hash",
        [&]() {
            SHA256::hash_to_bigint(test_message);
        }, iterations);
    results.push_back(hash_result);
    
    // Benchmark de firma ECDSA
    ECDSASignature signature;
    auto sign_result = run_benchmark("ECDSA Sign (" + curve.name + ")",
        [&]() {
            signature = ecdsa_sign(test_message, kp.private_key, curve, rng);
        }, iterations);
    results.push_back(sign_result);
    
    // Verificar que la firma es correcta antes del benchmark
    bool valid = ecdsa_verify(test_message, signature, kp.public_key, curve);
    if (!valid) {
        cerr << "ERROR: ECDSA signature failed verification!\n";
        return results;
    }
    
    // Benchmark de verificacion ECDSA
    auto verify_result = run_benchmark("ECDSA Verify (" + curve.name + ")",
        [&]() {
            ecdsa_verify(test_message, signature, kp.public_key, curve);
        }, iterations);
    results.push_back(verify_result);
    
    return results;
}

/**
 * @brief Benchmark de operaciones de punto en curva eliptica
 */
vector<BenchmarkResult> benchmark_ecc_point_ops(RNG& rng, CurveType curve_type, int iterations) {
    vector<BenchmarkResult> results;
    CurveParams curve = get_curve_params(curve_type);
    
    ECPoint G(curve.Gx, curve.Gy, &curve);
    
    // Generar puntos para las pruebas
    BigInt k1 = rng.random_range(to_ZZ(1), curve.n - 1);
    BigInt k2 = rng.random_range(to_ZZ(1), curve.n - 1);
    ECPoint P1 = ec_scalar_mult(k1, G);
    ECPoint P2 = ec_scalar_mult(k2, G);
    
    // Benchmark de suma de puntos
    auto add_result = run_benchmark("EC Point Addition (" + curve.name + ")",
        [&]() {
            ec_add(P1, P2);
        }, iterations);
    results.push_back(add_result);
    
    // Benchmark de duplicacion de punto
    auto double_result = run_benchmark("EC Point Doubling (" + curve.name + ")",
        [&]() {
            ec_double(P1);
        }, iterations);
    results.push_back(double_result);
    
    // Benchmark de multiplicacion escalar
    BigInt scalar = rng.random_range(to_ZZ(1), curve.n - 1);
    auto mult_result = run_benchmark("EC Scalar Multiplication (" + curve.name + ")",
        [&]() {
            ec_scalar_mult(scalar, G);
        }, iterations);
    results.push_back(mult_result);
    
    return results;
}

/**
 * @brief Benchmark completo de ECC
 */
void benchmark_ecc(RNG& rng, const string& curve_name, int iterations) {
    cout << "\n" << string(80, '=') << "\n";
    cout << "ECC BENCHMARK - " << curve_name << "\n";
    cout << string(80, '=') << "\n";
    
    // Convertir nombre de curva a tipo
    CurveType curve_type;
    if (curve_name == "P-256" || curve_name == "NIST_P256") {
        curve_type = CurveType::NIST_P256;
    } else if (curve_name == "P-384" || curve_name == "NIST_P384") {
        curve_type = CurveType::NIST_P384;
    } else if (curve_name == "secp256k1") {
        curve_type = CurveType::SECP256K1;
    } else {
        cerr << "Error: Unknown curve " << curve_name << "\n";
        return;
    }
    
    vector<BenchmarkResult> results;
    
    // Benchmark de operaciones de punto
    cout << "\n--- Point Operations ---\n";
    auto point_results = benchmark_ecc_point_ops(rng, curve_type, iterations);
    results.insert(results.end(), point_results.begin(), point_results.end());
    
    // Benchmark de generacion de claves
    cout << "\n--- Key Generation ---\n";
    results.push_back(benchmark_ecc_keygen(rng, curve_type, iterations));
    
    // Benchmark de ECDH
    cout << "\n--- ECDH ---\n";
    auto ecdh_results = benchmark_ecc_ecdh(rng, curve_type, iterations);
    results.insert(results.end(), ecdh_results.begin(), ecdh_results.end());
    
    // Benchmark de ECDSA
    cout << "\n--- ECDSA ---\n";
    auto ecdsa_results = benchmark_ecc_ecdsa(rng, curve_type, iterations);
    results.insert(results.end(), ecdsa_results.begin(), ecdsa_results.end());
    
    // Imprimir resultados detallados
    cout << "\n" << string(80, '-') << "\n";
    cout << "RESULTS SUMMARY - ECC " << curve_name << "\n";
    cout << string(80, '-') << "\n";
    
    for (const auto& result : results) {
        result.print();
    }
    
    // Tabla compacta
    cout << "\n" << string(80, '-') << "\n";
    cout << "COMPACT SUMMARY\n";
    cout << string(80, '-') << "\n";
    for (const auto& result : results) {
        result.print_compact();
    }
    
    cout << "\n" << string(80, '=') << "\n";
}

// ============================================================================
// MODO COMPARACION RSA vs ECC
// ============================================================================

/**
 * @brief Ejecuta una comparacion directa RSA vs ECC
 * 
 * Compara operaciones equivalentes en seguridad:
 * - RSA-3072 vs ECC P-256/secp256k1 (128-bit security)
 */
void benchmark_comparison(RNG& rng, int iterations) {
    cout << "\n" << string(80, '=') << "\n";
    cout << "COMPARISON: RSA vs ECC (equivalent security levels)\n";
    cout << string(80, '=') << "\n";
    
    // Estructura para comparaciones
    struct SecurityLevel {
        string name;
        int rsa_bits;
        CurveType ecc_curve;
        string curve_name;
    };
    
    vector<SecurityLevel> levels = {
        {"128-bit security", 3072, CurveType::NIST_P256, "NIST P-256"},
        {"128-bit security", 3072, CurveType::SECP256K1, "secp256k1"},
    };
    
    for (const auto& level : levels) {
        cout << "\n" << string(70, '-') << "\n";
        cout << level.name << " : RSA-" << level.rsa_bits 
             << " vs ECC " << level.curve_name << "\n";
        cout << string(70, '-') << "\n";
        
        CurveParams curve = get_curve_params(level.ecc_curve);
        
        // --- Generacion de claves ---
        auto rsa_keygen = run_benchmark(
            "RSA-" + to_string(level.rsa_bits) + " KeyGen",
            [&]() { RSA::generate_key(rng, level.rsa_bits); },
            iterations
        );
        
        auto ecc_keygen = run_benchmark(
            "ECC " + level.curve_name + " KeyGen",
            [&]() { generate_keypair(curve, rng); },
            iterations
        );
        
        // --- Firma digital ---
        auto rsa_kp = RSA::generate_key(rng, level.rsa_bits);
        string msg = "Comparison benchmark message";
        BigInt rsa_hash = SHA256::hash_to_bigint(msg) % rsa_kp.public_key.n;
        BigInt rsa_sig;
        
        auto rsa_sign = run_benchmark(
            "RSA-" + to_string(level.rsa_bits) + " Sign",
            [&]() { rsa_sig = RSA::sign(rsa_hash, rsa_kp.private_key, true); },
            iterations
        );
        
        auto rsa_verify = run_benchmark(
            "RSA-" + to_string(level.rsa_bits) + " Verify",
            [&]() { RSA::verify(rsa_hash, rsa_sig, rsa_kp.public_key); },
            iterations
        );
        
        ECKeyPair ecc_kp = generate_keypair(curve, rng);
        ECDSASignature ecc_sig;
        
        auto ecc_sign = run_benchmark(
            "ECDSA " + level.curve_name + " Sign",
            [&]() { ecc_sig = ecdsa_sign(msg, ecc_kp.private_key, curve, rng); },
            iterations
        );
        
        auto ecc_verify = run_benchmark(
            "ECDSA " + level.curve_name + " Verify",
            [&]() { ecdsa_verify(msg, ecc_sig, ecc_kp.public_key, curve); },
            iterations
        );
        
        // --- Resumen ---
        cout << "\n  COMPARISON TABLE:\n";
        cout << "  " << string(68, '-') << "\n";
        cout << "  " << left << setw(25) << "Operation" 
             << right << setw(15) << "RSA (us)"
             << setw(15) << "ECC (us)"
             << setw(13) << "Speedup" << "\n";
        cout << "  " << string(68, '-') << "\n";
        
        auto print_row = [](const string& op, long long rsa_t, long long ecc_t) {
            cout << "  " << left << setw(25) << op
                 << right << setw(15) << rsa_t
                 << setw(15) << ecc_t;
            if (ecc_t > 0 && rsa_t > 0) {
                double ratio = static_cast<double>(rsa_t) / ecc_t;
                if (ratio >= 1.0) {
                    cout << "  " << fixed << setprecision(1) << ratio << "x ECC faster";
                } else {
                    cout << "  " << fixed << setprecision(1) << (1.0/ratio) << "x RSA faster";
                }
            }
            cout << "\n";
        };
        
        print_row("Key Generation", rsa_keygen.avg_us, ecc_keygen.avg_us);
        print_row("Sign", rsa_sign.avg_us, ecc_sign.avg_us);
        print_row("Verify", rsa_verify.avg_us, ecc_verify.avg_us);
        
        cout << "  " << string(68, '-') << "\n";
    }
    
    cout << "\n" << string(80, '=') << "\n";
}

// ============================================================================
// FUNCION PRINCIPAL
// ============================================================================

void print_usage(const char* prog) {
    cout << "Usage: " << prog << " [options]\n"
         << "  -a ALGO      RSA, ECC, or CMP      (default: RSA)\n"
         << "               CMP = comparison mode\n"
         << "  -b BITS      RSA key size           (default: 2048)\n"
         << "  -c CURVE     ECC curve name         (default: secp256k1)\n"
         << "               Options: secp256k1, P-256, P-384\n"
         << "  -i ITERS     total iterations        (default: 50)\n"
         << "  -m MODE      seq | par               (default: seq)\n"
         << "  -p NCPUS     number of CPUs          (default: nproc)\n"
         << "  -s SEED      fixed | random          (default: fixed)\n"
         << "  -h           show this help\n"
         << "\nExamples:\n"
         << "  " << prog << " -a RSA -b 2048 -i 50\n"
         << "  " << prog << " -a ECC -c secp256k1 -i 30\n"
         << "  " << prog << " -a ECC -c P-256 -i 20\n"
         << "  " << prog << " -a CMP -i 10          # RSA vs ECC comparison\n";
}

int main(int argc, char** argv) {
    // Parametros por defecto
    string algo = "RSA";
    int bits = DEFAULT_RSA_BITS;
    string curve = DEFAULT_CURVE;
    int iterations = 50;
    string mode = "seq";
    int ncpus = 1;
    string seed_mode = "fixed";
    
    // Parsear argumentos
    int opt;
    while ((opt = getopt(argc, argv, "a:b:c:i:m:p:s:h")) != -1) {
        switch (opt) {
            case 'a': 
                algo = optarg; 
                break;
            case 'b': 
                bits = stoi(optarg); 
                break;
            case 'c': 
                curve = optarg; 
                break;
            case 'i': 
                iterations = stoi(optarg); 
                break;
            case 'm': 
                mode = optarg; 
                break;
            case 'p': 
                ncpus = stoi(optarg); 
                break;
            case 's': 
                seed_mode = optarg; 
                break;
            case 'h':
            default: 
                print_usage(argv[0]); 
                return (opt == 'h') ? 0 : 1;
        }
    }
    
    // Validar algoritmo
    if (algo != "RSA" && algo != "ECC" && algo != "CMP") {
        cerr << "Error: Algorithm must be RSA, ECC, or CMP\n";
        print_usage(argv[0]);
        return 1;
    }
    
    // Validar seed mode
    if (seed_mode != "fixed" && seed_mode != "random") {
        cerr << "Error: Seed mode must be 'fixed' or 'random'\n";
        print_usage(argv[0]);
        return 1;
    }
    
    // Inicializar RNG
    auto rng_ptr = create_rng(seed_mode, 0);
    auto& rng = *rng_ptr;
    
    // Imprimir configuracion
    cout << "\n" << string(80, '=') << "\n";
    cout << "BENCHMARK CONFIGURATION\n";
    cout << string(80, '=') << "\n";
    cout << "  Algorithm:    " << algo << "\n";
    
    if (algo == "RSA") {
        cout << "  Key size:     " << bits << " bits\n";
    } else if (algo == "ECC") {
        cout << "  Curve:        " << curve << "\n";
    } else {
        cout << "  Mode:         RSA vs ECC Comparison\n";
    }
    
    cout << "  Iterations:   " << iterations << "\n"
         << "  Mode:         " << mode << "\n"
         << "  Seed mode:    " << seed_mode << "\n"
         << "  Seed value:   " << rng.get_seed() << "\n";
    
    if (mode == "par") {
        cout << "  CPUs:         " << ncpus << "\n";
        cout << "\nNOTE: Parallel mode not yet implemented. Running in sequential mode.\n";
    }
    
    cout << string(80, '=') << "\n";
    
    // Ejecutar benchmark
    try {
        if (algo == "RSA") {
            benchmark_rsa(rng, bits, iterations);
        } else if (algo == "ECC") {
            benchmark_ecc(rng, curve, iterations);
        } else if (algo == "CMP") {
            benchmark_comparison(rng, iterations);
        }
    } catch (const CryptoException& e) {
        cerr << "\nCrypto Error: " << e.what() << "\n";
        return 1;
    } catch (const exception& e) {
        cerr << "\nError: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}