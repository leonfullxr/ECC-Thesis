<h1 align="center">ECC-Thesis</h1>

<p align="center">
   <img src="docs/Plantilla_TFG_latex/imagenes/torus.png" alt="Cypher Logo" width="100%">
</p>

---

## Table of Contents

- [ECC-Thesis](#ecc-thesis)
  - [1. Project Description](#1--project-description)
  - [2. Project Structure](#2--project-structure)
  - [3. CI / GitHub Workflows](#3-ci-/-github-workflows)
  - [4. Docker Image](#4--docker-image)
    - [Files](#files)
    - [Usage](#usage)
      - [1. Build the image](#1-build-the-image)
      - [2. Start a shell](#2-start-a-shell)
      - [3. Compile into `bin/`](#3-compile-into-bin)
      - [4. Run your binary](#4-run-your-binary)
  - [Local-override vs General](#local-override-vs-general)
  - [5. Testing](#5-testing)
    - [Manual tests](#manual-tests)
    - [RNG analysis tests](#rng-analysis-tests)

## 1. Project Description

This Computer Engineering Bachelor's Final Thesis for the University of Granada proposal focuses on the study of elliptic curves applied to cryptography, addressing both its theoretical foundations and its practical application in security systems. The mathematical basis of ECC, its implementations in comparison with traditional methods such as RSA and the discrete logarithm problem will be analyzed. In addition, the impact of quantum computing on the security of these systems will be explored and coding and visualization tools will be developed to illustrate its operation. The work aims to provide a comprehensive and up-to-date view on the potential of elliptic curves in modern cryptography.

The project also includes a Docker-based environment for compiling and running the code, which is designed to be easily portable and reproducible. The Docker image includes the necessary libraries and tools for working with elliptic curves, such as GMP and NTL.

## 2. Project Structure

```bash
mi_proyecto_crypto/
├── Makefile
├── include/               # Ficheros .hpp
│   ├── rng.hpp            # Interfaz de generador RNG
│   ├── timer.hpp          # Wrapper de temporización
│   ├── rsa.hpp            # Clases y funciones RSA
│   └── ecc.hpp            # Clases y funciones ECC
├── src/                   # Implementaciones .cpp
│   ├── rng.cpp
│   ├── timer.cpp
│   ├── rsa.cpp
│   └── ecc.cpp
├── bench/                 # Código de benchmarking
│   └── bench_main.cpp     # Script principal de pruebas
├── scripts/               # Scripts auxiliares (bash, Python)
│   ├── run_bench.sh       # Automatiza la ejecución de pruebas
│   └── analyze_results.py # (Opcional) Post-procesado con matplotlib
├── data/                  # Datos estáticos (semillas, curvas, etc.)
│   └── seeds.bin          # Semillas pre-generadas
├── bin/                   # Ejecutables compilados
├── results/               # Salidas de bench (CSV, logs)
├── docs/                  # Documentación y especificaciones
│   └── proyecto.md
├── Dockerfile             # Define el contenedor con GMP & NTL
├── docker-compose.yml     # Compose (general-purpose) creando servicio `crypto`
└── docker-compose.local.yml # (opcional) Compose apuntando a tu ruta USB
```

## 3. CI / GitHub Workflows

The repository includes a `docker-publish.yml` file and a `test.yml` for GitHub Actions to automatically rebuild and publish our Docker image on every push to `main` or when a new tag is created.

### Workflow file

- **`.github/workflows/docker-publish.yml`**: is an automation that, when a push is made to GitHub and changes are detected in the dependencies of any of the languages or the Dockerfile, it automatically rebuilds the image and uploads it to DockerHub under `leonfullxr/ecc-benchmarks:latest`.
- **`.github/workflows/test.yml`**: On every push or PR to `main`:
  1. Checks out your code  
  2. Pulls the latest image from Docker Hub  
  3. Runs a quick non-interactive command inside the container (e.g. `make`) to verify it launches correctly

### Secrets

In your fork (or upstream) you must configure two repository secrets under **Settings → Secrets and variables → Actions**:

```yaml
DOCKERHUB_USERNAME: your-dockerhub-username   # e.g. "leonfullxr"
DOCKERHUB_TOKEN:    your-dockerhub-access-token
```

## 4. Docker Image

We provide a containerized environment with **GMP 6.3.0** and **NTL 11.5.1** pre-installed. The `/workspace/bin` directory is automatically added to `PATH`, so any executable you compile there can be run by name.

## Files

* **Dockerfile**
  Builds on Ubuntu 22.04, installs build tools, GMP 6.3.0, then NTL 11.5.1.

* **docker-compose.yml**
  General-purpose compose file. Mounts your current project folder (`.`) into `/workspace`.

* **docker-compose.local.yml**
  (Optional) If you’re working from `/media/…` (like me) and need a custom host path, you can override the volume mount here.

## Usage

You can download the image from Docker Hub and run it:

```bash
# pull the image
docker pull leonfullxr/ecc-benchmarks:latest

# run an interactive shell
docker run --rm -it \
  -v "$(pwd)":/workspace \
  -w /workspace \
  leonfullxr/ecc-benchmarks:latest \
  bash
```

Or you can build it locally.

### 1. Build the image

From your project root:

```bash
docker compose build
```

Or, if you want to use Docker directly:

```bash
docker build -t ecc-thesis .
```

### 2. Start a shell

With Compose:

```bash
docker compose run crypto
```

With plain Docker:

```bash
docker run --rm -it \
  -v "$(pwd)":/workspace \
  -w /workspace \
  ecc-thesis
```

You’ll land in `/workspace`, with `/workspace/bin` on your `PATH`.

### 3. Compile into `bin/`

All your `.cpp` files live in `src/`, headers in `include/`, and you’ll place executables in `bin/`. For example:

```bash
g++ -std=c++17 \
    -I/usr/local/include \
    src/my_ecc.cpp \
    -L/usr/local/lib -lntl -lgmp -pthread \
    -o bin/ecc_demo
```

### 4. Run your binary

```bash
make
./bin/bench -h
./bin/bench -a RSA -b 4096 -i 5 -s fixed
```

## Local-override vs General

* **General use**:
  When someone does a plain `git clone …` anywhere on their filesystem, they run:

```bash
  docker-compose up --build
  docker-compose run crypto
```

  Compose will pick up `docker-compose.yml` and bind `.` → `/workspace`.

* **USB-path override**:
  If you need to mount a weird host path (like me) from your USB drive, use:

```bash
  docker-compose -f docker-compose.local.yml up -d --build
  docker-compose -f docker-compose.local.yml run crypto
```

## 5. Testing

You can run the provided benchmarks inside the container to verify everything is working correctly:

```bash
bash scripts/quick_demo.sh
bash scripts/benchmark_comparison.sh
```

<details>
<summary>Example output from `quick_demo.sh`:</summary>

```
root@5ec6aae22fc8:/workspace# bash scripts/quick_demo.sh 

╔════════════════════════════════════════════════════════════════════╗
║                    RSA vs ECC BENCHMARK - DEMO                     ║
║                   Leon Elliott Fuller - 2025                       ║
╚════════════════════════════════════════════════════════════════════╝

 Compilando proyecto...
 Compilación exitosa

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  DEMO 1: Generación de claves RSA (1024-bit, 5 iteraciones)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━


================================================================================
BENCHMARK CONFIGURATION
================================================================================
  Algorithm:    RSA
  Key size:     1024 bits
  Iterations:   5
  Mode:         seq
  Seed mode:    fixed
  Seed value:   0
================================================================================

================================================================================
RSA BENCHMARK - 1024 bits
================================================================================
Running RSA Key Generation (1024-bit) (5 iterations)... Done!

Generating RSA keypair for encryption/decryption tests...
Running RSA Encryption (1024-bit) (5 iterations)... Done!
Running RSA Decryption (1024-bit, no CRT) (5 iterations)... Done!
Running RSA Decryption (1024-bit, with CRT) (5 iterations)... Done!

--------------------------------------------------------------------------------
RESULTS SUMMARY
--------------------------------------------------------------------------------

=== RSA Key Generation (1024-bit) ===
  Iterations:   5
  Average:      1430 µs
  Min:          451 µs
  Max:          2678 µs
  Total:        7152 µs

=== RSA Encryption (1024-bit) ===
  Iterations:   5
  Average:      3 µs
  Min:          3 µs
  Max:          4 µs
  Total:        16 µs

=== RSA Decryption (1024-bit, no CRT) ===
  Iterations:   5
  Average:      182 µs
  Min:          181 µs
  Max:          189 µs
  Total:        914 µs

=== RSA Decryption (1024-bit, with CRT) ===
  Iterations:   5
  Average:      58 µs
  Min:          57 µs
  Max:          60 µs
  Total:        291 µs

================================================================================

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  DEMO 2: Comparación de cifrado vs descifrado (con y sin CRT)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Running RSA Encryption (2048-bit) (5 iterations)... Done!
Running RSA Decryption (2048-bit, no CRT) (5 iterations)... Done!
Running RSA Decryption (2048-bit, with CRT) (5 iterations)... Done!
  Average:      24170 µs
=== RSA Encryption (2048-bit) ===
  Average:      11 µs
=== RSA Decryption (2048-bit, no CRT) ===
  Average:      1253 µs
=== RSA Decryption (2048-bit, with CRT) ===
  Average:      366 µs

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  DEMO COMPLETADA
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

 Características demostradas:
   Generación de claves RSA con diferentes tamaños
   Cifrado y descifrado RSA
   Optimización CRT (4x más rápido)
   Sistema de semillas reproducible
   Estadísticas detalladas (avg, min, max)

 Para más información, consulte:
   - README.md para documentación completa
   - ./bin/bench -h para opciones disponibles
   - make test-rsa para tests predefinidos

```
</details>

<details>
<summary>Example output from `benchmark_comparison.sh`:</summary>

```
root@5ec6aae22fc8:/workspace# bash scripts/benchmark_comparison.sh 
=============================================================================
BENCHMARK COMPARATIVO RSA - DIFERENTES TAMAÑOS DE CLAVE
=============================================================================

Configuración:
  - Iteraciones por tamaño: 10
  - Modo de semilla: fixed
  - Archivo de salida: benchmark_results_20251206_122427.txt

=============================================================================

Ejecutando benchmark para RSA-1024 bits...
---------------------------------------------------------------------


================================================================================
BENCHMARK CONFIGURATION
================================================================================
  Algorithm:    RSA
  Key size:     1024 bits
  Iterations:   10
  Mode:         seq
  Seed mode:    fixed
  Seed value:   0
================================================================================

================================================================================
RSA BENCHMARK - 1024 bits
================================================================================
Running RSA Key Generation (1024-bit) (10 iterations)............. Done!

Generating RSA keypair for encryption/decryption tests...
Running RSA Encryption (1024-bit) (10 iterations)............. Done!
Running RSA Decryption (1024-bit, no CRT) (10 iterations)............. Done!
Running RSA Decryption (1024-bit, with CRT) (10 iterations)............. Done!

--------------------------------------------------------------------------------
RESULTS SUMMARY
--------------------------------------------------------------------------------

=== RSA Key Generation (1024-bit) ===
  Iterations:   10
  Average:      1587 µs
  Min:          309 µs
  Max:          4019 µs
  Total:        15875 µs

=== RSA Encryption (1024-bit) ===
  Iterations:   10
  Average:      3 µs
  Min:          3 µs
  Max:          4 µs
  Total:        39 µs

=== RSA Decryption (1024-bit, no CRT) ===
  Iterations:   10
  Average:      198 µs
  Min:          196 µs
  Max:          210 µs
  Total:        1988 µs

=== RSA Decryption (1024-bit, with CRT) ===
  Iterations:   10
  Average:      69 µs
  Min:          69 µs
  Max:          73 µs
  Total:        699 µs

================================================================================
Completado RSA-1024

Ejecutando benchmark para RSA-2048 bits...
---------------------------------------------------------------------


================================================================================
BENCHMARK CONFIGURATION
================================================================================
  Algorithm:    RSA
  Key size:     2048 bits
  Iterations:   10
  Mode:         seq
  Seed mode:    fixed
  Seed value:   0
================================================================================

================================================================================
RSA BENCHMARK - 2048 bits
================================================================================
Running RSA Key Generation (2048-bit) (10 iterations)............. Done!

Generating RSA keypair for encryption/decryption tests...
Running RSA Encryption (2048-bit) (10 iterations)............. Done!
Running RSA Decryption (2048-bit, no CRT) (10 iterations)............. Done!
Running RSA Decryption (2048-bit, with CRT) (10 iterations)............. Done!

--------------------------------------------------------------------------------
RESULTS SUMMARY
--------------------------------------------------------------------------------

=== RSA Key Generation (2048-bit) ===
  Iterations:   10
  Average:      18509 µs
  Min:          5251 µs
  Max:          43294 µs
  Total:        185091 µs

=== RSA Encryption (2048-bit) ===
  Iterations:   10
  Average:      11 µs
  Min:          11 µs
  Max:          17 µs
  Total:        119 µs

=== RSA Decryption (2048-bit, no CRT) ===
  Iterations:   10
  Average:      1290 µs
  Min:          1283 µs
  Max:          1299 µs
  Total:        12906 µs

=== RSA Decryption (2048-bit, with CRT) ===
  Iterations:   10
  Average:      376 µs
  Min:          374 µs
  Max:          380 µs
  Total:        3765 µs

================================================================================
Completado RSA-2048

Ejecutando benchmark para RSA-3072 bits...
---------------------------------------------------------------------


================================================================================
BENCHMARK CONFIGURATION
================================================================================
  Algorithm:    RSA
  Key size:     3072 bits
  Iterations:   10
  Mode:         seq
  Seed mode:    fixed
  Seed value:   0
================================================================================

================================================================================
RSA BENCHMARK - 3072 bits
================================================================================
Running RSA Key Generation (3072-bit) (10 iterations)............. Done!

Generating RSA keypair for encryption/decryption tests...
Running RSA Encryption (3072-bit) (10 iterations)............. Done!
Running RSA Decryption (3072-bit, no CRT) (10 iterations)............. Done!
Running RSA Decryption (3072-bit, with CRT) (10 iterations)............. Done!

--------------------------------------------------------------------------------
RESULTS SUMMARY
--------------------------------------------------------------------------------

=== RSA Key Generation (3072-bit) ===
  Iterations:   10
  Average:      92115 µs
  Min:          10253 µs
  Max:          255388 µs
  Total:        921154 µs

=== RSA Encryption (3072-bit) ===
  Iterations:   10
  Average:      24 µs
  Min:          23 µs
  Max:          30 µs
  Total:        248 µs

=== RSA Decryption (3072-bit, no CRT) ===
  Iterations:   10
  Average:      4013 µs
  Min:          4005 µs
  Max:          4021 µs
  Total:        40134 µs

=== RSA Decryption (3072-bit, with CRT) ===
  Iterations:   10
  Average:      1157 µs
  Min:          1148 µs
  Max:          1168 µs
  Total:        11570 µs

================================================================================
Completado RSA-3072

Ejecutando benchmark para RSA-4096 bits...
---------------------------------------------------------------------


================================================================================
BENCHMARK CONFIGURATION
================================================================================
  Algorithm:    RSA
  Key size:     4096 bits
  Iterations:   10
  Mode:         seq
  Seed mode:    fixed
  Seed value:   0
================================================================================

================================================================================
RSA BENCHMARK - 4096 bits
================================================================================
Running RSA Key Generation (4096-bit) (10 iterations)............. Done!

Generating RSA keypair for encryption/decryption tests...
Running RSA Encryption (4096-bit) (10 iterations)............. Done!
Running RSA Decryption (4096-bit, no CRT) (10 iterations)............. Done!
Running RSA Decryption (4096-bit, with CRT) (10 iterations)............. Done!

--------------------------------------------------------------------------------
RESULTS SUMMARY
--------------------------------------------------------------------------------

=== RSA Key Generation (4096-bit) ===
  Iterations:   10
  Average:      134016 µs
  Min:          20649 µs
  Max:          268057 µs
  Total:        1340165 µs

=== RSA Encryption (4096-bit) ===
  Iterations:   10
  Average:      39 µs
  Min:          39 µs
  Max:          44 µs
  Total:        395 µs

=== RSA Decryption (4096-bit, no CRT) ===
  Iterations:   10
  Average:      8797 µs
  Min:          8787 µs
  Max:          8820 µs
  Total:        87971 µs

=== RSA Decryption (4096-bit, with CRT) ===
  Iterations:   10
  Average:      2494 µs
  Min:          2489 µs
  Max:          2506 µs
  Total:        24944 µs

================================================================================
Completado RSA-4096

=============================================================================
BENCHMARK COMPLETADO
=============================================================================

Resultados guardados en: benchmark_results_20251206_122427.txt

RESUMEN DE TIEMPOS PROMEDIO (Key Generation):
---------------------------------------------------------------------
   1.   Average:      1587 µs
   2.   Average:      18509 µs
   3.   Average:      92115 µs
   4.   Average:      134016 µs

Para análisis detallado, consulte: benchmark_results_20251206_122427.txt
```

</details>

### Manual tests
You can also compile and run individual tests manually:

```bash
make test-rsa-2k ITERS=10
./bin/bench -a RSA -b 2048 -i 10 -s fixed
./bin/bench -a RSA -b 2048 -i 10 -s random
./bin/bench -a RSA -b 4096 -i 5 -s fixed
```

#### RNG analysis tests
This is essential to ensure the quality of our Random Number Generator (RNG) used in key generation and other cryptographic operations.

For a more detailed statistical analysis of the RNG, you can read the [RNG readme](rng.md) file. Here are some example commands to run the RNG analysis with different parameters:

```bash
# Con semilla aleatoria (timestamp)
./bin/rng_analysis -n 1000000 -r 1000 -s random -o data.csv

# Rango más grande
./bin/rng_analysis -n 1000000 -r 100000 -s fixed -o large_range.csv

# Números de 64 bits
./bin/rng_analysis -n 100000 -m fixedbits -b 64 -o 64bit.csv
```

Then analyze the reports and plots directory:

```bash
# Probar con 2048 bits
./bin/rng_analysis -n 100000 -m fixedbits -b 2048 -o results/data/rsa2048.csv -v

python3 scripts/analyze_randomness.py results/data/rsa2048.csv results/plots
# Ver reporte
cat reports/rng_analysis_*.txt

# Ver gráficas
ls plots/
# Verás: histogram.png, autocorrelation.png, consecutive_pairs.png, etc.
```

#### Running the full RNG analysis script
You can run the complete RNG analysis script which automates data generation, analysis, and plotting:

<details>
<summary>Example output from `run_rng_analysis.sh`:</summary>

```bash
root@18fbc909a7d0:/workspace# bash scripts/run_rng_analysis.sh 

====================================================================
         ANÁLISIS ESTADÍSTICO COMPLETO DEL RNG                     
====================================================================

====================================================================
  PASO 1: Compilando herramienta de análisis
====================================================================

  rng_analysis ya compilado

====================================================================
  PASO 2: Verificando dependencias Python
====================================================================

  Python version: Python 3.10.12

  Verificando paquetes Python...
  Todos los paquetes instalados

====================================================================
  PASO 3: Generando datasets de números aleatorios
====================================================================

  [1/4] Generando números acotados (1M samples)...

======================================================================
RNG DATA GENERATOR v3.0
======================================================================
  Output file:      results/data/bounded.csv
  Samples:          1000000
  Generation mode:  bounded
  Range:            [0, 1000)
  Seed mode:        fixed
  Seed value:       0
  Normalized:       YES
======================================================================

Generating 1000000 normalized numbers [0.0, 1.0]...
  Original range: [0, 1000)
  Generated 1000000 / 1000000
Generated 1000000 normalized numbers

======================================================================
COMPLETED
======================================================================
  Time elapsed:     267 ms
  Rate:             3.74532e+06 samples/sec
  Output file:      results/data/bounded.csv
  Data range:       [0.0, 1.0] (normalized)
======================================================================

Next step: Analyze with Python:
  python3 scripts/analyze_randomness.py results/data/bounded.csv results/plots


  [2/4] Generando bits individuales (5M samples)...

======================================================================
RNG DATA GENERATOR v3.0
======================================================================
  Output file:      results/data/bits.csv
  Samples:          5000000
  Generation mode:  bits
  Seed mode:        fixed
  Seed value:       0
  Normalized:       NO
======================================================================

Generating 5000000 random bits...
  Generated 5000000 / 5000000
Generated 5000000 bits

======================================================================
COMPLETED
======================================================================
  Time elapsed:     452 ms
  Rate:             1.10619e+07 samples/sec
  Output file:      results/data/bits.csv
======================================================================

Next step: Analyze with Python:
  python3 scripts/analyze_randomness.py results/data/bits.csv results/plots


  [3/4] Generando números de 2046 bits (100K samples)...

======================================================================
RNG DATA GENERATOR v3.0
======================================================================
  Output file:      results/data/fixedbits2046.csv
  Samples:          100000
  Generation mode:  fixedbits
  Bits per number:  2046
  Seed mode:        fixed
  Seed value:       0
  Normalized:       YES
======================================================================

Generating 100000 numbers of 2046 bits (normalized)...
  Generated 100000 / 100000
Generated 100000 normalized numbers

======================================================================
COMPLETED
======================================================================
  Time elapsed:     61 ms
  Rate:             1.63934e+06 samples/sec
  Output file:      results/data/fixedbits2046.csv
  Data range:       [0.0, 1.0] (normalized)
======================================================================

Next step: Analyze with Python:
  python3 scripts/analyze_randomness.py results/data/fixedbits2046.csv results/plots


  [4/4] Generando pares consecutivos (100K samples)...

======================================================================
RNG DATA GENERATOR v3.0
======================================================================
  Output file:      results/data/pairs.csv
  Samples:          100000
  Generation mode:  pairs
  Range:            [0, 10000)
  Seed mode:        fixed
  Seed value:       0
  Normalized:       YES
======================================================================

Generating 100000 consecutive pairs (normalized)...
  Generated 100000 / 100000
Generated 100000 pairs

======================================================================
COMPLETED
======================================================================
  Time elapsed:     51 ms
  Rate:             1.96078e+06 samples/sec
  Output file:      results/data/pairs.csv
  Data range:       [0.0, 1.0] (normalized)
======================================================================

Next step: Analyze with Python:
  python3 scripts/analyze_randomness.py results/data/pairs.csv results/plots


====================================================================
  PASO 4: Ejecutando análisis estadístico
====================================================================

  Analizando dataset principal (números acotados)...

======================================================================
  ANÁLISIS ESTADÍSTICO DE ALEATORIEDAD
======================================================================
  Archivo de entrada: results/data/bounded.csv

  Cargando datos...
  Cargados 1,000,000 valores
  Rango: [0.000000, 1.000000] (normalizado)

======================================================================
  SUITE COMPLETA DE TESTS ESTADÍSTICOS
======================================================================

======================================================================
  ANÁLISIS DE MEDIA Y VARIANZA
======================================================================
  Media observada:  0.5002
  Media esperada:   0.5000
  Var. observada:   0.0834
  Var. esperada:    0.0833
  P-value (media):  0.387811
  Resultado:        PASS

======================================================================
  TEST DE UNIFORMIDAD (Chi-cuadrado)
======================================================================
  Chi-cuadrado:     9.2236
  Grados libertad:  9
  P-value:          0.416894
  Resultado:        PASS (alpha=0.05)
  -> Los datos SON uniformes (no rechazo H0)

======================================================================
  TEST DE KOLMOGOROV-SMIRNOV
======================================================================
  Estadístico KS:   0.001254
  P-value:          0.086191
  Resultado:        PASS (alpha=0.05)

======================================================================
  TEST DE RUNS (RACHAS)
======================================================================
  Runs observados:  500220
  Runs esperados:   500000.75
  Z-score:          0.3101
  P-value:          0.756507
  Resultado:        PASS (alpha=0.05)

======================================================================
  TEST DE AUTOCORRELACIÓN
======================================================================
  Lags analizados:  20
  Límite confianza: ±0.0020
  Violaciones:      3 / 20
  Máximo permitido: 2
  Resultado:        FAIL

======================================================================
  ANÁLISIS DE ENTROPÍA
======================================================================
  Entropía:         7.9957 bits
  Entropía máxima:  8.0000 bits
  Ratio:            99.9459%
  Resultado:        PASS

======================================================================
  RESUMEN DE RESULTADOS
======================================================================
  Media y Varianza          PASS
  Uniformidad (Chi²)        PASS
  Kolmogorov-Smirnov        PASS

```

</details>