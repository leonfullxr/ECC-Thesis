// main.cpp
// Benchmark engine for RSA vs ECC comparative analysis
// Supports: RSA, ECC (affine), ECC (Jacobian), ECC (binary fields)
// Outputs structured CSV data for visualization
// Author: Leon Elliott Fuller
// Date: 2026-03-18

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <numeric>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <unistd.h>

#include "common.hpp"
#include "rng.hpp"
#include "rsa.hpp"
#include "ecc.hpp"
#include "ecc_binary.hpp"
#include "sha256.hpp"

using namespace crypto;
using namespace std;
using namespace std::chrono;

// ============================================================================
// BENCHMARK INFRASTRUCTURE
// ============================================================================

struct BenchmarkResult {
    string algorithm;       // RSA, ECC, ECC_JACOBIAN, ECC_BINARY
    string operation;       // keygen, sign, verify, encrypt, decrypt, etc.
    string params;          // key size or curve name
    int security_bits;      // equivalent security level in bits
    int iterations;
    vector<long long> times_us;  // individual measurements

    // Computed statistics
    double avg_us;
    double median_us;
    double stddev_us;
    long long min_us;
    long long max_us;
    double p5_us;           // 5th percentile
    double p95_us;          // 95th percentile

    void compute_stats() {
        if (times_us.empty()) return;

        vector<long long> sorted = times_us;
        sort(sorted.begin(), sorted.end());

        long long total = accumulate(sorted.begin(), sorted.end(), 0LL);
        avg_us = static_cast<double>(total) / sorted.size();

        int n = sorted.size();
        median_us = (n % 2 == 0)
            ? (sorted[n/2 - 1] + sorted[n/2]) / 2.0
            : sorted[n/2];

        double variance = 0;
        for (auto t : sorted) {
            double diff = t - avg_us;
            variance += diff * diff;
        }
        stddev_us = sqrt(variance / n);

        min_us = sorted.front();
        max_us = sorted.back();
        p5_us  = sorted[max(0, (int)(n * 0.05))];
        p95_us = sorted[min(n-1, (int)(n * 0.95))];
    }
};

// Number of warm-up iterations discarded before measurement
static const int WARMUP_RUNS = 3;

template<typename Func>
BenchmarkResult run_benchmark(const string& algo, const string& operation,
                              const string& params, int security_bits,
                              Func func, int iterations, bool verbose) {
    BenchmarkResult result;
    result.algorithm = algo;
    result.operation = operation;
    result.params = params;
    result.security_bits = security_bits;
    result.iterations = iterations;

    // Warm-up (not recorded)
    for (int i = 0; i < WARMUP_RUNS; i++) {
        func();
    }

    result.times_us.reserve(iterations);

    if (verbose) {
        cerr << "  " << algo << " " << operation << " [" << params << "] "
             << iterations << " iters..." << flush;
    }

    for (int i = 0; i < iterations; i++) {
        auto start = high_resolution_clock::now();
        func();
        auto end = high_resolution_clock::now();
        long long elapsed = duration_cast<microseconds>(end - start).count();
        result.times_us.push_back(elapsed);
    }

    result.compute_stats();

    if (verbose) {
        cerr << " avg=" << (long long)result.avg_us
             << "us med=" << (long long)result.median_us << "us" << endl;
    }

    return result;
}

// ============================================================================
// CSV OUTPUT
// ============================================================================

void print_csv_header(ostream& out) {
    out << "algorithm,operation,params,security_bits,"
        << "iterations,avg_us,median_us,stddev_us,"
        << "min_us,max_us,p5_us,p95_us" << endl;
}

void print_csv_row(ostream& out, const BenchmarkResult& r) {
    out << r.algorithm << ","
        << r.operation << ","
        << r.params << ","
        << r.security_bits << ","
        << r.iterations << ","
        << fixed << setprecision(1)
        << r.avg_us << ","
        << r.median_us << ","
        << r.stddev_us << ","
        << r.min_us << ","
        << r.max_us << ","
        << r.p5_us << ","
        << r.p95_us << endl;
}

// Also output raw per-iteration data for detailed analysis
void print_raw_csv_header(ostream& out) {
    out << "algorithm,operation,params,security_bits,iteration,time_us" << endl;
}

