# Puntos clave para la defensa del TFG: RSA vs ECC

**Autor:** León Elliott Fuller
**Proyecto:** Implementación y benchmarking de RSA y ECC en C++17
**Fecha:** 2026-06-07

> Documento interno de preparación para la defensa. No forma parte de la memoria.
> Reúne las explicaciones que conviene tener muy claras, aclara qué se ha
> implementado realmente (y qué no), recoge los datos clave, la profundidad
> algebraica para el tribunal de Álgebra, las inconsistencias del documento a
> revisar, y respuestas a preguntas típicas del tribunal.

## Índice

- **Parte 1** — Qué se ha implementado realmente (y qué no)
- **Parte 2** — Datos y resultados (tablas para memorizar)
- **Parte 3** — Profundidad teórica (el tribunal de Álgebra)
- **Parte 4** — Ataques (al ECDLP y a las implementaciones)
- **Parte 5** — Protocolos (ECDH y ECDSA)
- **Parte 6** — Metodología: por qué el benchmark es ciencia
- **Parte 7** — Estándares, contexto y contribución
- **Parte 8** — Inconsistencias y puntos débiles del documento (revisar antes de entregar)
- **Parte 9** — Batería de preguntas del tribunal (respuestas rápidas)
- **Parte 10** — Logística de la defensa y mapa pregunta → diapositiva

---

# PARTE 1 — Qué se ha implementado realmente (y qué no)

## 1.1 Arquitectura del software

```
main.cpp
├── rsa.{hpp,cpp}       → RSA completo: KeyGen, Encrypt, Decrypt, Decrypt CRT, Sign, Verify
├── ecc.{hpp,cpp}       → ECC sobre Fp: coordenadas afines Y Jacobianas, ECDH, ECDSA
├── ecc_binary.{hpp,cpp}→ ECC sobre F₂ₘ: solo afines, KeyGen, Scalar mult, ECDH
├── sha256.{hpp,cpp}    → SHA-256 desde cero (FIPS PUB 180-4)
├── rng.{hpp,cpp}       → Generador de números aleatorios (NTL, seed fija/aleatoria)
└── common.hpp          → BigInt (alias de NTL::ZZ), constantes globales
```

- **Lenguaje:** C++17, compilado con `g++ -std=c++17 -O2`.
- **Bibliotecas:** NTL 11.5.1 (aritmética modular y polinómica) sobre GMP 6.3.0 (motor de precisión arbitraria). OpenSSL 3.0.2 solo como baseline externo.
- **Entorno:** contenedor Docker con Ubuntu 22.04 para reproducibilidad bit a bit.
- **Motor de medición:** 3 iteraciones de calentamiento descartadas + 100 medidas con `std::chrono::high_resolution_clock`, resolución de µs, reporta la mediana.
- **Modo CMP:** ejecuta toda la suite automáticamente y genera **72 benchmarks** individuales (RSA 24, ECC 𝔽ₚ afín 18, ECC 𝔽ₚ Jacobiano 15, ECC 𝔽₂ₘ 15).

## 1.2 RSA: cómo está implementado

Hay que poder describir RSA "de memoria" porque es la mitad del trabajo. Módulo `rsa.{hpp,cpp}`.

**Generación de claves (`RSA::generate_key`):**

1. Se generan **dos primos** `p` y `q` de `bits/2` cada uno con un generador de primos probables (test de **Miller-Rabin**), garantizando además `gcd(e, p-1) = 1` y `gcd(e, q-1) = 1`.
2. Se asegura `q != p` y, por convención, `p > q`.
3. `n = p · q` (el módulo).
4. `phi = (p-1)(q-1)` (función de Euler).
5. `d = e⁻¹ mod phi` (inverso modular del exponente público).
6. Exponente público fijo `e = 65537` (0x10001): 17 bits, solo dos bits a 1, lo que hace muy rápidas las operaciones públicas.

**Operaciones:**

- **Cifrado:** `c = mᵉ mod n` (operación pública, barata).
- **Descifrado directo:** `m = cᵈ mod n` (operación privada, cara: d tiene la longitud completa del módulo).
- **Firma:** `s = hᵈ mod n` (en esencia un descifrado del hash; reúsa el mismo código que decrypt).
- **Verificación:** `h = sᵉ mod n` (operación pública, barata).

**Asimetría RSA (clave para la comparación):** cifrado/verificación ~3-38 µs; descifrado/firma ~179-8770 µs según tamaño. Es estructural y explica todos los resultados RSA vs ECC.

**Qué NO es (limitación honesta):** es **textbook RSA**, sin padding OAEP (cifrado) ni PSS (firma). Correcto matemáticamente y suficiente para medir rendimiento, pero **no es seguro para producción** (determinista, maleable). Documentado como limitación.

## 1.3 El Teorema Chino del Resto (CRT): SÍ está implementado

Era una duda explícita: **sí, está implementado y funcionando.** Las referencias al CRT en los capítulos 6 y 7 son correctas; no hay que quitar nada.

- Cap. 6.2.1: "rsa maneja todo el ciclo de vida de este algoritmo (incluyendo la optimización del Teorema Chino del Resto para el descifrado)".
- Cuadro 6.2: "Decrypt CRT – Descifrado optimizado (Teorema Chino del Resto)" aparece como operación medida.

**Dónde está (en `rsa.cpp`):**

- `RSAPrivateKey::compute_crt_params()` precalcula y guarda: `dp = d mod (p-1)`, `dq = d mod (q-1)`, `qinv = q⁻¹ mod p`.
- `RSA::decrypt_crt()` hace el descifrado optimizado.

**Cómo funciona (recombinación de Garner):** en lugar de `m = cᵈ mod n` (una exponenciación con el módulo grande n), se calcula:

```
m1 = (c mod p)^dp mod p
m2 = (c mod q)^dq mod q
h  = (m1 - m2) * qinv mod p
m  = m2 + h * q
```

Es decir, **dos exponenciaciones con módulos de la mitad de tamaño** (p y q) en vez de una con el módulo completo n.

**Por qué acelera (~3,5× medido):** el coste de una exponenciación modular crece muy rápido con el tamaño del módulo (≈O(k³)). Dos exponenciaciones de k/2 bits cuestan en teoría ~4× menos que una de k bits. Medimos **~3,5×** (de 3792 µs a 1089 µs para claves de 3072 bits). La diferencia con el 4× teórico se debe al coste extra de la recombinación y las reducciones `c mod p`, `c mod q`.

## 1.4 ECC sobre F_p: afín y Jacobiano (BigInt vs ZZ_p)

Módulo `ecc.{hpp,cpp}`. Curvas en forma corta de Weierstrass `y² = x³ + ax + b (mod p)`. Implementa **ambos** sistemas de coordenadas (afín y Jacobiano), y el booleano `use_jacobian` recorre keygen/ECDH/ECDSA para seleccionar el backend: así se mide el eje 2 con entradas idénticas.

**Diferencia entre los tipos `BigInt`, `ZZ_p` y `GF2E`:**

- **`BigInt`:** alias/typedef en `common.hpp` sobre `NTL::ZZ` → entero de precisión arbitraria SIN módulo asociado. Se usa para exponentes, claves RSA y aritmética intermedia antes de reducir.
- **`ZZ_p`:** tipo NTL para elementos de ℤ/pℤ → cuerpo primo Fp con módulo implícito p fijo. Se usa para coordenadas de puntos ECC en curvas sobre Fp. Las operaciones (+, ·, /) se reducen automáticamente módulo p.
- **`GF2E`:** tipo NTL para F₂ₘ → elementos del cuerpo binario (polinomios mod f(x)). Se usa en `ecc_binary` para coordenadas en curvas binarias.
- La ventaja de `ZZ_p` y `GF2E` es que la reducción es automática y transparente; `BigInt` es más flexible para operaciones que no viven en un cuerpo fijo.

## 1.5 ECC sobre F_2^m: solo afín (Koblitz y binarias)

