<h1 align="center">ECC-Thesis</h1>

<p align="center">
   <img src="docs/Plantilla_TFG_latex/imagenes/torus.png" alt="Cypher Logo" width="100%">
</p>

# Project Description

This Computer Engineering Bachelor's Final Thesis for the University of Granada proposal focuses on the study of elliptic curves applied to cryptography, addressing both its theoretical foundations and its practical application in security systems. The mathematical basis of ECC, its implementations in comparison with traditional methods such as RSA and the discrete logarithm problem will be analyzed. In addition, the impact of quantum computing on the security of these systems will be explored and coding and visualization tools will be developed to illustrate its operation. The work aims to provide a comprehensive and up-to-date view on the potential of elliptic curves in modern cryptography.

# Project Structure

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
````

# Docker Image

We provide a containerized environment with **GMP 6.3.0** and **NTL 11.5.1** pre-installed. The `/workspace/bin` directory is automatically added to `PATH`, so any executable you compile there can be run by name.

## Files

* **Dockerfile**
  Builds on Ubuntu 22.04, installs build tools, GMP 6.3.0, then NTL 11.5.1.

* **docker-compose.yml**
  General-purpose compose file. Mounts your current project folder (`.`) into `/workspace`.

* **docker-compose.local.yml**
  (Optional) If you’re working from `/media/…` (like me) and need a custom host path, you can override the volume mount here.

## Usage

### 1. Build the image

From your project root:

```bash
docker-compose build
```

Or, if you want to use Docker directly:

```bash
docker build -t ecc-thesis .
```

### 2. Start a shell

With Compose:

```bash
docker-compose run crypto
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
ecc_demo
```

Since `bin/` is on `PATH`, you can just type the executable name.

---

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
