// rng_analysis.cpp - VERSION 3
// Genera datos NORMALIZADOS en C++ para análisis estadístico
// Normalización en C++ evita problemas de overflow en Python
// Autor: Leon Elliott Fuller
// Fecha: 2025-12-07

#include "rng.hpp"
#include "common.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <ctime>
#include <unistd.h>
#include <NTL/ZZ.h>
#include <NTL/RR.h>

using namespace crypto;
using namespace std;
using namespace NTL;

// ============================================================================
// CONFIGURACIÓN
// ============================================================================

struct Config {
    string output_file;
    long num_samples;
    long range_max;          // Para bounded mode
    int bits_per_number;     // Para fixedbits mode
    string seed_mode;        // "fixed" o "random"
    long fixed_seed_value;
    bool normalize_output;   // NUEVO: normalizar en C++ a [0, 1]
    bool verbose;
};

// ============================================================================
// GENERADORES DE DATOS CON NORMALIZACIÓN
// ============================================================================

/**
 * @brief Genera números normalizados a [0, 1]
 * CRÍTICO: Usa aritmética de alta precisión de NTL antes de convertir a double
 */
void generate_normalized_numbers(RNG& rng, const Config& config) {
    ofstream outfile(config.output_file);
    if (!outfile.is_open()) {
        throw runtime_error("Cannot open output file: " + config.output_file);
    }
    
    // Header CSV
    outfile << "index,value\n";
    outfile << fixed << setprecision(17);  // Máxima precisión para double
    
    BigInt max_val(config.range_max);
    
    if (config.verbose) {
        cout << "Generating " << config.num_samples << " normalized numbers [0.0, 1.0]...\n";
        cout << "  Original range: [0, " << config.range_max << ")\n";
    }
    
    for (long i = 0; i < config.num_samples; ++i) {
        BigInt num = rng.random_bnd(max_val);
        
        // NORMALIZACIÓN EN C++ usando NTL
        // Convertir a double DESPUÉS de normalizar en BigInt
        // normalized = num / (max_val - 1)
        double normalized = conv<double>(num) / conv<double>(max_val - 1);
        
        // Asegurar que está en [0, 1]
        if (normalized < 0.0) normalized = 0.0;
        if (normalized > 1.0) normalized = 1.0;
        
        outfile << i << "," << normalized << "\n";
        
        if (config.verbose && (i + 1) % 100000 == 0) {
            cout << "  Generated " << (i + 1) << " / " << config.num_samples << "\r" << flush;
        }
    }
    
    if (config.verbose) {
        cout << "\nGenerated " << config.num_samples << " normalized numbers\n";
    }
    
    outfile.close();
}

/**
 * @brief Genera números de tamaño fijo de bits, NORMALIZADOS
 * Para análisis de números RSA de 2048-4096 bits
 */
void generate_fixedbits_normalized(RNG& rng, const Config& config) {
    ofstream outfile(config.output_file);
    if (!outfile.is_open()) {
        throw runtime_error("Cannot open output file: " + config.output_file);
    }
    
    // Header CSV
    outfile << "index,value\n";
    outfile << fixed << setprecision(17);
    
    if (config.verbose) {
        cout << "Generating " << config.num_samples << " numbers of " 
             << config.bits_per_number << " bits (normalized)...\n";
    }
    
    // Calcular el máximo valor posible con N bits
    BigInt max_value = power2_ZZ(config.bits_per_number) - 1;
    
    for (long i = 0; i < config.num_samples; ++i) {
        BigInt num = rng.random_bits(config.bits_per_number);
        
        // NORMALIZACIÓN: num / max_value
        // Usar conv<RR> para precisión arbitraria si es necesario
        RR num_rr = conv<RR>(num);
        RR max_rr = conv<RR>(max_value);
        RR normalized_rr = num_rr / max_rr;
        double normalized = conv<double>(normalized_rr);
        
        // Asegurar rango [0, 1]
        if (normalized < 0.0) normalized = 0.0;
        if (normalized > 1.0) normalized = 1.0;
        
        outfile << i << "," << normalized << "\n";
        
        if (config.verbose && (i + 1) % 100000 == 0) {
            cout << "  Generated " << (i + 1) << " / " << config.num_samples << "\r" << flush;
        }
    }
    
    if (config.verbose) {
        cout << "\nGenerated " << config.num_samples << " normalized numbers\n";
    }
    
    outfile.close();
}

