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

# Analisis de Optimizaciones ECC: Coordenadas Jacobianas y Campos Binarios

## 1. Por que empezamos con coordenadas afines

### 1.1 Decision de diseno educativa

La implementacion inicial de ECC uso exclusivamente **coordenadas afines** `(x, y)` por razones pedagogicas y de ingenieria:

**Claridad matematica**: La ecuacion de Weierstrass `y^2 = x^3 + ax + b (mod p)` se verifica directamente sustituyendo `(x, y)`. No hay capas de abstraccion entre la teoria y el codigo.

**Verificabilidad**: Despues de cada operacion (suma, doblado), el resultado se puede comprobar con `is_on_curve()`. En coordenadas Jacobianas, el mismo punto tiene infinitas representaciones equivalentes `(X, Y, Z) == (lambda^2 * X, lambda^3 * Y, lambda * Z)` para cualquier `lambda != 0`, lo que dificulta la comprobacion visual.

**Depuracion**: Cuando un test falla, las coordenadas afines permiten comparar directamente contra vectores de test conocidos (SEC 2, NIST). Con Jacobianas, hay que convertir primero y los errores intermedios son mas dificiles de rastrear.

**Correccion primero**: El principio de desarrollo fue "primero que funcione correctamente, luego optimizar". Las coordenadas afines proporcionan la base de referencia contra la que validar cualquier optimizacion.

### 1.2 El coste real de las coordenadas afines

El cuello de botella de las coordenadas afines es la **inversion modular**. Cada suma y cada doblado de puntos requiere calcular un inverso en `Fp`:

```
Suma:    lambda = (y2 - y1) * (x2 - x1)^(-1) mod p
Doblado: lambda = (3*x^2 + a) * (2*y)^(-1) mod p
```

La inversion modular usa el algoritmo extendido de Euclides, cuyo coste es `O(log^2 p)`, equivalente a **80-100 multiplicaciones modulares** para campos de 256 bits.

Para una multiplicacion escalar de 256 bits (la operacion fundamental de ECC):
- ~256 doblados + ~128 sumas = **~384 inversiones**
- Coste equivalente: **~30,000-38,000 multiplicaciones** solo en inversiones
- La aritmetica "util" (sumas y multiplicaciones) es una fraccion menor

---

## 2. Coordenadas Jacobianas: eliminando inversiones

### 2.1 La idea clave

Las coordenadas Jacobianas representan un punto afin `(x, y)` como una terna `(X, Y, Z)` donde:

```
x = X / Z^2
y = Y / Z^3
```

El punto en el infinito se representa con `Z = 0`.

La ventaja es que las formulas de suma y doblado se pueden reescribir **sin divisiones**, usando solo multiplicaciones y cuadrados:

**Doblado Jacobiano** (de `(X1, Y1, Z1)` a `(X3, Y3, Z3)`):
```
A = Y1^2
B = 4 * X1 * A
C = 8 * A^2
D = 3 * X1^2 + a * Z1^4
X3 = D^2 - 2*B
Y3 = D*(B - X3) - C
Z3 = 2 * Y1 * Z1
```
Coste: **4 multiplicaciones + 4 cuadrados**, 0 inversiones.

**Suma Jacobiana** (de `(X1,Y1,Z1)` + `(X2,Y2,Z2)`):
```
U1 = X1*Z2^2,  U2 = X2*Z1^2
S1 = Y1*Z2^3,  S2 = Y2*Z1^3
H = U2 - U1,   R = S2 - S1
X3 = R^2 - H^3 - 2*U1*H^2
Y3 = R*(U1*H^2 - X3) - S1*H^3
Z3 = H * Z1 * Z2
```
Coste: **12 multiplicaciones + 4 cuadrados**, 0 inversiones.

### 2.2 Analisis de coste comparativo

Sea `M` = multiplicacion modular, `S` = cuadrado modular (~0.8M), `I` = inversion (~80-100M).

| Operacion            | Afin              | Jacobiano     |
|----------------------|-------------------|---------------|
| Doblado              | 1I + 2M + 2S      | 4M + 4S       |
| Suma                 | 1I + 2M + 1S      | 12M + 4S      |
| Coste doblado (en M) | ~84M              | ~7.2M         |
| Coste suma (en M)    | ~83M              | ~15.2M        |

Para multiplicacion escalar de 256 bits (double-and-add):
- **Afin**: ~256 * 84M + ~128 * 83M = ~32,128M
- **Jacobiano**: ~256 * 7.2M + ~128 * 15.2M + 1 * 100M (inversion final) = ~3,892M

**Factor teorico: ~8.3x** mas rapido con Jacobianas.

En la practica, el speedup observado es de **3-5x** debido a:
- Overhead de la gestion del componente Z (mas datos por punto)
- Los cuadrados no son exactamente 0.8M en todos los casos
- Efectos de cache (puntos Jacobianos son 50% mas grandes)
- La inversion final sigue siendo costosa

### 2.3 Flujo de trabajo

```
Punto afin P = (x, y)
        |
        v  to_jacobian: (x, y) -> (x, y, 1)
Punto Jacobiano J = (x, y, 1)
        |
        v  ~256 doblados + ~128 sumas (0 inversiones)
Resultado Jacobiano R = (X, Y, Z)
        |
        v  to_affine: (X, Y, Z) -> (X/Z^2, Y/Z^3)  [1 inversion]
Resultado afin = (x', y')
```

---

## 3. Curvas sobre campos binarios GF(2^m)

### 3.1 Motivacion

Ademas de los campos primos `Fp`, las curvas elipticas se pueden definir sobre campos binarios `GF(2^m)`. Estas curvas tienen propiedades matematicas y de implementacion fundamentalmente diferentes que las hacen interesantes para el analisis comparativo.