Módulo `ecc_binary.{hpp,cpp}`. Cinco curvas SEC 2 (sect163k1, sect233k1, sect283k1, sect233r1, sect283r1); las de sufijo `k` son de Koblitz (a ∈ {0,1}). Ecuación no supersingular `y² + xy = x³ + Ax² + B`.

**La frase del Cap. 8 (limitación 4), traducida frase a frase:**

> "Las métricas obtenidas para las curvas de Koblitz y la aritmética binaria se calcularon exclusivamente operando en coordenadas afines, asumiendo la consecuente penalización por inversión, al no haberse portado el modelo Jacobiano proyectivo a los campos polinómicos."

1. **"curvas de Koblitz y aritmética binaria"** → todo lo que vive en F₂ₘ.
2. **"se calcularon exclusivamente operando en coordenadas afines"** → en el módulo binario los puntos solo existen en coordenadas afines `(x, y)`. **No hay versión Jacobiana/proyectiva para binario.** El optimizador Jacobiano solo se implementó para Fp (modo `ECCJ`).
3. **"asumiendo la consecuente penalización por inversión"** → en afín, **cada** suma o duplicación necesita una inversión en el cuerpo (el denominador de la pendiente λ). En proyectivo esa inversión se difiere a una sola al final. Como en binario solo tenemos afín, **pagamos una inversión por cada paso del grupo**.
4. **"al no haberse portado el modelo Jacobiano proyectivo"** → no se escribieron las fórmulas proyectivas para F₂ₘ (que además son distintas a las de Fp: cambia el negativo `-P=(x, x+y)`, cambian las fórmulas de doubling). El análogo proyectivo en cuerpos binarios son las **coordenadas de López-Dahab**; no se implementaron. Fue una cuestión de alcance/tiempo.

**Consecuencia para la comparativa Fp vs F₂ₘ:** se compara Fp **en afín** (secp256k1 afín, 722 µs) contra binario **en afín** (sect283k1, 1243 µs). Es justa porque ambas están en afín. La diferencia (~1,7× a favor de Fp) **no es matemática sino de software** (ver Parte 3.6). Documentado en Cap. 6.8 y Cap. 8.4 (punto 4) como trabajo futuro.

## 1.6 SHA-256 desde cero y RNG con semilla fija

- **SHA-256 desde cero (FIPS PUB 180-4):** decisión académica, para entender el hash que acompaña a ECDSA y para que ninguna actualización externa altere los tiempos medidos. Compatible con los vectores de prueba de NIST.
- **RNG con semilla fija:** usa la interfaz RNG de NTL. Con `-s fixed` (seed=0) la secuencia es idéntica en cada ejecución, lo que garantiza que todos los algoritmos se comparan frente a los mismos valores (comparación pareada). Con `-s random`, toma el tiempo del sistema. Validado estadísticamente: uniformidad, rachas, autocorrelación y entropía (todos superaron los umbrales).

## 1.7 Curvas implementadas

**Sobre cuerpos primos Fp (módulo `ecc`):**

| Curva               | Bits | a  | h | Seguridad |
|---------------------|------|----|---|-----------|
| NIST P-256          | 256  | -3 | 1 | ~128 bits |
| NIST P-384          | 384  | -3 | 1 | ~192 bits |
| secp256k1 (Bitcoin) | 256  | 0  | 1 | ~128 bits |

**Sobre cuerpos binarios F₂ₘ (módulo `ecc_binary`, estándar SEC 2):**

| Curva     | m   | h | Seguridad | Tipo      | Polinomio irreducible |
|-----------|-----|---|-----------|-----------|-----------------------|
| sect163k1 | 163 | 2 | ~80 bits  | Koblitz   | x¹⁶³+x⁷+x⁶+x³+1        |
| sect233k1 | 233 | 4 | ~112 bits | Koblitz   | x²³³+x⁷⁴+1             |
| sect283k1 | 283 | 4 | ~128 bits | Koblitz   | x²⁸³+x¹²+x⁷+x⁵+1       |
| sect233r1 | 233 | 2 | ~112 bits | Aleatoria | x²³³+x⁷⁴+1             |
| sect283r1 | 283 | 2 | ~128 bits | Aleatoria | x²⁸³+x¹²+x⁷+x⁵+1       |

**Nota importante:** las curvas Koblitz con h=4 (y en general h>1) requieren verificación adicional `n·Q=O` para validar claves públicas correctamente. Se documenta como limitación del prototipo.

## 1.8 Qué NO se ha implementado (limitaciones honestas — Cap. 6.8)

Mostrar honestidad sobre el alcance da credibilidad; decirlo **antes** de que lo pregunten.

1. **Sin protección contra canales laterales:** `double-and-add` clásico depende de los bits de k.
2. **Riesgo en firmas ECDSA:** nonce aleatorio estándar, NO determinista (RFC 6979).
3. **Derivación de secretos básica:** coordenada X directa, sin KDF robusta (HKDF, RFC 5869).
4. **RSA "textbook":** sin relleno OAEP/PSS.
5. **Validación de curvas asumida:** confiamos en NIST/SECG; no se verifica primalidad de n, no-anomalía, grado de inmersión ni `n·Q=O` para h>1 en tiempo de ejecución.
6. **ECDSA no implementada en curvas binarias** (solo KeyGen, mult. escalar y ECDH).
7. **Coordenadas Jacobianas solo en Fp, no en F₂ₘ** (López-Dahab no portado).
8. **Nivel de seguridad 192 bits (RSA-7680) no evaluado experimentalmente** (generar miles de claves de 7680 bits excedía los tiempos razonables).
9. **NAF / precomputación:** estudiadas, parcialmente pendientes (trabajo futuro).
10. **Generación propia de curvas (algoritmo SEA):** se usan solo curvas estándar.

---

# PARTE 2 — Datos y resultados (tablas para memorizar)

Hardware: AMD Ryzen 9 7950X, 64 GiB RAM, Docker Ubuntu 22.04, `gcc -O2` (sin `-march=native`). Mediana de 100 iteraciones, 3 de calentamiento descartadas.

**Los 4 números estrella (de memoria):** keygen ECC ~138× sobre RSA · CRT ~3,5× · Jacobiano práctico ~1,8× · ratio de tamaño de clave ~12×.

## 2.1 RSA (µs salvo donde se indica)

| Operación               | 1024b | 2048b  | 3072b  | 4096b   |
|-------------------------|-------|--------|--------|---------|
| Generación clave        | 1.9ms | 16.9ms | 58.1ms | 155.9ms |
| Cifrado                 | 3µs   | 10µs   | 22µs   | 38µs    |
| Descifrado directo      | 179µs | 1245µs | 3792µs | 8770µs  |
| Descifrado CRT          | 71µs  | 360µs  | 1089µs | 2492µs  |
| Firma (=Descifrado CRT) | 57µs  | 359µs  | 1089µs | 2490µs  |
| Verificación            | 3µs   | 10µs   | 22µs   | 38µs    |

## 2.2 ECC primas (coordenadas afines, mediana 100 iter., µs)

| Operación          | secp256k1 | P-256 | P-384 |
|--------------------|-----------|-------|-------|
| Generación clave   | 721       | 766   | 1540  |
| Mult. escalar      | 722       | 756   | 1531  |
| ECDH               | 731       | 771   | 1523  |
| Firma ECDSA        | 725       | 771   | 1536  |
| Verificación ECDSA | 1436      | 1522  | 3193  |

## 2.3 Mejora Jacobianas sobre Afines

| Curva     | Operación     | Factor |
|-----------|---------------|--------|
| secp256k1 | Mult. escalar | 1.72×  |
| P-256     | Mult. escalar | 1.70×  |
| P-384     | Mult. escalar | 1.88×  |
| P-384     | Verificación  | 1.96×  |

El beneficio crece con el tamaño del cuerpo.

## 2.4 Comparativa RSA-3072 vs ECC secp256k1 Jacobiana (128 bits)

| Operación                  | RSA-3072 | ECC Jacobiana | Ventaja              |
|----------------------------|----------|---------------|----------------------|
| Generación clave           | 58100µs  | 420µs         | ECC ≈138× más rápida |
| Firma / Op. privada        | 1089µs   | 415µs         | ECC ≈2,6× más rápida |
| Verificación / Op. pública | 22µs     | 838µs         | RSA ≈38× más rápida  |

