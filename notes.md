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