### 3.2 Aritmetica en GF(2^m)

Los elementos de `GF(2^m)` son polinomios de grado menor que `m` con coeficientes en `{0, 1}`:

```
a(x) = a_{m-1} * x^{m-1} + ... + a_1 * x + a_0, donde a_i in {0, 1}
```

El campo se construye como `GF(2)[x] / f(x)` donde `f(x)` es un polinomio irreducible de grado `m`.

**Operaciones en GF(2^m)**:

| Operacion      | Fp (campo primo)           | GF(2^m) (campo binario)        |
|----------------|----------------------------|--------------------------------|
| Suma           | (a + b) mod p              | a XOR b (bit a bit)            |
| Resta          | (a - b) mod p              | a XOR b (identica a la suma!)  |
| Multiplicacion | (a * b) mod p              | Producto de polinomios mod f(x)|
| Cuadrado       | (a * a) mod p              | Insertar 0s entre bits + reducir|
| Inversion      | Euclides ext. / Fermat     | Euclides ext. para polinomios  |

La **suma = XOR** es la ventaja clave: es una unica instruccion de CPU, sin carries, sin propagacion, perfectamente paralelizable.

El **cuadrado** en GF(2^m) tambien es especialmente eficiente. Para `a = sum(a_i * x^i)`:
```
a^2 = sum(a_i * x^(2i))
```
Es decir, solo hay que "espaciar" los bits (insertar un 0 entre cada par) y reducir modulo `f(x)`. Esto es O(m) en vez de O(m^2).

### 3.3 Ecuacion de la curva

En GF(2^m), la forma de Weierstrass se modifica (la forma estandar `y^2 = x^3 + ax + b` no funciona en caracteristica 2):

```
y^2 + xy = x^3 + ax^2 + b
```

Esta es la forma **no supersingular** (la unica segura criptograficamente). La presencia del termino `xy` es necesaria porque en caracteristica 2, `2y = 0`, lo que anularia la derivada parcial respecto a `y` en la forma estandar.

### 3.4 Operaciones de punto

**Suma** P + Q (P != Q):
```
lambda = (y1 + y2) / (x1 + x2)     [+ es XOR]
x3 = lambda^2 + lambda + x1 + x2 + a
y3 = lambda * (x1 + x3) + x3 + y1
```

**Doblado** 2P:
```
lambda = x + y/x
x3 = lambda^2 + lambda + a
y3 = x^2 + (lambda + 1) * x3
```

**Negacion**: `-P = (x, x + y)` (no `(x, -y)` como en Fp, ya que `-y = y` en GF(2^m))

### 3.5 Curvas Koblitz

Las curvas Koblitz son un caso especial donde `a in {0, 1}`. Esto permite usar el **endomorfismo de Frobenius** `phi(x, y) = (x^2, y^2)` para acelerar la multiplicacion escalar, ya que en GF(2^m) el cuadrado es lineal y muy eficiente.

En nuestra implementacion incluimos:
- **sect163k1**: Koblitz, 163 bits, ~80 bits de seguridad (a=1, b=1)
- **sect233k1**: Koblitz, 233 bits, ~112 bits de seguridad (a=0, b=1)
- **sect283k1**: Koblitz, 283 bits, ~128 bits de seguridad (a=0, b=1)

Y curvas aleatorias para comparacion:
- **sect233r1**: Aleatoria, 233 bits
- **sect283r1**: Aleatoria, 283 bits

### 3.6 Campos binarios vs campos primos: resumen comparativo

| Aspecto              | Fp (primos)                | GF(2^m) (binarios)         |
|----------------------|----------------------------|----------------------------|
| Estandarizacion      | NIST, ampliamente usado    | NIST (retirando), SEC 2    |
| Suma                 | Suma modular               | XOR (muy rapida)           |
| Multiplicacion SW    | Buena (librerias maduras)  | Mas compleja en software   |
| Multiplicacion HW    | Requiere sumadores         | Carry-free, natural en HW  |
| Cuadrado             | O(n^2) (como mult.)       | O(n) (lineal, Frobenius)   |
| Seguridad            | Muy estudiada              | Preocupaciones (Weil desc.)|
| Tendencia industria  | Dominante y creciendo      | Uso decreciente            |
| Valor educativo      | Alto                       | Alto (perspectiva diferente)|

---

## 4. Implicaciones para el TFG

### 4.1 Tres dimensiones de comparacion

Con estas implementaciones, el proyecto ofrece comparacion en tres ejes:

1. **RSA vs ECC**: Algoritmos fundamentalmente diferentes (factorizacion vs ECDLP)
2. **Afin vs Jacobiano**: Misma matematica, diferente representacion (claridad vs rendimiento)
3. **Campo primo vs campo binario**: Misma estructura algebraica, diferente aritmetica

### 4.2 Metodologia experimental recomendada

Para el capitulo de resultados experimentales del TFG:

1. **Benchmark afin vs Jacobiano** en las mismas curvas (P-256, P-384, secp256k1)
   - Medir speedup real del Jacobiano
   - Comparar con factor teorico (~3-5x esperado)
   - Analizar donde se pierde el factor teorico de ~8x

2. **Benchmark campo primo vs campo binario** al mismo nivel de seguridad
   - P-256 (~128 bits) vs sect283k1 (~128 bits)
   - Analizar que es mas rapido en software y por que

3. **Profiling** de operaciones individuales
   - Tiempo en inversiones (afin) vs multiplicaciones extra (Jacobiano)
   - Desglose por operacion: keygen, sign, verify, ECDH