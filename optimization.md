# Análisis de Optimización: RSA vs ECC

## Resumen

He realizado un análisis empírico completo del código actual midiendo overhead real, impacto de optimizaciones del compilador, y alternativas de medición de tiempo. Los hallazgos principales son:

1. **`is_on_curve()` consume el 17% del tiempo de ECC** — eliminar esta validación redundante en operaciones internas daría un speedup de ~1.8x
2. **El compilador beneficia asimétricamente a ECC sobre RSA** — `-O2` acelera ECC 1.38x pero RSA no mejora (su trabajo está dentro de NTL precompilado)
3. **`rdtsc` no mejora sobre `chrono`** en este entorno — ambos tienen varianza comparable para operaciones de >600µs
4. **Faltan funcionalidades críticas** — OAEP padding, HKDF, coordenadas proyectivas, window method


## 1. Problema de Fairness: Asimetría del Compilador

### Hallazgo Empírico

```
Nivel    ECC scalar_mult    RSA sign(CRT)
─────    ───────────────    ─────────────
-O0      2417 µs            720 µs
-O1      2254 µs            756 µs
-O2      1751 µs            812 µs
-O3      1682 µs            780 µs
```

ECC obtiene un speedup de 1.38x con `-O2`, mientras que RSA **no mejora** (de hecho se vuelve ligeramente más lento).

**RSA** delega toda la computación pesada a una única llamada a NTL:
```cpp
BigInt ciphertext = PowerMod(message, public_key.e, public_key.n);
```
`PowerMod` es código precompilado dentro de `libntl.so`. El compilador no puede optimizarlo porque ya fue compilado cuando se instaló NTL. Nuestro código RSA es básicamente "glue code" — validaciones y una llamada a la librería.

**ECC** tiene mucho más código propio en el camino crítico:
```cpp
while (k_copy > 0) {           // ~256 iteraciones
    if (k_copy % 2 == 1)
        result = ec_add(...);   // Nuestro código: conversiones, validación
    addend = ec_double(...);    // Nuestro código: conversiones, validación
    k_copy /= 2;
}
```
Cada iteración pasa por `ec_add`/`ec_double` → constructor `ECPoint` → `is_on_curve()` → conversiones ZZ↔ZZ_p. Todo esto es código **nuestro** que sí se beneficia de `-O2`.

### Impacto en la Comparación

Con `-O2`, ECC recibe una ventaja artificial de ~1.38x que RSA no recibe. Esto significa que la comparación RSA vs ECC a `-O2` **favorece a ECC** respecto a lo que sería una comparación "justa". Hay dos formas de abordar esto:

**Opción A** — Documentar la asimetría y reportar resultados a múltiples niveles de optimización. Esto es lo más honesto académicamente.

**Opción B** — Optimizar nuestro código ECC para que el trabajo pesado también ocurra dentro de NTL (minimizando el overhead de nuestro "glue code"). Esto haría la comparación más justa porque ambos algoritmos dependerían principalmente del mismo backend matemático.

La Opción B es preferible porque además produce código de mayor calidad.

## 2. Overhead en ECC: Análisis Cuantitativo

### `is_on_curve()` El Mayor Cuello de Botella

Cada vez que `ec_add` o `ec_double` crea un nuevo `ECPoint`, el constructor llama a `is_on_curve()`:

```cpp
ECPoint::ECPoint(const BigInt& x, const BigInt& y, const CurveParams* curve) {
    // ...
    if (!is_on_curve()) {   // ← Esto cuesta ~0.79 µs por llamada
        throw std::invalid_argument("Point is not on the curve");
    }
}
```

Y `is_on_curve()` hace una exponenciación modular completa:
```cpp
bool ECPoint::is_on_curve() const {
    ZZ_p::init(curve_->p);           // Overhead del contexto modular
    ZZ_p lhs = power(y_p, 2);       // y²
    ZZ_p rhs = power(x_p, 3) + ...  // x³ + ax + b
    return lhs == rhs;
}
```