void print_raw_csv_row(ostream& out, const BenchmarkResult& r) {
    for (int i = 0; i < (int)r.times_us.size(); i++) {
        out << r.algorithm << ","
            << r.operation << ","
            << r.params << ","
            << r.security_bits << ","
            << i << ","
            << r.times_us[i] << endl;
    }
}

// ============================================================================
// RSA BENCHMARKS
// ============================================================================

// Map RSA key size to approximate security level in bits (NIST SP 800-57)
int rsa_security_bits(int key_bits) {
    if (key_bits <= 1024) return 80;
    if (key_bits <= 2048) return 112;
    if (key_bits <= 3072) return 128;
    if (key_bits <= 4096) return 140;
    if (key_bits <= 7680) return 192;
    return 256;
}

vector<BenchmarkResult> benchmark_rsa(RNG& rng, int bits, int iters, bool verbose) {
    vector<BenchmarkResult> results;
    int sec = rsa_security_bits(bits);
    string params = to_string(bits) + "-bit";

    if (verbose) cerr << "\n[RSA-" << bits << "]\n";

    // Key generation
    results.push_back(run_benchmark("RSA", "keygen", params, sec,
        [&]() { RSA::generate_key(rng, bits); }, iters, verbose));

    // Generate keys and test message for remaining benchmarks
    auto keypair = RSA::generate_key(rng, bits);
    BigInt message = rng.random_bnd(keypair.public_key.n);
    if (message < 2) message = to_ZZ(12345);

    // Encrypt
    BigInt ciphertext;
    results.push_back(run_benchmark("RSA", "encrypt", params, sec,
        [&]() { ciphertext = RSA::encrypt(message, keypair.public_key); },
        iters, verbose));
    ciphertext = RSA::encrypt(message, keypair.public_key);

    // Decrypt (no CRT)
    results.push_back(run_benchmark("RSA", "decrypt", params, sec,
        [&]() { RSA::decrypt(ciphertext, keypair.private_key, false); },
        iters, verbose));

    // Decrypt (CRT)
    results.push_back(run_benchmark("RSA", "decrypt_crt", params, sec,
        [&]() { RSA::decrypt(ciphertext, keypair.private_key, true); },
        iters, verbose));

    // Sign (CRT) - uses SHA-256 hash
    string test_msg = "Benchmark test message for digital signature verification";
    BigInt hash_val = SHA256::hash_to_bigint(test_msg);
    hash_val = hash_val % keypair.public_key.n;
    if (hash_val < 1) hash_val = to_ZZ(1);

    results.push_back(run_benchmark("RSA", "sign", params, sec,
        [&]() { RSA::sign(hash_val, keypair.private_key, true); },
        iters, verbose));

    // Verify
    BigInt signature = RSA::sign(hash_val, keypair.private_key, true);
    results.push_back(run_benchmark("RSA", "verify", params, sec,
        [&]() { RSA::verify(hash_val, signature, keypair.public_key); },
        iters, verbose));

    return results;
}

// ============================================================================
// ECC BENCHMARKS (PRIME FIELD - AFFINE COORDINATES)
// ============================================================================

int ecc_security_bits(CurveType type) {
    switch (type) {
        case CurveType::NIST_P256:  return 128;
        case CurveType::SECP256K1:  return 128;
        case CurveType::NIST_P384:  return 192;
        default: return 0;
    }
}

CurveType parse_curve(const string& name) {
    if (name == "P-256" || name == "NIST_P256" || name == "p256") return CurveType::NIST_P256;
    if (name == "P-384" || name == "NIST_P384" || name == "p384") return CurveType::NIST_P384;
    if (name == "secp256k1" || name == "k1")                      return CurveType::SECP256K1;
    throw runtime_error("Unknown prime curve: " + name);
}

// CSV-friendly curve name (no spaces or special chars)
string csv_curve_name(CurveType type) {
    switch (type) {
        case CurveType::NIST_P256:  return "P-256";
        case CurveType::SECP256K1:  return "secp256k1";
        case CurveType::NIST_P384:  return "P-384";
        default: return "custom";
    }
}