## 2.5 Opciones de compilación (secp256k1, mult. escalar)

- `-O0`: 976µs (base)
- `-O1`: 758µs (1.29×)
- `-O2`: 743µs (1.31×) ← estándar del proyecto
- `-O3`: 769µs (1.27×) ← ¡más lento que -O2! (code bloat en caché L1)
- `-Ofast`: 754µs (1.29×)
- El cuello de botella real está en GMP (precompilado), no en el C++ propio.

## 2.6 Los tres ejes de comparación (estructura del Cap. 7)

1. **RSA vs ECC** (a niveles de seguridad equivalentes según NIST SP 800-57):
   - ECC domina en generación de claves (~138×) y firma (~2,6×).
   - RSA domina en verificación (~38×) gracias a e=65537.
   - La elección óptima depende del caso de uso (servidor emisor masivo de firmas → ECC; cliente que valida certificados → RSA o indiferente).
2. **Coordenadas afines vs Jacobianas** (para ECC sobre Fp):
   - Mejora 1,70×–1,96× según curva y operación; el beneficio crece con el tamaño del cuerpo.
   - Más conservador que los modelos teóricos (~8×) porque GMP optimiza parcialmente la inversión.
3. **Cuerpos primos Fp vs cuerpos binarios F₂ₘ** (a igual nivel de seguridad):
   - En software puro con NTL/GMP: Fp ≈1,7× más rápido que F₂ₘ.
   - Consecuencia del software (ZZ_p→GMP ensamblador vs GF2E genérico), no una inferioridad matemática intrínseca.

---

# PARTE 3 — Profundidad teórica (el tribunal de Álgebra)

El tutor es del **Departamento de Álgebra**. Aquí es donde se juega la matrícula: no basta con saber USAR la curva; hay que entender la ESTRUCTURA.

## 3.1 char(K): por qué 2 y 3 son especiales

La característica de un cuerpo K es el menor entero n > 0 tal que sumar el 1 consigo mismo n veces da 0. Para cuerpos finitos siempre es un **primo p**. En el proyecto: en Fp (p > 3), char(K) = p; en F₂ₘ, char(K) = 2.

**El matiz clave: 2 y 3 son las excepciones, no la norma.** La forma corta de Weierstrass `y² = x³ + ax + b` solo se obtiene cuando se puede **dividir por 2 y por 3**:

- **Completar el cuadrado** para eliminar `a1·x·y` y `a3·y`. Requiere que 2 sea invertible (char ≠ 2).
- **Completar el cubo** para eliminar `a2·x²`. Requiere dividir por 3 (char ≠ 3).

Por tanto, cuando char(K) = 2 o 3 la simplificación estándar se rompe y hay formas reducidas distintas (Cap. 4.3):

- **char ≠ 2,3** → forma corta `y² = x³ + ax + b` (la que usamos en Fp).
- **char = 2** → `y² + xy = x³ + Ax² + B` (no supersingular, la binaria) o `y² + Cy = x³ + Ax + B` (supersingular).
- **char = 3** → `y² = x³ + Ax² + B` (no la usamos; aparece solo por completitud).

**Señal con el discriminante:** `Δ = -16(4a³ + 27b²)`. El factor 16 = 2⁴ se anula en char 2 y el 27 = 3³ en char 3: la "huella" de esas dos divisiones.

> Respuesta de defensa: "Usamos char ≠ 2,3 para los primos (p > 3), lo que da la forma corta; y char = 2 para los binarios, con `y²+xy=x³+Ax²+B`. La característica 3 aparece por completitud teórica, sin uso práctico en criptografía estándar."

## 3.2 La ley de grupo: asociatividad, punto del infinito, Picard

Los puntos de E sobre K, más el punto del infinito O, forman un **grupo abeliano** (E(K), +). De los axiomas:

- **Neutro O, inversos (-P es el reflejo), conmutatividad:** triviales (la recta por P y Q es la misma que por Q y P).
- **Asociatividad `(P+Q)+R = P+(Q+R)`:** es el axioma DIFÍCIL y la pregunta estrella. No es obvio geométricamente. Dos vías clásicas:
  1. **Teorema de Cayley-Bacharach / cuentas con las nueve intersecciones** de dos cúbicas (argumento geométrico-proyectivo).
  2. **Vía el grupo de Picard `Pic⁰(E)`:** se identifica cada punto con una clase de divisores de grado 0; la suma de puntos corresponde a la suma en el grupo de clases, asociativa por construcción (Riemann-Roch). Es la respuesta "de matemático" y la que más impresiona.

> Si preguntan: *"es el único axioma no trivial; se prueba con Cayley-Bacharach o, más elegante, viendo E como su grupo de clases de divisores Pic⁰"*.

**Punto del infinito y lo proyectivo:** la forma afín no "ve" el neutro. O vive en el **plano proyectivo** P²: el único punto en la recta del infinito por el que pasan todas las rectas verticales `x = cte`. Por eso la suma necesita el modelo proyectivo para ser un grupo cerrado, y por eso las coordenadas Jacobianas (X:Y:Z) no son solo una optimización: son la forma natural del objeto.

## 3.3 Teorema de Hasse y estructura del grupo

- **Hasse:** `#E(F_q) = q + 1 - t`, con `|t| ≤ 2√q` (t = traza). El orden está "cerca" de q.
- **Estructura (teorema):** `E(F_q) ≅ Z/n1 ⊕ Z/n2` con `n1 | n2` y `n1 | (q-1)`. En cripto buscamos que sea **cíclico de orden primo** (o casi): `#E = h·n`, n primo grande, cofactor h pequeño.
- **Por qué orden primo / cofactor pequeño:** si #E factoriza en primos pequeños, **Pohlig-Hellman** descompone el ECDLP en logaritmos en subgrupos pequeños y lo resuelve barato. El factor primo grande n da la seguridad ~√n.

## 3.4 Por qué ECC tiene claves mucho más pequeñas (el corazón del TFG)

- La seguridad de **RSA** depende de la **factorización de enteros**, con algoritmos **subexponenciales** (GNFS, complejidad L_n[1/3]).
- La seguridad de **ECC** depende del **ECDLP**. Para curvas bien elegidas **no se conoce ningún algoritmo subexponencial**: el mejor ataque genérico es **rho de Pollard**, O(√n), totalmente exponencial en bits.

Resultado: para 128 bits de seguridad basta una curva de **~256 bits** (√(2²⁵⁶) = 2¹²⁸) frente a un módulo **RSA de ~3072 bits**. Por eso las claves ECC son ~12× más cortas a igual seguridad.

## 3.5 Afín vs Jacobiano: teoría (~8×) vs práctica (~1,8×)

Las coordenadas Jacobianas representan (x,y) como (X:Y:Z) con x=X/Z², y=Y/Z³.

**De dónde sale el "~8×" teórico.** El argumento clásico: en afín, cada suma y cada duplicación necesitan una **inversión modular `I`** (el denominador de la pendiente λ), y se asume el coste de libro `I ≈ 80-100 M` (multiplicaciones). En Jacobianas la inversión desaparece de cada paso y se difiere a **UNA sola al final**. Con un conteo de operaciones para una mult. escalar de 256 bits (~256 doblados + ~128 sumas), tomando `I ≈ 80M` y `S ≈ 0,8M`:

- **Afín** ≈ 384·I + ~768M + ~640S ≈ 384·80M + … ≈ **~32 000 M-equivalentes** (dominado por las ~384 inversiones).
- **Jacobiano** (doblado 4M+4S, suma 12M+4S, +1 inversión final) ≈ ~2560M + ~1536S + 1I ≈ **~3870 M-equivalentes**.
- Ratio ≈ 32000/3870 ≈ **8×**. De ahí sale el número teórico.