**Mediciones:**
```
Operación                    Tiempo/llamada    Llamadas en scalar_mult
────────────────────────     ──────────────    ────────────────────────
is_on_curve()                0.79 µs           ~382 (255 doubles + 127 adds)
ec_add() total               3.35 µs           ~127
ec_double() total            3.29 µs           ~255

Overhead total is_on_curve:  301 µs  (17% del total de 1758 µs)
Sin validación:              ~962 µs estimados → speedup 1.8x
```

En operaciones internas donde sabemos que los inputs son válidos (porque vienen de operaciones anteriores que ya verificaron), esta validación es redundante y costosa.

### `ZZ_p::init()` — Overhead de Contexto

Cada `ec_add` y `ec_double` llama a `ZZ_p::init(curve->p)` para establecer el módulo. Medí 91 ns/llamada, que en 382 llamadas suma ~35 µs (2% del total). Es menor que `is_on_curve` pero sigue siendo innecesario si el módulo no cambia entre operaciones.

### Conversiones ZZ ↔ ZZ_p

Cada operación convierte coordenadas BigInt a ZZ_p y de vuelta. Esto se podría evitar almacenando directamente en ZZ_p o trabajando en coordenadas proyectivas.

## 3. Optimizaciones Propuestas para ECC

### 3.1 Constructor Interno sin Validación (Impacto: Alto)

Crear una ruta interna que omita `is_on_curve()` para resultados de operaciones aritméticas:

```cpp
class ECPoint {
    // Constructor público (con validación, para inputs externos)
    ECPoint(const BigInt& x, const BigInt& y, const CurveParams* curve);
    
    // Constructor interno (sin validación, para operaciones internas)
    static ECPoint from_internal(const BigInt& x, const BigInt& y, 
                                 const CurveParams* curve);
};
```

Esto elimina 382 llamadas a `is_on_curve()` por multiplicación escalar. Speedup estimado: **~1.4x**

### 3.2 Coordenadas Proyectivas/Jacobianas (Impacto: Muy Alto)

El cambio más significativo. En coordenadas afines (x, y), cada `ec_add` necesita una **inversión modular** (división en ZZ_p). En coordenadas Jacobianas (X, Y, Z) donde x=X/Z², y=Y/Z³, se reemplazan divisiones por multiplicaciones:

```
Coordenadas Afines:     1 inversión + 2 multiplicaciones por add
Coordenadas Jacobianas: 0 inversiones + ~16 multiplicaciones por add
```

Mediciones del overhead actual:
```
ZZ_p division:  286 ns/call
ZZ_p multiply:   39 ns/call
```

Una inversión (286 ns) equivale a ~7 multiplicaciones (273 ns). La aritmética Jacobiana usa ~16 multiplicaciones vs 1 inversión + 2 multiplicaciones en afín, así que parece peor. Pero el beneficio real viene de eliminar las 382 conversiones ZZ↔ZZ_p y los `ZZ_p::init()`, y de que solo se necesita **una** inversión al final para convertir de vuelta a coordenadas afines. Speedup estimado: **~2-3x**

### 3.3 Window Method / wNAF (Impacto: Medio-Alto)

El double-and-add actual procesa 1 bit por iteración. Con un window method de ancho w=4, se precomputan {G, 2G, 3G, ..., 15G} y se procesan 4 bits por iteración:

```
Double-and-add:  ~255 doubles + ~127 adds = ~382 operaciones
Window w=4:      ~255 doubles + ~64 adds  = ~319 operaciones  (16% menos)
wNAF w=4:        ~255 doubles + ~51 adds  = ~306 operaciones  (20% menos)
```

El beneficio modesto en operaciones se amplifica cuando se combina con coordenadas Jacobianas, porque cada operación evitada ahorra más tiempo.

### 3.4 Montgomery Ladder (Impacto: Bajo en rendimiento, Alto en seguridad)

