// main.cpp
// Programa principal para benchmarking de RSA y ECC
// Autor: Leon Elliott Fuller
// Fecha: 2026-01-04

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <numeric>
#include <iomanip>
#include <unistd.h>
#include <memory>

#include "common.hpp"
#include "rng.hpp"
#include "rsa.hpp"
#include "ecc.hpp"

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
    long long min_us;      // Tiempo mínimo
    long long max_us;      // Tiempo máximo
    long long total_us;    // Tiempo total
    int iterations;
    
    void print() const {
        cout << "\n=== " << operation << " ===\n"
             << "  Iterations:   " << iterations << "\n"
             << "  Average:      " << avg_us << " µs\n"
             << "  Min:          " << min_us << " µs\n"
             << "  Max:          " << max_us << " µs\n"
             << "  Total:        " << total_us << " µs\n";
    }
};

/**
 * @brief Ejecuta un benchmark genérico y retorna estadísticas
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
    
    // Calcular estadísticas
    long long total = accumulate(times.begin(), times.end(), 0LL);
    long long avg = total / iterations;
    long long min_time = *min_element(times.begin(), times.end());
    long long max_time = *max_element(times.begin(), times.end());
    
    BenchmarkResult result;
    result.operation = label;
    result.avg_us = avg;
    result.min_us = min_time;
    result.max_us = max_time;
    result.total_us = total;
    result.iterations = iterations;
    
    return result;
}

// ============================================================================
// BENCHMARKS RSA
// ============================================================================

/**
 * @brief Benchmark de generación de claves RSA
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
 * @brief Benchmark completo de RSA
 */
void benchmark_rsa(RNG& rng, int bits, int iterations) {
    cout << "\n" << string(80, '=') << "\n";
    cout << "RSA BENCHMARK - " << bits << " bits\n";
    cout << string(80, '=') << "\n";
    
    vector<BenchmarkResult> results;
    
    // Benchmark de generación de claves
    results.push_back(benchmark_rsa_keygen(rng, bits, iterations));
    
    // Benchmark de cifrado/descifrado
    auto enc_dec_results = benchmark_rsa_encrypt_decrypt(rng, bits, iterations);
    results.insert(results.end(), enc_dec_results.begin(), enc_dec_results.end());
    
    // Imprimir todos los resultados
    cout << "\n" << string(80, '-') << "\n";
    cout << "RESULTS SUMMARY\n";
    cout << string(80, '-') << "\n";
    
    for (const auto& result : results) {
        result.print();
    }
    
    cout << "\n" << string(80, '=') << "\n";
}

// ============================================================================
// BENCHMARKS ECC
// ============================================================================

/**
 * @brief Benchmark de generación de claves ECC
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
    
    // Benchmark de cálculo de secreto compartido
    auto ecdh_result = run_benchmark("ECDH Shared Secret (" + curve.name + ")",
        [&]() {
            ecdh_shared_secret(alice.private_key, bob.public_key);
        }, iterations);
    results.push_back(ecdh_result);
    
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
    
    // Benchmark de generación de claves
    results.push_back(benchmark_ecc_keygen(rng, curve_type, iterations));
    
    // Benchmark de ECDH
    auto ecdh_results = benchmark_ecc_ecdh(rng, curve_type, iterations);
    results.insert(results.end(), ecdh_results.begin(), ecdh_results.end());
    
    // Imprimir resultados
    cout << "\n" << string(80, '-') << "\n";
    cout << "RESULTS SUMMARY\n";
    cout << string(80, '-') << "\n";
    
    for (const auto& result : results) {
        result.print();
    }
    
    cout << "\n" << string(80, '=') << "\n";
}

// ============================================================================
// FUNCIÓN PRINCIPAL
// ============================================================================

void print_usage(const char* prog) {
    cout << "Usage: " << prog << " [options]\n"
         << "  -a ALGO      RSA or ECC           (default: RSA)\n"
         << "  -b BITS      RSA key size         (default: 2048)\n"
         << "  -c CURVE     ECC curve name       (default: secp256k1)\n"
         << "  -i ITERS     total iterations     (default: 50)\n"
         << "  -m MODE      seq | par            (default: seq)\n"
         << "  -p NCPUS     number of CPUs       (default: nproc)\n"
         << "  -s SEED      fixed | random       (default: fixed)\n"
         << "  -h           show this help\n";
}

int main(int argc, char** argv) {
    // Parámetros por defecto
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
    if (algo != "RSA" && algo != "ECC") {
        cerr << "Error: Algorithm must be RSA or ECC\n";
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
    
    // Imprimir configuración
    cout << "\n" << string(80, '=') << "\n";
    cout << "BENCHMARK CONFIGURATION\n";
    cout << string(80, '=') << "\n";
    cout << "  Algorithm:    " << algo << "\n";
    
    if (algo == "RSA") {
        cout << "  Key size:     " << bits << " bits\n";
    } else {
        cout << "  Curve:        " << curve << "\n";
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