**Por qué en la práctica solo medimos 1,7×-2×.** La clave es que las coordenadas Jacobianas **no eliminan coste, lo intercambian**: quitan 1 inversión por paso pero **añaden multiplicaciones y cuadrados** (un doblado afín es ~2M+2S+1I; uno Jacobiano, 4M+4S sin inversión). Ese intercambio solo es muy rentable si `I` es de verdad tan cara como supone el modelo. Y **no lo es**: la rutina de inversión de **NTL/GMP está extremadamente optimizada** (Euclides extendido / GCD binario en ensamblador), de modo que el cociente real **`I/M` en este entorno está más cerca de ~10-20 que de 80-100**.

Rehaciendo el conteo con `I ≈ 14M` (el valor que reproduce nuestra medida): afín ≈ 384·14M + ~1280M ≈ **~6600 M-equiv** frente a ~3870 del Jacobiano → ratio ≈ **1,7×**. Es decir, el speedup observado es exactamente lo que predice el modelo **cuando se usa el coste real de la inversión de GMP, no el de los libros**. Por eso, además, la aceleración **crece con el tamaño del cuerpo** (1,72× en 256 bits, ~2× en P-384): al crecer el primo, la inversión se encarece más rápido que la multiplicación, sube el I/M efectivo y aumenta el ahorro de quitarlas.

> Lección de defensa: "El ~8× sale de suponer I≈80-100M. Pero Jacobianas no borran coste: intercambian 1 inversión por ~10 multiplicaciones extra por paso, y solo compensa mucho si la inversión es cara. NTL/GMP la implementan tan bien (I/M real ~10-20) que el intercambio queda en ~1,7-2×. Es uno de los hallazgos más ilustrativos del trabajo: el conteo teórico de operaciones falla cuando la librería de bajo nivel no respeta el ratio I/M supuesto."

## 3.6 F_p vs F_2^m: por qué sale más rápido el primo (~1,7×)

**No es una limitación intrínseca de los cuerpos binarios.** Es un artefacto de software:

- `Fp` usa el tipo `ZZ_p` de NTL, que delega el cálculo pesado a las rutinas en **ensamblador de GMP** (ultra-optimizadas).
- `F₂ₘ` usa `GF2E`, cuya multiplicación polinómica **no tiene ese nivel de afinado** en software genérico, y además aquí opera solo en afín.

En hardware con soporte nativo (instrucción **PCLMULQDQ** de multiplicación sin acarreo, o síntesis en FPGA), la aritmética binaria suele **ganar** por la ausencia de acarreos. Por eso la conclusión subraya que es un efecto de la implementación, no del álgebra.

## 3.7 Multiplicación escalar, a=-3 vs a=0, NAF, GLV/Frobenius

- **Multiplicación escalar `kP`:** operación dominante en ECC. Se hace con **double-and-add** (análogo a la exponenciación rápida). Por eso keygen, ECDH y firma cuestan casi lo mismo (una mult. escalar), y la **verificación ECDSA cuesta el doble** (calcula `u1·G + u2·Q`, dos mult. escalares).
- **a = -3 y a = 0:** las curvas NIST usan `a = -3` porque simplifica el doubling Jacobiano (`3X₁²+aZ₁⁴ = 3(X₁-Z₁²)(X₁+Z₁²)`); secp256k1 usa `a = 0`, que elimina un término entero (de ahí que sea ligeramente más rápida que P-256 a igual seguridad).
- **Forma NAF (Non-Adjacent Form):** representación del escalar con dígitos {-1,0,1} sin dos no nulos consecutivos. Reduce las sumas en double-and-add (densidad ~1/3 en vez de ~1/2). Mejora futura identificada.
- **GLV y Frobenius en Koblitz:** las curvas de Koblitz admiten el endomorfismo de Frobenius (eleva al cuadrado las coordenadas), que acelera kP descomponiendo el escalar (GLV / τ-NAF). No se aprovechó en la versión afín.

## 3.8 Parámetros del dominio: por qué h=1 y n≠p