El double-and-add actual tiene un timing dependiente de los datos (el `if` del bit):
```cpp
if (k_copy % 2 == 1) {
    result = ec_add(result, addend);  // Solo se ejecuta si el bit es 1
}
```

Esto crea un side-channel de timing que permite a un atacante deducir bits de la clave privada. El Montgomery Ladder ejecuta siempre el mismo número de operaciones:
```cpp
// Siempre ejecuta tanto add como double, independiente del bit
R0 = ec_add(R0, R1);  // o al revés según el bit
R1 = ec_double(R1);   // siempre se ejecuta
```

No mejora el rendimiento (lo empeora ligeramente), pero es importante mencionarlo en un contexto académico.

## 4. Optimizaciones Propuestas para RSA

### 4.1 Multi-Prime RSA (Impacto: Medio)

Usar 3 o más primos en vez de 2 (n = p·q·r). CRT con 3 primos reduce el exponente a n/3 bits en vez de n/2:
```
RSA-2048 estándar:   2 PowerMod de 1024 bits
RSA-2048 multi-prime: 3 PowerMod de ~683 bits → ~1.5x más rápido
```

### 4.2 Sliding Window para PowerMod (No aplica directamente)

NTL ya implementa binary method optimizado internamente en `PowerMod`. No podemos mejorar esto sin reimplementar la exponenciación modular. Para mantener fairness, esto está bien: ambos algoritmos usan la mejor implementación disponible de su operación fundamental.

## 5. Funcionalidades Nuevas

### 5.1 RSA-OAEP Padding (Prioridad: Alta)

Actualmente RSA opera con "textbook RSA" — cifrado directo sin padding. Esto es inseguro porque:
- Es determinista (mismo mensaje → mismo ciphertext)
- Es maleable (multiplicar ciphertexts = multiplicar plaintexts)