vector<BenchmarkResult> benchmark_ecc(RNG& rng, CurveType curve_type,
                                       int iters, bool verbose) {
    vector<BenchmarkResult> results;
    CurveParams curve = get_curve_params(curve_type);
    int sec = ecc_security_bits(curve_type);
    string params = csv_curve_name(curve_type);

    if (verbose) cerr << "\n[ECC-Affine " << params << "]\n";

    // Key generation (affine)
    results.push_back(run_benchmark("ECC", "keygen", params, sec,
        [&]() { generate_keypair(curve, rng, false); }, iters, verbose));

    // Generate keys for remaining benchmarks
    ECKeyPair alice = generate_keypair(curve, rng, false);
    ECKeyPair bob   = generate_keypair(curve, rng, false);

    // Scalar multiplication (core operation)
    ECPoint G(curve.Gx, curve.Gy, &curve);
    BigInt k = rng.random_range(to_ZZ(1), curve.n - 1);
    results.push_back(run_benchmark("ECC", "scalar_mult", params, sec,
        [&]() { ec_scalar_mult(k, G); }, iters, verbose));

    // ECDH - shared secret computation
    results.push_back(run_benchmark("ECC", "ecdh", params, sec,
        [&]() { ecdh_shared_secret(alice.private_key, bob.public_key, false); },
        iters, verbose));

    // ECDSA Sign
    string test_msg = "Benchmark test message for digital signature verification";
    results.push_back(run_benchmark("ECC", "sign", params, sec,
        [&]() { ecdsa_sign(test_msg, alice.private_key, curve, rng, false); },
        iters, verbose));

    // ECDSA Verify
    ECDSASignature sig = ecdsa_sign(test_msg, alice.private_key, curve, rng, false);
    results.push_back(run_benchmark("ECC", "verify", params, sec,
        [&]() { ecdsa_verify(test_msg, sig, alice.public_key, curve, false); },
        iters, verbose));

    // SHA-256 hash (baseline reference)
    results.push_back(run_benchmark("ECC", "sha256_hash", params, sec,
        [&]() { SHA256::hash(test_msg); }, iters, verbose));

    return results;
}

// ============================================================================
// ECC BENCHMARKS (PRIME FIELD - JACOBIAN COORDINATES)
// ============================================================================

/**
 * Benchmarks the same prime field curves but using Jacobian coordinates.
 * This allows direct comparison of affine vs Jacobian performance on
 * identical operations and curve parameters.
 *
 * The algorithm label is "ECC_JACOBIAN" in CSV output so the visualization
 * pipeline can distinguish between the two coordinate systems.
 */
vector<BenchmarkResult> benchmark_ecc_jacobian(RNG& rng, CurveType curve_type,
                                                int iters, bool verbose) {
    vector<BenchmarkResult> results;
    CurveParams curve = get_curve_params(curve_type);
    int sec = ecc_security_bits(curve_type);
    string params = csv_curve_name(curve_type);

    if (verbose) cerr << "\n[ECC-Jacobian " << params << "]\n";

    // Key generation (Jacobian)
    results.push_back(run_benchmark("ECC_JACOBIAN", "keygen", params, sec,
        [&]() { generate_keypair(curve, rng, true); }, iters, verbose));

    // Generate keys for remaining benchmarks (using Jacobian for consistency)
    ECKeyPair alice = generate_keypair(curve, rng, true);
    ECKeyPair bob   = generate_keypair(curve, rng, true);

    // Scalar multiplication (Jacobian)
    ECPoint G(curve.Gx, curve.Gy, &curve);
    BigInt k = rng.random_range(to_ZZ(1), curve.n - 1);
    results.push_back(run_benchmark("ECC_JACOBIAN", "scalar_mult", params, sec,
        [&]() { ec_scalar_mult_jacobian(k, G); }, iters, verbose));

    // ECDH (Jacobian)
    results.push_back(run_benchmark("ECC_JACOBIAN", "ecdh", params, sec,
        [&]() { ecdh_shared_secret(alice.private_key, bob.public_key, true); },
        iters, verbose));

    // ECDSA Sign (Jacobian)
    string test_msg = "Benchmark test message for digital signature verification";
    results.push_back(run_benchmark("ECC_JACOBIAN", "sign", params, sec,
        [&]() { ecdsa_sign(test_msg, alice.private_key, curve, rng, true); },
        iters, verbose));

    // ECDSA Verify (Jacobian)
    // Sign with Jacobian for consistency; the signature (r,s) is identical
    // regardless of coordinate system used internally
    ECDSASignature sig = ecdsa_sign(test_msg, alice.private_key, curve, rng, true);
    results.push_back(run_benchmark("ECC_JACOBIAN", "verify", params, sec,
        [&]() { ecdsa_verify(test_msg, sig, alice.public_key, curve, true); },
        iters, verbose));

    return results;
}

