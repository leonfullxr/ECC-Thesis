// main.cpp
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>
#include <chrono>
#include <functional>  // para std::function
#include <numeric>     // para std::accumulate
#include "rng.hpp"
#include "rsa.hpp"
#include "ecc.hpp"

using namespace crypto;
using namespace std;

// Programa principal para benchmarking de RSA y ECC
template<typename K>
long long benchmark_keygen(const string& label, function<K(RNG&)> gen, RNG& rng, int iters) {
    vector<long long> times;
    for (int i = 0; i < iters; ++i) {
        auto start = chrono::high_resolution_clock::now();
        gen(rng);
        auto end = chrono::high_resolution_clock::now();
        times.push_back(chrono::duration_cast<chrono::microseconds>(end-start).count());
    }
    long long sum = accumulate(times.begin(), times.end(), 0LL);
    long long avg = sum / iters;
    cout << label << " average generation time (us): " << avg << "\n";
    return avg;
}

void print_usage(const char* prog) {
    cout << "Usage: " << prog << " [options]\n"
         << "  -a ALGO      RSA or ECC           (default: RSA)\n"
         << "  -b BITS      RSA key size         (default: 2048)\n"
         << "  -c CURVE     ECC curve name       (default: secp256k1)\n"
         << "  -i ITERS     total iterations     (default: 50)\n"
         << "  -s SEED      fixed | random      (default: fixed)\n";
}

int main(int argc, char** argv) {
    string algo = "RSA";
    int bits = DEFAULT_RSA_BITS;
    string curve = DEFAULT_CURVE;
    int iters = 50;
    string seed_mode = "fixed";
    int opt;
    while ((opt = getopt(argc, argv, "a:b:c:i:s:h")) != -1) {
        switch (opt) {
            case 'a': algo = optarg; break;
            case 'b': bits = stoi(optarg); break;
            case 'c': curve = optarg; break;
            case 'i': iters = stoi(optarg); break;
            case 's': seed_mode = optarg; break;
            default: print_usage(argv[0]); return 1;
        }
    }

    // Inicializar RNG con semilla fija (0) o variable (time)
    BigInt seed = (seed_mode == "random") ? BigInt(time(nullptr)) : BigInt(0);
    NTLRNG rng(seed);

    // Mostrar configuración
    cout << "Algorithm: " << algo << "\n"
         << (algo=="RSA"?("Key size: " + to_string(bits) + "\n"): ("Curve: " + curve + "\n"))
         << "Iterations: " << iters << "\n"
         << "Seed: " << seed << "\n";

    if (algo == "RSA") {
        benchmark_keygen<RSAKeyPair>("RSA (" + to_string(bits) + "-bit)",
                                     [&](RNG& r){ return RSA::generate_key(r, bits); },
                                     rng, iters);
    } else if (algo == "ECC") {
        benchmark_keygen<ECCKeyPair>("ECC (" + curve + ")",
                                     [&](RNG& r){ return ECC::generate_key(r, curve); },
                                     rng, iters);
    } else {
        cerr << "Unknown algorithm: " << algo << "\n";
        return 1;
    }
    return 0;
}