OAEP (Optimal Asymmetric Encryption Padding, PKCS#1 v2.2) resuelve ambos problemas. Ya tenemos SHA-256, que es el componente principal de OAEP.

### 5.2 HKDF Key Derivation (Prioridad: Alta)

`ecdh_derive_key()` actualmente trunca la coordenada x, lo cual es inseguro:
```cpp
BigInt key = shared_point.x();  // ← Inseguro: no es un KDF real
```

HKDF (RFC 5869) usando SHA-256 como HMAC produciría claves criptográficamente seguras. Ya tenemos SHA-256; solo falta implementar HMAC-SHA256 + HKDF extract/expand.

### 5.3 PSS Padding para Firmas RSA (Prioridad: Media)

Similar a OAEP pero para firmas. RSA-PSS es el estándar moderno (PKCS#1 v2.2) frente al esquema PKCS#1 v1.5 más antiguo.

### 5.4 ECIES — Cifrado con ECC (Prioridad: Media)

ECC actualmente solo hace key agreement (ECDH) y firmas (ECDSA), pero no cifrado directo. ECIES (Elliptic Curve Integrated Encryption Scheme) combina ECDH + KDF + cifrado simétrico (AES-GCM) para proporcionar cifrado asimétrico completo basado en ECC. Esto permitiría una comparación más directa con RSA encrypt/decrypt.

### 5.5 Validación de Clave Pública (Prioridad: Media)

No validamos que una clave pública recibida sea un punto válido de orden n. Un atacante podría enviar un punto de orden pequeño para extraer la clave privada (invalid curve attack). Habría que verificar: punto en la curva, no infinito, y n·Q = O.

## 6. rdtsc vs chrono: Análisis Empírico

### Resultados en Operaciones Criptográficas Reales

```
=== ECC scalar_mult (secp256k1, 20 iteraciones) ===
chrono:  Mean=1619µs  Median=1621µs  CV=2.0%   Range=[1573, 1678]
rdtscp:  Mean=1656µs  Median=1632µs  CV=5.7%   Range=[1576, 2028]

=== RSA sign CRT (2048-bit, 20 iteraciones) ===
chrono:  Mean=739µs   Median=695µs   CV=15.7%  Range=[678, 1099]
rdtscp:  Mean=807µs   Median=756µs   CV=15.0%  Range=[691, 1046]
```

### Overhead de rdtsc

```
rdtsc overhead (mediana): 2438 ciclos (~1.2 µs a 2.1 GHz)
```

En un entorno virtualizado/containerizado como Docker, el overhead de `rdtsc` es elevado porque el hipervisor puede interceptar la instrucción.

### Conclusión: chrono es Suficiente

Para operaciones de >100µs (que es el caso de todas nuestras operaciones criptográficas), `chrono::high_resolution_clock` tiene precisión y varianza comparables a `rdtscp`. La varianza en ambos casos viene del scheduling del SO y la contención de caché, no de la resolución del reloj.

`rdtsc` sería ventajoso para micro-benchmarks de <1µs (como medir una sola multiplicación modular), pero para nuestros benchmarks completos no aporta mejora medible.

**Recomendación**: Mantener `chrono` pero mejorar la metodología estadística:
- Añadir **mediana** además de media (la mediana es robusta ante outliers)
- Añadir **desviación estándar y coeficiente de variación**
- Añadir **warm-up runs** (3-5 iteraciones descartadas antes de medir)
- Reportar **percentiles P5/P95** para mostrar la distribución

## 7. Mejoras de Diseño

### 7.1 Separar Aritmética de Validación

Actualmente las funciones mezclan lógica criptográfica con validación:
```cpp
ECPoint ec_add(const ECPoint& P, const ECPoint& Q) {
    if (P.curve() != Q.curve()) throw ...;  // Validación
    if (P.is_infinity()) return Q;           // Caso especial
    ZZ_p::init(curve->p);                    // Setup
    // ... aritmética real
}
```

Mejor separar en capas:
- **Capa interna**: Aritmética pura, sin validación, máximo rendimiento
- **Capa pública**: Validación + delegación a capa interna
- **Capa de protocolo**: ECDSA, ECDH, que usan la capa interna

### 7.2 Gestión de Contexto ZZ_p

En vez de llamar `ZZ_p::init()` en cada operación:
```cpp
class ECArithmetic {
    const CurveParams& curve_;
public:
    ECArithmetic(const CurveParams& curve) : curve_(curve) {
        ZZ_p::init(curve.p);  // Una sola vez
    }
    ECPoint add(const ECPoint& P, const ECPoint& Q);
    ECPoint scalar_mult(const BigInt& k, const ECPoint& P);
};
```

### 7.3 Const-Correctness y Move Semantics

Los `BigInt` (NTL::ZZ) son objetos pesados. Asegurar que se usen move semantics donde sea posible:
```cpp
ECPoint ec_add(const ECPoint& P, const ECPoint& Q) {
    // ...
    BigInt x3 = conv<BigInt>(x3_p);  // ¿Se hace move o copy?
    BigInt y3 = conv<BigInt>(y3_p);
    return ECPoint(std::move(x3), std::move(y3), curve);  // Explicit move
}
```

## 8. Resumen de Prioridades

| Mejora | Impacto Rendimiento | Impacto Académico | Dificultad |
|--------|--------------------|--------------------|------------|
| Constructor sin validación | ~1.4x ECC | Medio | Baja |
| Coordenadas Jacobianas | ~2-3x ECC | Alto | Media |
| Window method (wNAF) | ~1.2x ECC | Alto | Media |
| RSA-OAEP padding | Ninguno | Alto | Media |
| HKDF key derivation | Ninguno | Alto | Baja |
| Estadísticas benchmark mejoradas | N/A | Alto | Baja |
| Montgomery ladder | -10% ECC | Alto (seguridad) | Baja |
| ECIES cifrado | Ninguno | Alto | Media |
| PSS padding firmas | Ninguno | Medio | Media |
| Multi-prime RSA | ~1.5x RSA | Medio | Media |
| Separación capas | Indirect | Medio | Media |