/**
 * @brief Genera pares consecutivos normalizados
 */
void generate_pairs_normalized(RNG& rng, const Config& config) {
    ofstream outfile(config.output_file);
    if (!outfile.is_open()) {
        throw runtime_error("Cannot open output file: " + config.output_file);
    }
    
    // Header CSV
    outfile << "index,x,y\n";
    outfile << fixed << setprecision(17);
    
    BigInt max_val(config.range_max);
    
    if (config.verbose) {
        cout << "Generating " << config.num_samples << " consecutive pairs (normalized)...\n";
    }
    
    for (long i = 0; i < config.num_samples; ++i) {
        BigInt x_big = rng.random_bnd(max_val);
        BigInt y_big = rng.random_bnd(max_val);
        
        double x = conv<double>(x_big) / conv<double>(max_val - 1);
        double y = conv<double>(y_big) / conv<double>(max_val - 1);
        
        if (x < 0.0) x = 0.0;
        if (x > 1.0) x = 1.0;
        if (y < 0.0) y = 0.0;
        if (y > 1.0) y = 1.0;
        
        outfile << i << "," << x << "," << y << "\n";
        
        if (config.verbose && (i + 1) % 100000 == 0) {
            cout << "  Generated " << (i + 1) << " / " << config.num_samples << "\r" << flush;
        }
    }
    
    if (config.verbose) {
        cout << "\nGenerated " << config.num_samples << " pairs\n";
    }
    
    outfile.close();
}

/**
 * @brief Genera bits individuales (0 o 1)
 */
void generate_random_bits(RNG& rng, const Config& config) {
    ofstream outfile(config.output_file);
    if (!outfile.is_open()) {
        throw runtime_error("Cannot open output file: " + config.output_file);
    }
    
    // Header CSV
    outfile << "index,bit\n";
    
    if (config.verbose) {
        cout << "Generating " << config.num_samples << " random bits...\n";
    }
    
    for (long i = 0; i < config.num_samples; ++i) {
        BigInt num = rng.random_bits(1);
        outfile << i << "," << num << "\n";
        
        if (config.verbose && (i + 1) % 100000 == 0) {
            cout << "  Generated " << (i + 1) << " / " << config.num_samples << "\r" << flush;
        }
    }
    
    if (config.verbose) {
        cout << "\nGenerated " << config.num_samples << " bits\n";
    }
    
    outfile.close();
}

// ============================================================================
// MAIN
// ============================================================================

void print_usage(const char* prog) {
    cout << "Usage: " << prog << " [options]\n"
         << "\nOptions:\n"
         << "  -o FILE      Output file (default: rng_data.csv)\n"
         << "  -n NUM       Number of samples (default: 1000000)\n"
         << "  -r RANGE     Maximum value for bounded mode (default: 1000)\n"
         << "  -b BITS      Bits per number for fixedbits mode (default: 32)\n"
         << "  -s MODE      Seed mode: fixed | random (default: fixed)\n"
         << "  -S SEED      Fixed seed value (default: 0)\n"
         << "  -m MODE      Generation mode:\n"
         << "                 bounded    - Normalized numbers from [0, RANGE)\n"
         << "                 bits       - Individual random bits (0 or 1)\n"
         << "                 fixedbits  - Normalized numbers of B bits\n"
         << "                 pairs      - Consecutive normalized pairs\n"
         << "               (default: bounded)\n"
         << "  --no-normalize  Do NOT normalize output (keep original values)\n"
         << "  -v           Verbose output\n"
         << "  -h           Show this help\n"
         << "\nNOTE: By default, all modes except 'bits' generate normalized\n"
         << "      output in [0.0, 1.0] range for statistical analysis.\n"
         << "\nExamples:\n"
         << "  # 1M normalized numbers for statistical tests\n"
         << "  " << prog << " -n 1000000 -r 1000 -s fixed\n"
         << "\n"
         << "  # 100K numbers from 2048-bit range (for RSA analysis)\n"
         << "  " << prog << " -n 100000 -m fixedbits -b 2048 -o rsa2048.csv\n"
         << "\n"
         << "  # 10M random bits\n"
         << "  " << prog << " -n 10000000 -m bits -o bits.csv\n";
}