// ============================================================================
// ECC BENCHMARKS (BINARY FIELD GF(2^m))
// ============================================================================

int binary_ecc_security_bits(BinaryCurveType type) {
    switch (type) {
        case BinaryCurveType::SECT163K1: return 80;
        case BinaryCurveType::SECT233K1: return 112;
        case BinaryCurveType::SECT283K1: return 128;
        case BinaryCurveType::SECT233R1: return 112;
        case BinaryCurveType::SECT283R1: return 128;
        default: return 0;
    }
}

BinaryCurveType parse_binary_curve(const string& name) {
    if (name == "sect163k1") return BinaryCurveType::SECT163K1;
    if (name == "sect233k1") return BinaryCurveType::SECT233K1;
    if (name == "sect283k1") return BinaryCurveType::SECT283K1;
    if (name == "sect233r1") return BinaryCurveType::SECT233R1;
    if (name == "sect283r1") return BinaryCurveType::SECT283R1;
    throw runtime_error("Unknown binary curve: " + name);
}

string csv_binary_curve_name(BinaryCurveType type) {
    switch (type) {
        case BinaryCurveType::SECT163K1: return "sect163k1";
        case BinaryCurveType::SECT233K1: return "sect233k1";
        case BinaryCurveType::SECT283K1: return "sect283k1";
        case BinaryCurveType::SECT233R1: return "sect233r1";
        case BinaryCurveType::SECT283R1: return "sect283r1";
        default: return "custom_binary";
    }
}

/**
 * Benchmarks elliptic curve operations over binary fields GF(2^m).
 *
 * Operations benchmarked:
 * - Key generation (scalar multiplication of generator)
 * - Scalar multiplication (core operation)
 * - ECDH shared secret computation
 *
 * Note: ECDSA over binary fields is not benchmarked here because our
 * implementation focuses on the field arithmetic and point operations.
 * The ECDSA algorithm itself is identical to prime fields; the difference
 * lies entirely in the underlying field arithmetic.
 */
vector<BenchmarkResult> benchmark_ecc_binary(RNG& rng, BinaryCurveType curve_type,
                                              int iters, bool verbose) {
    vector<BenchmarkResult> results;
    BinaryCurveParams curve = get_binary_curve_params(curve_type);
    int sec = binary_ecc_security_bits(curve_type);
    string params = csv_binary_curve_name(curve_type);

    if (verbose) cerr << "\n[ECC-Binary " << params << " GF(2^" << curve.m << ")]\n";

    // Initialize the binary field once
    curve.init_field();

    // Key generation
    results.push_back(run_benchmark("ECC_BINARY", "keygen", params, sec,
        [&]() { binary_generate_keypair(curve, rng); }, iters, verbose));

    // Generate keys for remaining benchmarks
    BinaryECKeyPair alice = binary_generate_keypair(curve, rng);
    BinaryECKeyPair bob   = binary_generate_keypair(curve, rng);

    // Scalar multiplication
    GF2E Gx = curve.hex_to_gf2e(curve.Gx_hex);
    GF2E Gy = curve.hex_to_gf2e(curve.Gy_hex);
    BinaryECPoint G(Gx, Gy, &curve);
    BigInt k = rng.random_range(to_ZZ(1), curve.n - 1);
    results.push_back(run_benchmark("ECC_BINARY", "scalar_mult", params, sec,
        [&]() { binary_ec_scalar_mult(k, G, curve.n); }, iters, verbose));

    // ECDH shared secret
    results.push_back(run_benchmark("ECC_BINARY", "ecdh", params, sec,
        [&]() { binary_ecdh_shared_secret(alice.private_key, bob.public_key, curve.n); },
        iters, verbose));

    return results;
}

