## 📝 Notas de Implementación

### NTL

- Usamos `SetSeed(ZZ)` para inicializar RNG
- `GenPrime()` usa Miller-Rabin internamente
- `PowerMod()` para exponenciación modular
- `InvMod()` para inversos modulares
- Todo es thread-safe según documentación

### C++17

- `std::unique_ptr` para ownership claro
- Templates para benchmarking genérico
- Lambdas para operaciones
- Chrono para timing preciso

### Cuestiones (Compilador, optimizaciones, CPU affinity)

Lo dejamos en [optimi­zaciones](./optimization.md) para mas detalle.

### Testing

- Usa semilla fija para debugging
- Múltiples iteraciones para eliminar ruido
- Compara resultados esperados
- Valida con assertions

### Pendientes (Trabajo Futuro)

- [ ] Modo paralelo (`-m par`)
- [ ] Exportar resultados a CSV/JSON
- [ ] Padding OAEP para RSA seguro
- [ ] Gráficos de comparación
- [ ] Análisis estadístico avanzado

### RSA vs ECC
#### Niveles de Seguridad Aproximados
| Seguridad (bits) | RSA (bits) | ECC (bits) | Relación de Seguridad |
|------------------|------------|------------| -----------------------|
| 80               | 1024       | 160        | débil
| 112              | 2048       | 224        | mínimo recomendado
| 128              | 3072       | 256        | estándar
| 192              | 4096       | 384        | alta seguridad


### TL;DR

1. **PR: RSA Core**

   * Implementa `rsa.hpp`/`rsa.cpp` con generación de claves, cifrado y descifrado.
   * Incluye tests unitarios básicos (p. ej. generar clave, cifrar/des­cifrar un número pequeño).

2. **PR: ECC Core**

   * Define `ecc.hpp`/`ecc.cpp` con soporte para al menos una curva estándar (e.g. secp256k1).
   * Añade generación de claves ECC y operaciones punto (suma, multiplicación escalar).
   * Incluye tests unitarios (comprueba que la clave pública corresponde al privado).

3. **PR: RNG Abstraction & NTLRNG**

   * Crea la interfaz `RNG` y la implementación `NTLRNG`.
   * Testea que, dado un seed fijo, `get_random_bits` sea reproducible.

4. **PR: Benchmark Harness (`main.cpp`)**

   * Añade el programa de benchmarking genérico, parsing de flags y salida formateada.
   * Cubre RSA y ECC con medición de tiempos.

5. **PR: Makefile y Scripts de Ejecución**

   * Actualiza el Makefile con targets `all`, `test-rsa`, `test-ecc`, `clean`, preámbulo y colores.
   * Incorpora `run_bench.sh` u otros scripts de automatización (e.g. `run_bench_parallel.sh`).

6. **PR: Análisis de Resultados**

   * Añade un script Python (o Jupyter) para procesar los CSV generados, calcular medias/desviaciones, percentiles y generar gráficos (boxplots, histogramas).
   * Incluir un ejemplo de salida/grafico de comparación RSA vs ECC.

7. **PR: Documentación**

   * Incluye un `README.md` con descripción del proyecto, instrucciones de compilación, ejecución de tests y ejemplos.
   * Añade `docs/proyecto.md` con diseño de la arquitectura, descripción de módulos y criterios de medición.

8. **PR: CI / GitHub Actions**

   * Configura un pipeline que compile el proyecto, ejecute `make test` y valide los scripts de análisis.
   * Opcionalmente, publique los resultados de benchmarking como artefactos.

9. **PR: Docker / Contenedor Reproducible**

   * Añade un `Dockerfile` que instale NTL/GMP, compile el proyecto y permita ejecutar los benchmarks con entorno controlado.

10. **PR: Tests Unitarios & Coverage**

    * Integra un framework de tests en C++ (Google Test, Catch2) para cubrir RSA, ECC y RNG.
    * Genera informe de cobertura (lcov).

11. **PR: Soporte Extendido de Curvas y Parámetros**

    * Permite elegir curvas ECC adicionales (`prime256v1`, `brainpoolP256r1`, etc.) y distintos exponentes RSA.
    * Añade validación de parámetros y tests correspondientes.

12. **PR: Optimización y Afinamiento**

    * Añade opciones de compilación (`-march=…`, perfiles PGO).
    * Integra Warm-up automático, CPU affinity y governor en el harness.

Cada PR debería centrarse en una única área de responsabilidad y venir acompañado de su propio conjunto de tests o ejemplos de uso, de modo que la revisión sea sencilla y el historial de cambios claro.


# ECC notes

## Basic Operations

Last but surely not least, ECC hinges on a collection of basic operations, which are also crucial for the implementation
of higher-level cryptographic protocols. The operations benchmarked were:
• Scalar Multiplication: This is the most compute-heavy operation in ECC, as it corresponds to adding an elliptic
curve point P by a scalar k multiple times. Scalar multiplication is fundamental to ECC because it forms the basis for other
functions such as key generation and signature schemes. Its efficiency is essential for gauging whether ECC is viable
on resource-limited platforms.
• Point Addition: This operation takes the two points P, Q on the elliptic curve to get R = P + Q. Although point
addition is lighter than scalar multiplication, it is also carried out many times within higher-level operations, e.g., scalar
multiplication and key exchange.
• Small Scalar Multiplication: A Special case of scalar multiplication where the scalar value k is small. This is an
operation that encrypts one bit of data, and is typically used to be lightweight cryptographic applications like session key
generation in IoT devices where cryptographic strength is compromised for speed.
Execution time, memory usage, and energy consumption were measured for these basic operations to obtain a detailed
resource profile and identify potential bottlenecks

## Higher Level Operations

High-level cryptographic operations use the basic operations to create secure protocols and real-world applications. The
following high-level operations were benchmarked:
• Encryption/ Decryption: Encryption enables encoding a plaintext to a cipher text using a recipient's public key, whereas
decryption takes an encrypted code to get the original plaintext back using the corresponding private key. ECCbased encryption systems are very efficient in terms of traditional node systems, such as RSA, especially on resourceconstrained platforms.
Signature Generation and Verification: Signature generation is a mechanism to form the digital signature by using the
sender’s private key in order to guarantee message authenticity and integrity. Signature verification makes use of the
sender’s public key to check the legitimacy of the signature. Such operations are a core requirement for secure
communication protocols and are widely applicable in IoT systems and blockchain.
• Key Exchange (ECDH): The Elliptic Curve Diffie-Hellman (ECDH) algorithm allows two parties to securely agree to a
shared secret over an insecure channel. Given the decisive role that scalar multiplication has on ECDH performance, it is
fundamental to offer an extensive exploration of its construction, especially when considering its practical employment for
the purpose of secure key exchange.
To provide consistency and reliability, each of these operations was benchmarked several times. The performance data
collected offers valuable insights into the computational requirements and trade-offs involved in utilizing ECC for protocol
implementation on the Raspberry Pi platform. The goal of the study is to provide insights into ECC optimization by profiling
the resource-intensive operations in the ECC. By getting an insight into the resource requirements of such operations, the
study aims to optimize ECC for resource-poor environments.

Reference: https://www.ijirss.com/index.php/ijirss/article/download/6195/1165/9774#:~:text=The%20average%20execution%20time%20to,operations%2C%20Resource%2Dconstrained%20platforms.