Los parámetros de dominio son la séxtupla `T = (p, a, b, G, n, h)`: el cuerpo (p), la curva (a,b), el subgrupo de trabajo (G y su orden n) y la relación con el grupo total (cofactor h = #E(Fp)/n).

**¿Por qué querer h=1?**

- h=1 significa n = #E(Fp): el subgrupo generado por G abarca TODOS los puntos.
- Si h>1, existen puntos de orden pequeño (divisores de h) que permiten **invalid-curve / small-subgroup attacks**: un atacante envía un punto de orden pequeño como clave pública para deducir bits de la clave privada módulo ese orden.
- Con h=1, todo punto no trivial tiene orden n (primo); elimina ese vector y Pohlig-Hellman.

**¿Por qué n≠p (curva no anómala)?**

- Si n = #E(Fp) = p exactamente, la curva es "anómala".
- El ataque **Smart-Semaev-Satoh** resuelve el ECDLP en tiempo **lineal** O(log p) mediante un levantamiento p-ádico, sin importar el tamaño de clave.
- Por Hasse, #E(Fp) ∈ [p+1-2√p, p+1+2√p], así que n≈p siempre, pero deben ser distintos.

**Son COMPATIBLES:** h=1 y n≠p coexisten perfectamente. Todas las curvas estándar (P-256, P-384, secp256k1) satisfacen ambas.

**¿Por qué n debe ser primo?** Para que ⟨G⟩ sea cíclico de orden primo; así Pohlig-Hellman no puede descomponer el ECDLP en subgrupos pequeños.

**¿Qué validaciones implementamos?** `CurveParams::validate()` comprueba (Algoritmo 5, pasos 1-3 y 6 parcial): p primo, discriminante ≠ 0, G en la curva, n y h positivos. NO se comprueba (por redundancia con estándares auditados) la primalidad de n, la no-anomalía, el grado de inmersión ni `n·Q=O` para h>1: documentados como requisitos teóricos y limitación del prototipo.

## 3.9 Profundidad algebraica adicional (para apurar la nota)

- **j-invariante:** clasifica las curvas elípticas salvo isomorfismo sobre la clausura algebraica (dos curvas con el mismo j son isomorfas sobre K̄). En forma corta, `j = 1728·4a³/(4a³+27b²)`.
- **Twist cuadrática:** curva isomorfa a E sobre una extensión cuadrática pero no sobre el cuerpo base; comparte el mismo j. La "twist security" exige que la twist también tenga orden con factor primo grande (Curve25519 está diseñada para cumplirlo).
- **Modelos de curva:** Weierstrass (general, el que usamos); Montgomery (`B y² = x³ + A x² + x`, la de Curve25519, con escalera de Montgomery muy eficiente y resistente a canal lateral); Edwards / twisted Edwards (Ed25519, ley de suma completa y unificada, ideal para tiempo constante). Todas birracionalmente equivalentes.
- **Aritmética en Fp y F₂ₘ:** en Fp, enteros módulo p con reducción eficiente (Montgomery o Barrett) sobre GMP/ensamblador. En F₂ₘ, polinomios sobre F₂ en base polinómica, multiplicación + reducción módulo un polinomio irreducible; `GF2E` de NTL es genérico, de ahí que salga más lento.
- **Grado de inmersión (embedding degree):** el menor k tal que `n | qᵏ - 1`. Si es pequeño, los emparejamientos (Weil/Tate) trasladan el ECDLP a F_{qᵏ}* (ataque MOV). Las curvas seguras lo tienen enorme; las de emparejamiento lo tienen pequeño a propósito.

---

# PARTE 4 — Ataques (al ECDLP y a las implementaciones)

## 4.1 Ataques genéricos al ECDLP: BSGS y rho de Pollard

La memoria incluye **dos ejemplos numéricos completos con pseudocódigo** (por si el tribunal pide recorrer uno):

1. **Baby-step/Giant-step** (Shanks), O(√n) en tiempo *y* espacio (Ejemplo 5.1, Algoritmo 3): curva E: y²=x³+2x+3 sobre F₉₇, P=(0,10), n=50, Q=(92,81)=16P. Se calcula m=⌈√50⌉=8, se listan baby steps (iP para i=0..7) y giant steps hasta encontrar **d=16**.
2. **Rho de Pollard** (Ejemplo numérico, Algoritmo 4), O(√n) en tiempo / O(1) en memoria: curva E: y²=x³+2x+1 sobre F₈₉, P=(1,2), n=47, Q=(87,73)=15P. Tabla tortuga/liebre, colisión en i=13, extracción de **d=15**. Visualización en Figura 5.3. Se apoya en la **paradoja del cumpleaños** (cota √(πn/2) ≈ 1,25√n), el **paseo pseudoaleatorio de 3 conjuntos** y la **detección de ciclos de Floyd** (tortuga = t, liebre = l).

**No confundir las dos versiones del rho de Pollard:**

- **Para factorización de enteros:** busca un factor de n detectando un ciclo en `x_{i+1} = x_i² + c mod n`.
- **Para ECDLP:** dado `Q = dP`, busca el escalar `d` detectando una **colisión** en un paseo pseudoaleatorio sobre el grupo de puntos.

Comparten la idea (detección de ciclos) pero **el dominio y el objetivo son distintos**. En la memoria el rho que se estudia como ataque a ECC es la **versión del logaritmo discreto**. Es un algoritmo **genérico**: solo usa la operación de grupo, idéntico en Fp y F₂ₘ — lo que es precisamente *por qué* ECC es segura (no hay estructura de cuerpo que explotar, a diferencia del index calculus).

## 4.2 Ataques estructurales (no de fuerza bruta)

Saber estos tres distingue a un candidato a matrícula:

- **Pohlig-Hellman:** si #E factoriza en primos pequeños, descompone el ECDLP en logaritmos en subgrupos pequeños. Defensa: n primo grande.
- **MOV / Frey-Rück (por emparejamientos):** usando el pairing de Weil/Tate se traslada el ECDLP a un logaritmo discreto en una extensión `F_{qᵏ}`, donde hay algoritmos subexponenciales. Eficaz si el **grado de inmersión k es pequeño** (curvas supersingulares, k ≤ 6). Defensa: usar curvas con k grande.
- **Curvas anómalas (Smart / Satoh-Araki-Semaev):** si `#E(Fp) = p` (traza t = 1), el ECDLP se resuelve en **tiempo polinómico** vía levantamiento p-ádico. Defensa: rechazar curvas con #E = p.

Por esto solo se usan curvas estandarizadas (SEC 2, NIST): ya vienen verificadas contra MOV, anómalas y con cofactor controlado. Es un argumento de seguridad, no de comodidad.

## 4.3 Canal lateral (side-channel)

- **Timing / SPA-DPA:** un `double-and-add` ingenuo ejecuta la rama "add" solo cuando el bit del escalar es 1, y eso se ve en el tiempo y el consumo. Filtra la clave privada bit a bit.
- **Mitigación:** algoritmos en **tiempo constante** (escalera de Montgomery, add-and-double-always, accesos a memoria independientes del secreto). Nuestra implementación prioriza claridad y benchmarking, **NO es de tiempo constante**, y por tanto no es apta para producción. Decirlo es honestidad académica y suma.

## 4.4 El nonce de ECDSA: el fallo que rompe sistemas reales

- ECDSA usa un nonce `k` aleatorio por firma. Si `k` se **repite** o es **predecible**, se despeja la clave privada con álgebra elemental: dos firmas con el mismo k comparten r, de `s1, s2` se despeja `k = (e1-e2)/(s1-s2)` y de ahí `d = (s1·k - e1)/r`.
- Casos reales: la **PlayStation 3 de Sony** (k fijo, 2010) y robos de **Bitcoin** por RNG defectuoso en carteras Android.
- Mitigación moderna: **nonce determinista (RFC 6979)**, que deriva k del mensaje y la clave por HMAC-DRBG. No implementado (limitación).

## 4.5 Ataques sobre el punto de entrada y RSA "de libro"

- **Invalid-curve attack:** si no validas que el punto recibido está REALMENTE en la curva, un atacante envía un punto de una curva débil y extrae la clave. Defensa: validar siempre `y² == x³ + ax + b` (lo hace el constructor de `ECPoint`).
- **Small-subgroup attack:** relacionado con el cofactor; por eso se valida el orden del punto cuando h>1.
- **Por qué el textbook RSA es inseguro:** determinista (mismo mensaje → mismo cifrado), maleable (`(m1ᵉ)(m2ᵉ) = (m1·m2)ᵉ`), vulnerable a Coppersmith/Håstad con exponentes o mensajes pequeños sin padding. Por eso el RSA real usa **OAEP** (cifrado) y **PSS** (firma). En el TFG está identificado como trabajo futuro; decisión consciente de alcance, no olvido.

## 4.6 Amenaza cuántica (Shor) y post-cuántico

- El algoritmo de **Shor** (1994) resuelve factorización y logaritmo discreto en tiempo **polinómico** (O(log³n)) en un ordenador cuántico tolerante a fallos suficientemente grande. Rompe **a la vez RSA y ECC**.
- Detalle útil: ECC necesita **menos qubits lógicos** que RSA para una seguridad clásica equivalente (clave más pequeña), así que **bajo Shor ECC caería antes** que RSA.
- El NIST cerró en **2024** sus primeros estándares post-cuánticos: **FIPS 203 (ML-KEM, antes Kyber)** para encapsulado de clave; **FIPS 204 (ML-DSA, antes Dilithium)** y **FIPS 205 (SLH-DSA, antes SPHINCS+)** para firma; FALCON (FN-DSA) en preparación. Se basan en **retículos (lattices)** y en **hashes**, no en logaritmo discreto.
- Cierre ideal: *"este TFG mide la criptografía de clave pública que domina HOY; la migración a post-cuántico sería la continuación natural, y el marco de benchmarking que he construido es directamente reutilizable para comparar ML-KEM/ML-DSA frente a ECC"*.

> **Nota sobre el resumen del TFG:** una nota previa sugería "quitar los ataques cuánticos del resumen porque no se llega a ver". Es **incorrecto**: Shor está justificado en el abstract (ES/EN), en el Cap. 5.1 (historia) y en el Cap. 8.5 (trabajo futuro). NO hay que quitarlo.

---

# PARTE 5 — Protocolos

## 5.1 ECDH (intercambio de claves)

- **Qué garantiza:** confidencialidad de un secreto común establecido sobre canal público. Con claves efímeras (ECDHE) aporta además **forward secrecy** (secreto perfecto hacia adelante).
- **¿Autentica?** NO. Por sí solo es vulnerable a man-in-the-middle. Por eso TLS combina ECDHE con una firma/certificado (ECDSA/EdDSA).
- **¿Por qué efímero (ECDHE)?** Forward secrecy: si en el futuro se compromete una clave de largo plazo, las sesiones pasadas (con claves efímeras ya destruidas) siguen protegidas.
- **¿En qué problema se apoya?** En el problema computacional de Diffie-Hellman (CDH) sobre la curva: calcular `abG` a partir de `aG` y `bG`. A lo sumo tan difícil como el ECDLP (romper ECDLP rompe CDH; el recíproco no se conoce en general).
- **CDH vs DDH:** CDH = calcular abG. DDH (decisional) = distinguir abG de un punto aleatorio. La seguridad de esquemas como ECIES descansa sobre DDH.
- **¿Por qué solo la coordenada x del secreto S?** Por convención y compresión, y porque el resultado pasa por una KDF; ±y comparten la misma x.

## 5.2 ECDSA (firma)

**Generación:** `e = H(m)`; nonce `k` aleatorio en [1, n-1]; `R = kG = (x_R, y_R)`; `r = x_R mod n` (si 0, repetir); `s = k⁻¹(e + d·r) mod n` (si 0, repetir); firma = `(r, s)`.

**Verificación:** comprobar `r, s ∈ [1, n-1]`; `e = H(m)`; `w = s⁻¹ mod n`; `u1 = e·w mod n`, `u2 = r·w mod n`; `R' = u1·G + u2·Q`; aceptar sii `x_{R'} mod n = r`.

**Demostración de corrección:** de `s = k⁻¹(e+dr)` se obtiene `k = s⁻¹e + s⁻¹r·d = u1 + u2·d (mod n)`. Entonces `R' = u1·G + u2·Q = u1·G + u2·d·G = (u1+u2·d)·G = k·G = R`, luego `x_{R'} mod n = x_R mod n = r`. ∎

- **Por qué se firma H(m) y no m:** eficiencia y seguridad; el hash resistente a colisiones permite mensajes de cualquier tamaño y fija la entrada. Si |H(m)| > |n|, se truncan los bits más significativos.
- **Maleabilidad:** si (r,s) es válida, (r, n-s) también. Importa en Bitcoin, que fuerza "low-s" (BIP 62).
- **ECDSA vs EdDSA (Ed25519):** EdDSA usa nonce determinista por diseño, opera sobre curva de Edwards con ley de suma completa y es naturalmente de tiempo constante: más robusta ante errores de implementación. ECDSA es el estándar previo, más extendido en PKI/FIPS.
- **r=0 o s=0:** casos degenerados; se descartan y se repite con otro k.

## 5.3 ¿Por qué ECC no cifra como RSA? Cómo se compara el cifrado

Pregunta muy probable, porque rompe la simetría aparente "RSA vs ECC".

**RSA es de forma nativa un esquema de cifrado.** Es una *permutación con trampa* (trapdoor permutation): el mensaje se mete directamente en la operación, `c = mᵉ mod n`, y solo quien tiene `d` lo recupera, `m = cᵈ mod n`. Cifrar y firmar son la misma maquinaria (exponenciación modular) con las claves intercambiadas.

**ECC NO tiene una primitiva de cifrado directa análoga.** El grupo de puntos de la curva solo ofrece suma y multiplicación escalar; no existe una permutación con trampa que "meta" el mensaje en un punto y lo recupere con la clave privada. No se puede cifrar un mensaje "directamente con la clave pública ECC" como en RSA.

**Cómo se cifra realmente con ECC (esquema híbrido):**

- **ECDH** para acordar un secreto compartido sobre canal público, del que se deriva (con una KDF) una clave simétrica.
- Un **cifrado simétrico** (p. ej. AES) cifra el mensaje con esa clave.
- Opcionalmente un **MAC** para integridad.
- El paquete estandarizado de todo esto es **ECIES** (Elliptic Curve Integrated Encryption Scheme), el análogo de ElGamal sobre curvas. **ECDSA es solo firma** (autenticación/integridad/no repudio), NO cifra.

**Qué hay en este TFG:** ECIES se describe como variante teórica (Cap. 5.4.3) pero **no se implementa** (tampoco AES/MAC/HKDF). Por eso en el benchmark **no existe una operación "Encrypt" de ECC**; las operaciones ECC medidas son Keygen, mult. escalar, ECDH y ECDSA sign/verify.

**Cómo se compara entonces el "cifrado":** la operación representativa de ECC con material secreto para **establecer confidencialidad** es **ECDH**, y en la tabla comparativa (Cuadro 7.6) se enfrenta al **descifrado CRT de RSA** (la operación privada). El propio pie de tabla lo dice: *"Descifrado CRT en RSA frente a ECDH en ECC; ambas son las operaciones representativas con material secreto"*. El **cifrado público** de RSA (Encrypt con e=65537) es baratísimo (~3-38 µs) y simplemente **no tiene análogo directo** en ECC, por lo que esa casilla queda vacía en la comparación.

> Respuesta corta: "RSA cifra directamente porque es una permutación con trampa; ECC no tiene esa primitiva, cifra con un esquema híbrido (ECDH + simétrico + MAC = ECIES). En el TFG no se implementó ECIES, así que la confidencialidad se compara enfrentando la operación privada de RSA (descifrado CRT) con ECDH; el Encrypt público de RSA no tiene análogo en ECC."

---

# PARTE 6 — Metodología: por qué el benchmark es CIENCIA y no anécdota

Esta es la sección que separa un notable de una matrícula. Un tribunal exigente no pregunta "qué mediste", pregunta **"por qué debería creerme tus números"**.

## 6.1 Validez interna (el número que mido es el que creo medir)

- **Calentamiento + mediana de N iteraciones:** descarta caché fría y outliers del planificador del SO.
- **Misma máquina, mismo compilador, mismas flags, mismo contenedor Docker:** elimina la varianza de entorno. Docker es la garantía de reproducibilidad bit a bit.
- **RNG con semilla fija:** todos los algoritmos se enfrentan a los MISMOS valores de entrada (comparación pareada).
- **Aislamiento de la operación:** se mide solo la primitiva (keygen, firma, etc.), no el I/O ni el parseo de argumentos.

## 6.2 Validez externa (hasta dónde generalizan las conclusiones)

Decirlo ANTES de que lo pregunten:

- Los resultados son válidos **para esta arquitectura (x86-64), este compilador (GCC -O2) y estas bibliotecas (NTL/GMP)**. NO son una verdad universal sobre "ECC vs RSA".
- En un microcontrolador ARM sin unidad de división, o con PCLMULQDQ para campos binarios, el ranking podría cambiar. El campo binario sale lento aquí por un **artefacto de software**, no por desventaja del álgebra.
- La conclusión robusta y transferible no es "ECC es X veces más rápida", sino **"la ventaja asintótica de tamaño de clave de ECC es estructural (ECDLP exponencial vs factorización subexponencial), mientras que las ventajas concretas de velocidad dependen de la implementación"**.

## 6.3 Justicia de la comparación (el punto más atacable)

- **¿Por qué P-256 con RSA-3072 y no con RSA-2048?** Porque NIST SP 800-57 los sitúa en el MISMO nivel de seguridad (~128 bits). Comparar a igual tamaño de clave sería el error clásico; se compara a **igual seguridad**.
- **RSA con CRT vs sin CRT:** se documentan ambos para que la comparación con ECC use la versión optimizada y realista de RSA, no un hombre de paja lento.
- **Afín vs Jacobiano solo en Fp:** se reconoce explícitamente que el binario solo se midió en afín (limitación 4 del Cap. 8). No se esconde: se enuncia como sesgo conocido.

## 6.4 Preguntas probables de metodología

- **¿Mediana o media, y por qué?** Mediana: robusta frente a outliers del planificador y la caché. La media, desviación típica y P5/P95 están en los CSV.
- **¿-O2 y no -O3?** -O3 desenrolla bucles de forma impredecible (code bloat, presión de caché L1) e introduce ruido; además el cálculo pesado vive en GMP precompilado, así que las ganancias por encima de -O2 son marginales (datos: -O3 da 769µs vs 743µs de -O2 en mult. escalar).
- **¿Controlasteis turbo/throttling de CPU?** Si se fijó el governor o se ejecutó en entorno controlado, mencionarlo; si no, reconocer que es ruido residual mitigado por la mediana.
- **¿Por qué chrono y no contadores hardware (rdtsc/perf)?** `std::chrono::high_resolution_clock` es suficiente a la escala de µs de estas operaciones; para sub-microsegundo habría que ir a contadores de ciclo. Conocer el límite de la herramienta da puntos.
- **¿Cómo validasteis que la implementación es CORRECTA, no solo rápida?** Vectores de prueba conocidos (SHA-256 contra FIPS 180-4; curvas y puntos contra SEC 2; cifrar-descifrar y firmar-verificar como test de ida y vuelta).

## 6.5 Comparación con OpenSSL: leerla con cuidado

- **En RSA "ganamos" ~8-9%.** NO significa que seamos mejores: OpenSSL paga sobrecargas de producción que nosotros no tenemos → **blinding** (defensa de canal lateral por temporización), **padding** EMSA-PKCS#1 y montaje de DigestInfo, e indirección de su arquitectura **providers/EVP**. Nosotros medimos la exponenciación modular pura. Ambos en el **mismo orden de magnitud** porque usan técnicas equivalentes (reducción de Montgomery, ventanas).
- **En ECC OpenSSL gana 14×-34× en P-256.** Aquí sí hay diferencia real, pero **no es matemática**: es ensamblador escrito a mano, rutinas de tiempo constante y multiplicaciones sin acarreo específicas para esa curva. La brecha **se desploma en P-384** (1,7×-4×) porque OpenSSL la optimiza menos. Demuestra que su ventaja viene del **trabajo de bajo nivel curva a curva**, no de los fundamentos algebraicos.

---

# PARTE 7 — Estándares, contexto y contribución

## 7.1 Curvas NIST vs Curve25519/Ed25519 (Dual_EC_DRBG)

- Las curvas NIST (P-256, etc.) tienen semillas de generación **inexplicadas** que alimentan SHA-1. Tras el escándalo del generador **Dual_EC_DRBG** (sospecha de puerta trasera de la NSA en un estándar NIST, revelada por Snowden en 2013, retirado en 2014), parte de la comunidad **desconfía** de ellas.
- **Curve25519 / Ed25519** (Daniel J. Bernstein) se diseñaron con criterios "nothing-up-my-sleeve" (parámetros justificables y rígidos), para ser **rápidas y difíciles de implementar mal** (escalera de Montgomery, sin ramas dependientes del secreto). Estándar de facto en **TLS 1.3, Signal, SSH, WireGuard**.
- Opinión informada que da matrícula: *"usamos curvas estándar SEC 2 por trazabilidad y porque ya están verificadas contra MOV/anómalas; en un sistema nuevo de producción consideraría Ed25519 por su robustez frente a errores de implementación"*.

## 7.2 La contribución del TFG (meta-pregunta del tribunal)

Pregunta típica: *"esto ya está en Hankerson-Menezes-Vanstone, ¿qué aportas tú?"*.

- **No es investigación de frontera, es un TFG de ingeniería:** la aportación es una **implementación propia, modular y verificable** (RSA+CRT, ECC afín y Jacobiano sobre Fp, ECC binario, SHA-256 desde cero) y una **comparativa empírica sistemática en tres ejes** bajo un entorno reproducible.
- **El valor diferencial** es el rigor del banco de pruebas: mismo entorno, RNG controlado, Docker, y separar lo que es **álgebra** (ventaja estructural de ECC) de lo que es **artefacto de implementación** (por qué el binario sale lento, por qué el speedup Jacobiano práctico es menor que el teórico).
- **La progresión pedagógica afín → Jacobiano** documentada es en sí misma una aportación didáctica: muestra el coste real de la inversión modular y por qué existen las coordenadas proyectivas.

## 7.3 Contexto histórico (cronología)

- **1976:** Diffie-Hellman proponen criptografía de clave pública.
- **1977:** RSA (Rivest, Shamir, Adleman).
- **1985:** Miller y Koblitz proponen ECC independientemente.
- **1997:** Certicom lanza el ECC Challenge para evaluar la dificultad práctica del ECDLP.
- **1998:** ANSI X9.62 define ECDSA (primer estándar formal).
- **2000:** NIST incorpora ECC en FIPS 186-2.
- **2005:** NSA Suite B: recomienda P-256, P-384, P-521.
- **2006:** Bernstein presenta Curve25519.
- **2009:** récord práctico ECDLP: curva ECC2K-130 (130 bits, binaria) resuelta con cómputo distribuido masivo.
- **2010:** PlayStation 3 hackeada por reutilización del nonce k en ECDSA.
- **2013:** Snowden revela la sospecha de puerta trasera en Dual EC DRBG; NIST lo retira en 2014.
- **2018:** RFC 8422 → TLS 1.2 adopta secp256r1 y X25519 como preferidos; TLS 1.3 (RFC 8446) elimina los cipher suites RSA para intercambio de claves (ECDHE predeterminado).

---

# PARTE 8 — Inconsistencias y puntos débiles del documento (revisar antes de entregar)

## 8.1 Curvas binarias con h>1 (importante para el tribunal)

- sect233k1 y sect283k1 tienen **h=4**; sect163k1, sect233r1 y sect283r1 tienen h=2.
- El texto dice que con h>1 "sería necesario verificar adicionalmente que n·Q=O para garantizar la pertenencia al subgrupo correcto, lo que se documenta como limitación del prototipo" (Cap. 5.2.4).
- Respuesta honesta si preguntan por validación de claves públicas en binarias con h>1: **no completamente**, es una limitación documentada del prototipo educativo.

## 8.2 Comparación con OpenSSL: "implementación propia más rápida en RSA"

- Cuadro 7.7: la implementación propia es ~1,08-1,09× más rápida que OpenSSL en RSA. **NO** significa que sea mejor (ver Parte 6.5: blinding, padding, EVP). Para ECC, OpenSSL es 14-34× más rápida en P-256.

## 8.3 RSA de 192 bits (7680 bits) no evaluado experimentalmente

- Cap. 8.4 (limitación 3): la latencia de generar/validar módulos RSA de 7680 bits excedía los plazos razonables. Se analizó matemáticamente (Cuadro 5.3, figura 7.9) pero no se midió empíricamente.

## 8.4 Sign/Verify en campos binarios: hueco de alcance, no matemático

En `ecc_binary.cpp` están implementadas la aritmética de campo, las operaciones de punto (`binary_ec_add`, `binary_ec_double`, `binary_ec_negate`, `binary_ec_scalar_mult`), la generación de claves y `binary_ecdh_shared_secret`, pero **no ECDSA**. Por eso esas filas (figura 7.14) solo tienen Keygen, mult. escalar y ECDH.

**Justificación defendible (no es un olvido, es alcance):** el eje "primo vs binario" busca comparar la **aritmética del cuerpo** (𝔽ₚ frente a 𝔽₂ᵐ), y eso se ve con total nitidez al nivel de la **multiplicación escalar, keygen y ECDH** — las tres operaciones que dependen directamente de la aritmética del cuerpo base. ECDSA, en cambio, añadiría hashing (SHA-256) y aritmética modular en el **cuerpo escalar** (módulo n), que es **prácticamente idéntica sea cual sea el cuerpo base**: `r = x_R mod n` y `s = k⁻¹(e+dr) mod n` operan sobre enteros módulo n, no sobre el cuerpo de la curva. Así que ECDSA aportaría poco al contraste binario-vs-primo a cambio de bastante código adicional. La parte que SÍ distingue Fp de F₂ₘ — la multiplicación escalar `R = kG` — ya está medida.

> Respuesta corta: "En binarias medimos keygen, mult. escalar y ECDH porque son las que dependen de la aritmética del cuerpo, que es justo lo que compara ese eje. ECDSA añade hash y aritmética mod n (idéntica en ambos cuerpos), así que no aportaría contraste; su parte sensible al cuerpo, la mult. escalar, ya está cubierta."

## 8.5 Derivación de clave (KDF) no implementada

- En ECDH se usa directamente la coordenada X, sin KDF robusta. Cap. 6.8 (3) y Cap. 5.4.2 lo reconocen. En producción: HKDF (RFC 5869).

## 8.6 Terminología "Koblitz en Fp"

- secp256k1 se describe (p. 87) como "curva de Koblitz en Fp" por tener a=0. Pero las curvas Koblitz clásicas son sobre F₂ₘ (a ∈ {0,1}, Frobenius). En Fp, a=0 permite el endomorfismo **GLV** (Gallant-Lambert-Vanstone), distinto del Frobenius. El texto lo aclara en la práctica, pero puede confundir.

## 8.7 Inconsistencia memoria↔código: valor por defecto de `-i`

- La memoria (Cap. 6.2.2, p. 93) documenta `-i ITERS ... (default: 50)`.
- El código real (`src/main.cpp`, línea 567 y `print_usage` línea 545) usa **`default: 10`**.
- Detalle menor, pero quien ejecute `./bin/bench -h` verá "default: 10". La suite se ejecutó con `-i 100` explícito (resultados no afectados). Conviene alinear: corregir el texto del Cap. 6 (50 → 10) o el código (10 → 50).

## 8.8 Páginas de autorización y formato

- **Revisar** que la página de autorización (NIE y fecha exacta de junio de 2026) esté rellena antes de entregar.
- **Cap. 4:** al principio hay mucha separación entre párrafos; juntar para que no se vea fragmentado.
- **Cap. 7:** las figuras 7.2, 7.3, 7.6, 7.7, 7.8, 7.13, 7.14 son pequeñas (sobre todo 7.13); agrandarlas o reorganizarlas.

---

# PARTE 9 — Batería de preguntas del tribunal (respuestas rápidas)

**¿Por qué RSA verifica más rápido que ECC?** RSA usa e=65537 (17 bits, dos bits a 1): exponenciación pública trivial. ECC siempre necesita una mult. escalar completa. Asimetría estructural.

**¿El CRT lo habéis programado vosotros?** Sí, recombinación de Garner con dp, dq, qinv. ~3,5× más rápido que el descifrado directo. Sustituye una exponenciación mod n por dos mod p y mod q.

**¿Por qué los binarios salen más lentos si en teoría deberían ser rápidos?** Por software: `GF2E` genérico vs `ZZ_p` sobre ensamblador de GMP, y además solo en afín. Con PCLMULQDQ/FPGA el binario suele ganar.

**¿Qué son las coordenadas Jacobianas y por qué aceleran?** (X:Y:Z) con x=X/Z², y=Y/Z³. No requieren inversiones por paso (se difiere a una al final). De ~384 inversiones a 1. Speedup 1,7×-1,96×, más conservador que el teórico (~8×) porque GMP optimiza la inversión.

**¿Qué es el ECDLP y por qué es difícil?** Dado Q = d·P, hallar d. No hay algoritmo subexponencial para curvas generales (a diferencia del DLP en Fp* con index calculus L[1/3]). Mejor ataque genérico: rho de Pollard, O(√n) ≈ 2¹²⁸ para curvas de 256 bits.

**¿Por qué -O2 y no -O3?** -O3 puede ser más lento por code bloat (agota caché L1). El cuello de botella vive en GMP precompilado. Datos: -O3 769µs vs -O2 743µs.

**¿Implementasteis el algoritmo de Shor?** No. Requiere ordenador cuántico a escala, inaccesible. Se estudia teóricamente: rompería ECDLP en O(log³n). Por eso PQC como trabajo futuro.

**¿Por qué n≠p es un requisito de seguridad?** Si n=#E(Fp)=p (anómala), Smart-Semaev-Satoh resuelve el ECDLP en tiempo lineal vía levantamiento p-ádico.

**¿Por qué se prefiere h=1?** Con h>1 hay puntos de orden pequeño; en ECDH un atacante puede enviar uno (invalid-curve attack) y deducir bits de la clave. Con h=1 todo punto no trivial tiene orden n primo.

**¿Diferencia entre curvas NIST (P-256, P-384) y secp256k1?** NIST usan a=-3 (optimiza doubling Jacobiano) y primos pseudo-Mersenne. secp256k1 usa a=0 (elimina un término) y permite GLV; es la de Bitcoin/Ethereum. Marginalmente más rápida que P-256 a igual seguridad.

**¿Qué hace el RNG y cómo se validó?** Interfaz RNG de NTL. `-s fixed` (seed=0) → determinista. Validado con uniformidad, rachas, autocorrelación y entropía.

**¿Por qué la figura 7.9 muestra "101×" pero el texto dice "138×"?** Comparaciones distintas: "101×" = RSA-4096 vs P-384 (192 bits); "138×" = RSA-3072 vs secp256k1 Jacobiana (128 bits). Ambas correctas para su nivel.

**¿Por qué empezasteis por coordenadas afines?** Claridad y verificabilidad (un punto se comprueba con `y²=x³+ax+b`). Luego Jacobianas como optimización medible.

**¿Por qué SHA-256 desde cero si OpenSSL lo trae?** Integridad académica y evitar que dependencias externas alteren los tiempos.

**¿Por qué char(K) debe evitar 2 y 3 para la forma corta?** Completar el cuadrado exige dividir por 2 y completar el cubo por 3; en char 2 o 3 esas divisiones no existen.

**¿Es ECC siempre más rápida que RSA?** No. ECC arrasa en keygen (~138×) y firma (~2,6×); RSA gana en verificación (~38×) por e pequeño. Depende del perfil de uso.

**Demuestra que ECDSA verifica correctamente.** Ver Parte 5.2 (de s=k⁻¹(e+dr) se llega a R'=kG=R).

**¿ECC cifra como RSA? ¿Cómo se compara el cifrado?** No. RSA es una permutación con trampa y cifra directamente; ECC no tiene esa primitiva. Se cifra con esquema híbrido (ECDH + simétrico + MAC = ECIES), no implementado aquí. La confidencialidad se compara enfrentando el descifrado CRT de RSA con ECDH (Cuadro 7.6); el Encrypt público de RSA no tiene análogo en ECC. ECDSA es solo firma. Ver Parte 5.3.

**¿Por qué las curvas binarias no tienen Sign/Verify?** Porque el eje primo-vs-binario compara la aritmética del cuerpo, visible en keygen/mult. escalar/ECDH. ECDSA añade hash y aritmética mod n (idéntica sea cual sea el cuerpo base), aportando poco contraste a cambio de mucho código. Decisión de alcance, no olvido. Ver Parte 8.4.

**¿Por qué el speedup Jacobiano teórico es ~8× pero en la práctica ~2×?** El 8× sale de suponer que una inversión cuesta I≈80-100M. Pero Jacobianas no eliminan coste: intercambian 1 inversión por ~10 multiplicaciones extra por paso. Solo compensa mucho si la inversión es cara, y NTL/GMP la optimizan tanto (I/M real ~10-20) que el intercambio queda en ~1,7-2×. Ver Parte 3.5.

**Preguntas-trampa:**

- **Si no sabes algo:** no inventes. *"No lo verifiqué en este trabajo, pero el enfoque que seguiría es..."*. La honestidad vale más que un farol que se desmonta.
- **"¿Por qué C++ y no Python/Rust/Sage?"** C++17 por control de bajo nivel y porque NTL/GMP son el estándar de oro en aritmética de precisión múltiple. Sage esconde la implementación (justo lo que se quería estudiar). Rust sería alternativa moderna razonable (seguridad de memoria).
- **"¿Vuestro speedup Jacobiano (~1,8×) es mucho menor que el teórico (~8×), no es un fallo?"** No: es un HALLAZGO. El teórico asume inversión cara (I~80-100 M); NTL/GMP la implementan tan bien que el ahorro real se reduce.

---

# PARTE 10 — Logística de la defensa y mapa pregunta → diapositiva

## 10.1 Consejos de logística

- **Estructura sugerida (10-15 min):** 1 frase de motivación (Koblitz/Miller, claves pequeñas) → los 3 ejes → 2-3 gráficas clave (keygen ECC vs RSA, afín vs Jacobiano, tabla resumen 7.14) → 1 conclusión estructural (ECDLP exponencial) + 1 honesta (limitaciones: tiempo constante, OAEP/PSS, binario solo afín) → 1 frase de futuro (post-cuántico reutilizando el banco).
- **Lleva la gráfica de tamaño de clave a igual seguridad:** resume todo el TFG en un vistazo.
- **Anticipa la limitación en vez de esconderla:** "soy consciente de que no es tiempo constante" dicho por ti suma; dicho por el tribunal, resta.
- **Memoriza los 3-4 números estrella:** keygen ECC ~138×, CRT ~3,5×, Jacobiano práctico ~1,8×, ratio de tamaño de clave ~12×.
- **Cierra conectando con el futuro:** post-cuántico + reutilización del framework.

## 10.2 Mapa rápido pregunta → diapositiva de respaldo

- Asociatividad de la ley de grupo → respaldo "asociatividad" (Cayley-Bacharach / Pic⁰).
- "Demuestra que ECDSA verifica" → respaldo "corrección de la verificación ECDSA".
- "Por qué ~3,5× con CRT" → respaldo "CRT en el descifrado RSA".
- Tabla completa de tiempos → respaldo "panorama completo de tiempos".
- "Quién gana en cada operación" → respaldo "razón de rendimiento RSA/ECC".
- Elección de -O2 → diapositiva de metodología (flags de compilación).