// ============================================================================
// FULL COMPARISON MODE
// ============================================================================

/**
 * Runs the complete comparative benchmark across all three dimensions:
 *
 * 1. RSA vs ECC (algorithm comparison)
 *    - RSA: 1024, 2048, 3072, 4096 bits
 *    - ECC: secp256k1, P-256, P-384 (affine coordinates)
 *
 * 2. Affine vs Jacobian (coordinate system comparison)
 *    - Same curves benchmarked with both coordinate systems
 *    - Allows direct measurement of Jacobian speedup
 *
 * 3. Prime field vs Binary field (field arithmetic comparison)
 *    - Prime: P-256 (~128 bits), P-384 (~192 bits)
 *    - Binary: sect283k1 (~128 bits), sect233k1 (~112 bits)
 *    - Compared at equivalent security levels
 */
vector<BenchmarkResult> benchmark_comparison(RNG& rng, int iters, bool verbose) {
    vector<BenchmarkResult> all_results;

    if (verbose) {
        cerr << "\n============================================================\n"
             << "FULL RSA vs ECC COMPARATIVE BENCHMARK\n"
             << "Dimensions: RSA vs ECC | Affine vs Jacobian | Fp vs GF(2^m)\n"
             << "============================================================\n";
    }

    // --- Dimension 1: RSA key sizes ---
    if (verbose) cerr << "\n--- RSA benchmarks ---\n";
    vector<int> rsa_sizes = {1024, 2048, 3072, 4096};
    for (int bits : rsa_sizes) {
        auto results = benchmark_rsa(rng, bits, iters, verbose);
        all_results.insert(all_results.end(), results.begin(), results.end());
    }

    // --- Dimension 2a: ECC prime field (affine) ---
    if (verbose) cerr << "\n--- ECC prime field (affine coordinates) ---\n";
    vector<CurveType> prime_curves = {
        CurveType::SECP256K1,
        CurveType::NIST_P256,
        CurveType::NIST_P384
    };
    for (auto ct : prime_curves) {
        auto results = benchmark_ecc(rng, ct, iters, verbose);
        all_results.insert(all_results.end(), results.begin(), results.end());
    }

    // --- Dimension 2b: ECC prime field (Jacobian) ---
    if (verbose) cerr << "\n--- ECC prime field (Jacobian coordinates) ---\n";
    for (auto ct : prime_curves) {
        auto results = benchmark_ecc_jacobian(rng, ct, iters, verbose);
        all_results.insert(all_results.end(), results.begin(), results.end());
    }

    // --- Dimension 3: ECC binary field GF(2^m) ---
    if (verbose) cerr << "\n--- ECC binary field GF(2^m) ---\n";
    vector<BinaryCurveType> binary_curves = {
        BinaryCurveType::SECT163K1,
        BinaryCurveType::SECT233K1,
        BinaryCurveType::SECT283K1,
        BinaryCurveType::SECT233R1,
        BinaryCurveType::SECT283R1
    };
    for (auto bt : binary_curves) {
        auto results = benchmark_ecc_binary(rng, bt, iters, verbose);
        all_results.insert(all_results.end(), results.begin(), results.end());
    }

    return all_results;
}

// ============================================================================
// MAIN
// ============================================================================