int main(int argc, char** argv) {
    Config config;
    config.output_file = "rng_data.csv";
    config.num_samples = 1000000;
    config.range_max = 1000;
    config.bits_per_number = 32;
    config.seed_mode = "fixed";
    config.fixed_seed_value = 0;
    config.normalize_output = true;  // Por defecto, normalizar
    config.verbose = false;
    
    string generation_mode = "bounded";
    
    // Parse arguments
    int opt;
    while ((opt = getopt(argc, argv, "o:n:r:b:s:S:m:vh")) != -1) {
        switch (opt) {
            case 'o': config.output_file = optarg; break;
            case 'n': config.num_samples = stol(optarg); break;
            case 'r': config.range_max = stol(optarg); break;
            case 'b': config.bits_per_number = stoi(optarg); break;
            case 's': config.seed_mode = optarg; break;
            case 'S': config.fixed_seed_value = stol(optarg); break;
            case 'm': generation_mode = optarg; break;
            case 'v': config.verbose = true; break;
            case 'h':
            default: 
                print_usage(argv[0]); 
                return (opt == 'h') ? 0 : 1;
        }
    }
    
    // Verificar si hay --no-normalize
    for (int i = 1; i < argc; ++i) {
        if (string(argv[i]) == "--no-normalize") {
            config.normalize_output = false;
        }
    }
    
    // Bits mode NUNCA se normaliza (ya es 0 o 1)
    if (generation_mode == "bits") {
        config.normalize_output = false;
    }
    
    try {
        // Crear RNG
        auto rng_ptr = create_rng(config.seed_mode, config.fixed_seed_value);
        auto& rng = *rng_ptr;
        
        // Mostrar configuración
        cout << "\n" << string(70, '=') << "\n";
        cout << "RNG DATA GENERATOR v3.0\n";
        cout << string(70, '=') << "\n";
        cout << "  Output file:      " << config.output_file << "\n";
        cout << "  Samples:          " << config.num_samples << "\n";
        cout << "  Generation mode:  " << generation_mode << "\n";
        
        if (generation_mode == "bounded" || generation_mode == "pairs") {
            cout << "  Range:            [0, " << config.range_max << ")\n";
        }
        if (generation_mode == "fixedbits") {
            cout << "  Bits per number:  " << config.bits_per_number << "\n";
        }
        
        cout << "  Seed mode:        " << config.seed_mode << "\n";
        cout << "  Seed value:       " << rng.get_seed() << "\n";
        cout << "  Normalized:       " << (config.normalize_output ? "YES" : "NO") << "\n";
        cout << string(70, '=') << "\n\n";
        
        // Generar datos según el modo
        auto start_time = chrono::high_resolution_clock::now();
        
        if (generation_mode == "bounded") {
            if (config.normalize_output) {
                generate_normalized_numbers(rng, config);
            } else {
                // Versión sin normalizar (legacy)
                throw runtime_error("Non-normalized bounded mode not implemented - use --normalize");
            }
        } else if (generation_mode == "bits") {
            generate_random_bits(rng, config);
        } else if (generation_mode == "fixedbits") {
            if (config.normalize_output) {
                generate_fixedbits_normalized(rng, config);
            } else {
                throw runtime_error("Non-normalized fixedbits mode not implemented - use --normalize");
            }
        } else if (generation_mode == "pairs") {
            if (config.normalize_output) {
                generate_pairs_normalized(rng, config);
            } else {
                throw runtime_error("Non-normalized pairs mode not implemented - use --normalize");
            }
        } else {
            throw runtime_error("Unknown generation mode: " + generation_mode);
        }
        
        auto end_time = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
        
        cout << "\n" << string(70, '=') << "\n";
        cout << "COMPLETED\n";
        cout << string(70, '=') << "\n";
        cout << "  Time elapsed:     " << duration.count() << " ms\n";
        cout << "  Rate:             " << (config.num_samples * 1000.0 / duration.count()) 
             << " samples/sec\n";
        cout << "  Output file:      " << config.output_file << "\n";
        if (config.normalize_output) {
            cout << "  Data range:       [0.0, 1.0] (normalized)\n";
        }
        cout << string(70, '=') << "\n\n";
        
        cout << "Next step: Analyze with Python:\n";
        cout << "  python3 scripts/analyze_randomness.py " << config.output_file << " results/plots\n\n";
        
    } catch (const exception& e) {
        cerr << "\nError: " << e.what() << "\n\n";
        return 1;
    }
    
    return 0;
}