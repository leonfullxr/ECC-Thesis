// rng_analysis.cpp
// Programa para generar datasets de números aleatorios para análisis estadístico
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

using namespace crypto;
using namespace std;
using namespace NTL;

// ============================================================================
// CONFIGURACIÓN
// ============================================================================

struct Config {
    string output_file;
    long num_samples;
    long range_max;          // Generar números en [0, range_max)
    int bits_per_number;     // Para random_bits
    string seed_mode;        // "fixed" o "random"
    long fixed_seed_value;
    bool generate_bits;      // true para bits individuales, false para números
    bool verbose;
};

// ============================================================================
// GENERADORES DE DATOS
// ============================================================================

/**
 * @brief Genera números aleatorios en un rango
 */
void generate_bounded_numbers(RNG& rng, const Config& config) {
    ofstream outfile(config.output_file);
    if (!outfile.is_open()) {
        throw runtime_error("Cannot open output file: " + config.output_file);
    }
    
    // Header CSV
    outfile << "index,value\n";
    
    BigInt max_val(config.range_max);
    
    if (config.verbose) {
        cout << "Generating " << config.num_samples << " numbers in range [0, " 
             << config.range_max << ")...\n";
    }
    
    for (long i = 0; i < config.num_samples; ++i) {
        BigInt num = rng.random_bnd(max_val);
        outfile << i << "," << num << "\n";
        
        if (config.verbose && (i + 1) % 100000 == 0) {
            cout << "  Generated " << (i + 1) << " / " << config.num_samples << "\r" << flush;
        }
    }
    
    if (config.verbose) {
        cout << "\nGenerated " << config.num_samples << " numbers\n";
    }
    
    outfile.close();
}

/**
 * @brief Genera bits aleatorios individuales
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
        // Generar 1 bit aleatorio
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

/**
 * @brief Genera números de un tamaño fijo de bits
 */
void generate_fixed_bit_numbers(RNG& rng, const Config& config) {
    ofstream outfile(config.output_file);
    if (!outfile.is_open()) {
        throw runtime_error("Cannot open output file: " + config.output_file);
    }
    
    // Header CSV
    outfile << "index,value,binary\n";
    
    if (config.verbose) {
        cout << "Generating " << config.num_samples << " numbers of " 
             << config.bits_per_number << " bits...\n";
    }
    
    for (long i = 0; i < config.num_samples; ++i) {
        BigInt num = rng.random_bits(config.bits_per_number);
        
        // Convertir a binario (para análisis de distribución de bits)
        string binary = "";
        BigInt temp = num;
        for (int b = 0; b < config.bits_per_number; ++b) {
            binary = (bit(temp, b) ? "1" : "0") + binary;
        }
        
        outfile << i << "," << num << "," << binary << "\n";
        
        if (config.verbose && (i + 1) % 100000 == 0) {
            cout << "  Generated " << (i + 1) << " / " << config.num_samples << "\r" << flush;
        }
    }
    
    if (config.verbose) {
        cout << "\nGenerated " << config.num_samples << " numbers\n";
    }
    
    outfile.close();
}

/**
 * @brief Genera pares consecutivos para análisis de correlación
 */
void generate_consecutive_pairs(RNG& rng, const Config& config) {
    ofstream outfile(config.output_file);
    if (!outfile.is_open()) {
        throw runtime_error("Cannot open output file: " + config.output_file);
    }
    
    // Header CSV
    outfile << "index,x,y\n";
    
    BigInt max_val(config.range_max);
    
    if (config.verbose) {
        cout << "Generating " << config.num_samples << " consecutive pairs...\n";
    }
    
    for (long i = 0; i < config.num_samples; ++i) {
        BigInt x = rng.random_bnd(max_val);
        BigInt y = rng.random_bnd(max_val);
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

// ============================================================================
// MAIN
// ============================================================================

void print_usage(const char* prog) {
    cout << "Usage: " << prog << " [options]\n"
         << "\nOptions:\n"
         << "  -o FILE      Output file (default: rng_data.csv)\n"
         << "  -n NUM       Number of samples (default: 1000000)\n"
         << "  -r RANGE     Maximum value for bounded random (default: 1000)\n"
         << "  -b BITS      Bits per number for fixed-bit mode (default: 32)\n"
         << "  -s MODE      Seed mode: fixed | random (default: fixed)\n"
         << "  -S SEED      Fixed seed value (default: 0)\n"
         << "  -m MODE      Generation mode:\n"
         << "                 bounded    - Numbers in [0, RANGE)\n"
         << "                 bits       - Individual random bits\n"
         << "                 fixedbits  - Numbers of fixed bit size\n"
         << "                 pairs      - Consecutive pairs (for correlation)\n"
         << "               (default: bounded)\n"
         << "  -v           Verbose output\n"
         << "  -h           Show this help\n"
         << "\nExamples:\n"
         << "  # Generate 1M numbers in [0, 1000) with fixed seed\n"
         << "  " << prog << " -n 1000000 -r 1000 -s fixed\n"
         << "\n"
         << "  # Generate 10M random bits\n"
         << "  " << prog << " -n 10000000 -m bits -o bits.csv\n"
         << "\n"
         << "  # Generate 100K 32-bit numbers\n"
         << "  " << prog << " -n 100000 -m fixedbits -b 32\n"
         << "\n"
         << "  # Generate pairs for correlation analysis\n"
         << "  " << prog << " -n 100000 -m pairs -r 10000\n";
}

int main(int argc, char** argv) {
    Config config;
    config.output_file = "rng_data.csv";
    config.num_samples = 1000000;
    config.range_max = 1000;
    config.bits_per_number = 32;
    config.seed_mode = "fixed";
    config.fixed_seed_value = 0;
    config.generate_bits = false;
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
    
    try {
        // Crear RNG
        auto rng_ptr = create_rng(config.seed_mode, config.fixed_seed_value);
        auto& rng = *rng_ptr;
        
        // Mostrar configuración
        cout << "\n" << string(70, '=') << "\n";
        cout << "RNG DATA GENERATOR\n";
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
        cout << string(70, '=') << "\n\n";
        
        // Generar datos según el modo
        auto start_time = chrono::high_resolution_clock::now();
        
        if (generation_mode == "bounded") {
            generate_bounded_numbers(rng, config);
        } else if (generation_mode == "bits") {
            generate_random_bits(rng, config);
        } else if (generation_mode == "fixedbits") {
            generate_fixed_bit_numbers(rng, config);
        } else if (generation_mode == "pairs") {
            generate_consecutive_pairs(rng, config);
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
        cout << string(70, '=') << "\n\n";
        
        cout << "Next step: Analyze with Python:\n";
        cout << "  python3 analyze_randomness.py " << config.output_file << "\n\n";
        
    } catch (const exception& e) {
        cerr << "\nError: " << e.what() << "\n\n";
        return 1;
    }
    
    return 0;
}