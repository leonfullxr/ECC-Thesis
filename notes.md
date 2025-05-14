# Compile

```bash
export CXXFLAGS="-I/home/leon/sw/include"
export LDFLAGS="-L/home/leon/sw/lib -Wl,-rpath=/home/leon/sw/lib"
PREFIX=/home/leon/sw GMP_PREFIX=/home/leon/sw
export LD_LIBRARY_PATH="/home/leon/sw/lib:$LD_LIBRARY_PATH"
g++ -std=c++17 \                                                         ─╯
    -I/home/leon/sw/include \
    src/rsa.cpp \      
    -L/home/leon/sw/lib \
    -Wl,-rpath=/home/leon/sw/lib \
    -lntl -lgmp -pthread \
    -o ./bin/RSA
```

## 1. Pushing your RSA branch up to the remote

Assuming you’ve done work on your local `rsa-implementation` branch:

```bash
# 1. Make sure you’re on your RSA branch:
git checkout rsa-implementation

# 2. Stage and commit your work:
git add .
git commit -m "Add RSA implementation"

# 3. Push it to origin (remote):
git push origin rsa-implementation
```

Now you can open a Pull Request (PR) on your Git host (GitHub/GitLab/Bitbucket/etc.) **from** `rsa-implementation` **into** `main`. That PR is how you’ll ultimately get those commits merged into `main`.

---

## 2. Merging your RSA branch into `main`

Once your RSA PR is approved, you have two equivalent options:

### A. Via the Git host UI (*recommended*)

1. Click **Merge** (or **Squash and merge**) in the PR on GitHub/GitLab.
2. That action will merge the commits on `rsa-implementation` into `main` and update the remote `main` branch.

### B. Locally, then push

```bash
# 1. Switch to main
git checkout main

# 2. Make sure it’s up to date
git pull origin main

# 3. Merge in your feature branch
git merge rsa-implementation

# 4. Push the updated main
git push origin main
```

---

## 3. Bringing your documentation changes into your RSA branch

Suppose you have a separate branch `docs-updates` with commits that you also want in your RSA branch. You have two principal choices:

### A. **Merge** `docs-updates` into `rsa-implementation`

```bash
# 1. Fetch the latest:
git fetch origin

# 2. Check out your RSA branch:
git checkout rsa-implementation

# 3. Merge in the docs branch:
git merge origin/docs-updates
```

Resolve any conflicts, then:

```bash
# 4. Push the result
git push origin rsa-implementation
```

This creates a merge commit on `rsa-implementation` that brings in all the documentation commits.

### B. **Rebase** your RSA branch onto `docs-updates`

```bash
git fetch origin
git checkout rsa-implementation
git rebase origin/docs-updates
```

This “rewrites” your RSA commits so they sit on top of the documentation commits. After a rebase you’ll need:

```bash
git push --force-with-lease origin rsa-implementation
```

> **When to merge vs. rebase?**
>
> * **Merge** if you want a record of when you combined the two branches (adds a merge commit).
> * **Rebase** if you prefer a linear history (no merge commit), but only on branches that aren’t yet shared—or if you’re comfortable force-pushing.

---

## 4. Propagating docs → main → RSA (alternative workflow)

A common pattern is:

1. **Merge** `docs-updates` into `main` first.
2. **Merge** (or **rebase**) **latest** `main` into your `rsa-implementation` branch, so that RSA sees the docs:

   ```bash
   git checkout main
   git pull origin main          # get the merged docs
   git checkout rsa-implementation
   git merge main               # or `git rebase main`
   git push origin rsa-implementation
   ```

That way you only ever merge docs into one place (`main`), and then pull them downstream into every feature branch that needs them.

---

### TL;DR

* **Feature branches**: one per logical change (e.g. `docs-updates`, `rsa-implementation`).
* **To get a feature into `main`**: open (or locally run) a **merge** or **rebase** from your feature-branch into `main`, then `git push origin main`.
* **To share commits between two feature branches**: check out the branch that needs the commits and **merge** (or **rebase**) the other branch into it.


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