void print_usage(const char* prog) {
    cerr << "Usage: " << prog << " [options]\n"
         << "\n"
         << "Modes:\n"
         << "  -a RSA         Benchmark RSA only\n"
         << "  -a ECC         Benchmark ECC (affine coordinates, prime field)\n"
         << "  -a ECCJ        Benchmark ECC (Jacobian coordinates, prime field)\n"
         << "  -a BIN         Benchmark ECC (binary field GF(2^m))\n"
         << "  -a CMP         Full comparison (all algorithms, all coordinates)\n"
         << "\n"
         << "Parameters:\n"
         << "  -b BITS        RSA key size (default: 2048)\n"
         << "  -c CURVE       Curve name (default: secp256k1)\n"
         << "                 Prime: secp256k1, P-256, P-384\n"
         << "                 Binary: sect163k1, sect233k1, sect283k1,\n"
         << "                         sect233r1, sect283r1\n"
         << "  -i ITERS       Iterations per benchmark (default: 10)\n"
         << "  -s MODE        Seed mode: fixed or random (default: fixed)\n"
         << "  -r FILE        Output raw per-iteration CSV to FILE\n"
         << "  -v             Verbose progress to stderr\n"
         << "  -h             Show this help\n"
         << "\n"
         << "Output:\n"
         << "  Summary CSV is written to stdout.\n"
         << "  Use -r to additionally write raw per-iteration data.\n"
         << "\n"
         << "Examples:\n"
         << "  " << prog << " -a CMP -i 20 -v > results/summary.csv\n"
         << "  " << prog << " -a RSA -b 4096 -i 50 -r raw.csv > summary.csv\n"
         << "  " << prog << " -a ECC -c P-384 -i 30 -v > ecc_p384.csv\n"
         << "  " << prog << " -a ECCJ -c P-256 -i 30 -v > ecc_jacobian.csv\n"
         << "  " << prog << " -a BIN -c sect283k1 -i 10 -v > binary.csv\n";
}

int main(int argc, char** argv) {
    string algo = "CMP";
    int bits = DEFAULT_RSA_BITS;
    string curve_name = DEFAULT_CURVE;
    int iterations = 10;
    string seed_mode = "fixed";
    string raw_file = "";
    bool verbose = false;

    int opt;
    while ((opt = getopt(argc, argv, "a:b:c:i:s:r:vh")) != -1) {
        switch (opt) {
            case 'a': algo = optarg; break;
            case 'b': bits = stoi(optarg); break;
            case 'c': curve_name = optarg; break;
            case 'i': iterations = stoi(optarg); break;
            case 's': seed_mode = optarg; break;
            case 'r': raw_file = optarg; break;
            case 'v': verbose = true; break;
            case 'h':
            default:
                print_usage(argv[0]);
                return (opt == 'h') ? 0 : 1;
        }
    }

    if (algo != "RSA" && algo != "ECC" && algo != "ECCJ"
        && algo != "BIN" && algo != "CMP") {
        cerr << "Error: Algorithm must be RSA, ECC, ECCJ, BIN, or CMP\n";
        return 1;
    }

    auto rng_ptr = create_rng(seed_mode, 0);
    auto& rng = *rng_ptr;

    if (verbose) {
        cerr << "Configuration: algo=" << algo
             << " iters=" << iterations
             << " seed=" << seed_mode
             << " (" << rng.get_seed() << ")" << endl;
    }

    // Run benchmarks
    vector<BenchmarkResult> results;

    try {
        if (algo == "RSA") {
            results = benchmark_rsa(rng, bits, iterations, verbose);
        } else if (algo == "ECC") {
            CurveType ct = parse_curve(curve_name);
            results = benchmark_ecc(rng, ct, iterations, verbose);
        } else if (algo == "ECCJ") {
            CurveType ct = parse_curve(curve_name);
            results = benchmark_ecc_jacobian(rng, ct, iterations, verbose);
        } else if (algo == "BIN") {
            BinaryCurveType bt = parse_binary_curve(curve_name);
            results = benchmark_ecc_binary(rng, bt, iterations, verbose);
        } else {
            results = benchmark_comparison(rng, iterations, verbose);
        }
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    // Output summary CSV to stdout
    print_csv_header(cout);
    for (const auto& r : results) {
        print_csv_row(cout, r);
    }

    // Output raw data if requested
    if (!raw_file.empty()) {
        ofstream raw_out(raw_file);
        if (!raw_out) {
            cerr << "Error: Cannot open " << raw_file << endl;
            return 1;
        }
        print_raw_csv_header(raw_out);
        for (const auto& r : results) {
            print_raw_csv_row(raw_out, r);
        }
        if (verbose) {
            cerr << "Raw data written to " << raw_file << endl;
        }
    }

    if (verbose) {
        cerr << "\nDone. " << results.size() << " benchmarks completed." << endl;
    }

    return 0;
}