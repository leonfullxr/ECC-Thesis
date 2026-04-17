# ROADMAP - Proyecto RSA vs ECC Benchmark
 
## FASE 1: Infraestructura Base (COMPLETADO)
 
### Logros
- [x] Sistema RNG reproducible con semillas
- [x] Implementación completa de RSA
- [x] Optimización CRT (~4x speedup)
- [x] Sistema de benchmarking con estadísticas
- [x] Arquitectura modular sin duplicación
- [x] Documentación exhaustiva
- [x] Corrección de bugs (PowerMod, random_prime)
- [x] Normalización en C++ con NTL::RR
  - [x] Realizar toda la generación y normalización en C++, volcar datos ya normalizados, y Python lee los datos limpios
- [x] Scripts de automatización para análisis RNG
- [x] Análisis estadístico básico (histogramas, autocorrelación, entropía)
## FASE 2: Validación Estadística del RNG (COMPLETADO)
 
### Objetivos
Verificar la calidad criptográfica del generador de números aleatorios.
 
### Tareas
 
#### 2.1. Programa de Generación de Datos
- [x] Implementar `rng_analysis.cpp` para generar datasets
- [x] Soportar múltiples semillas (fijas y aleatorias)
- [x] Generar millones de números para análisis
- [x] Exportar a formato CSV para análisis
#### 2.2. Análisis Estadístico
- [x] Implementar `analyze_randomness.py` con:
  - [x] Test de uniformidad (Chi-cuadrado)
  - [x] Test de autocorrelación
  - [x] Test de runs
  - [x] Análisis de entropía
  - [x] Distribución de bits
  - [x] Test de cumpleaños (birthday paradox)
  - [x] Test de poker
#### 2.3. Visualización
- [x] Histogramas de distribución
- [x] Gráficas de autocorrelación
- [x] Heatmaps de bits
- [x] Scatter plots de números consecutivos
- [x] Gráficas de entropía
#### 2.4. Documentación
- [x] Informe de resultados estadísticos
- [x] Comparación con generadores estándar
- [x] Conclusiones sobre calidad criptográfica
## FASE 3: Implementación Completa de ECC (COMPLETADO)
 
### Objetivos
Implementar ECC con las mismas características que RSA.
 
### Tareas
 
#### 3.1. Aritmética de Curvas Elípticas
- [x] Estructura de punto en curva elíptica (`ECPoint`)
- [x] Operaciones de punto (suma, doblado, negación)
- [x] Multiplicación escalar (double-and-add, O(log k))
- [x] Verificación de punto en curva (`is_on_curve`)
- [x] Selección de curvas estándar:
  - [x] secp256k1 (Bitcoin/Ethereum)
  - [x] NIST P-256 (secp256r1)
  - [x] NIST P-384
- [x] Validación de parámetros de curva (discriminante, generador)
#### 3.2. Generación de Claves ECC
- [x] Generar clave privada (escalar aleatorio en [1, n-1])
- [x] Calcular clave pública (Q = d*G)
- [x] Validación de puntos en curva
- [x] Estructura `ECKeyPair` con impresión segura
#### 3.3. ECDSA (Firma Digital)
- [x] Implementación de SHA-256 desde cero (FIPS PUB 180-4)
  - [x] Constantes de ronda (64 constantes K)
  - [x] Valores iniciales del hash (H0)
  - [x] Operaciones lógicas (Ch, Maj, Σ0, Σ1, σ0, σ1)
  - [x] Preprocesamiento (padding)
  - [x] Procesamiento de bloques (64 rondas de compresión)
  - [x] Verificación contra vectores de test NIST
- [x] Implementar firma ECDSA (FIPS 186-4, Sección 6.4)
  - [x] Estructura `ECDSASignature` (r, s)
  - [x] Firma de mensaje string (`ecdsa_sign`)
  - [x] Firma de hash precalculado (`ecdsa_sign_hash`)
  - [x] Truncamiento de hash según tamaño del orden
  - [x] Selección de k aleatorio con reintentos
- [x] Implementar verificación ECDSA
  - [x] Verificación de formato de firma
  - [x] Verificación de clave pública
  - [x] Verificación con mensaje (`ecdsa_verify`)
  - [x] Verificación con hash (`ecdsa_verify_hash`)
- [x] Tests exhaustivos:
  - [x] Firma válida en las 3 curvas
  - [x] Rechazo de mensaje incorrecto
  - [x] Rechazo de clave incorrecta
  - [x] Rechazo de firma alterada
  - [x] Firmas múltiples diferentes pero válidas
#### 3.4. ECDH (Intercambio de Claves)
- [x] Implementar ECDH (`ecdh_shared_secret`)
- [x] Derivación de clave (`ecdh_derive_key`)
- [x] Benchmark de acuerdo de claves
#### 3.5. Benchmarking ECC
- [x] Benchmark de operaciones de punto:
  - [x] Suma de puntos
  - [x] Duplicación de punto
  - [x] Multiplicación escalar
- [x] Benchmark de generación de claves
- [x] Benchmark de firma ECDSA
- [x] Benchmark de verificación ECDSA
- [x] Benchmark de ECDH (secreto compartido + derivación)
- [x] Benchmark de SHA-256
- [x] Estadísticas mejoradas: media, mediana, min, max, desv. estándar
#### 3.6. Benchmarking RSA (Mejoras)
- [x] Benchmark de firma RSA (con CRT)
- [x] Benchmark de verificación RSA
- [x] Modo comparación (`-a CMP`): RSA vs ECC al mismo nivel de seguridad
#### 3.7. Utilidades ECC
- [x] Mapeo de seguridad ECC → RSA (NIST SP 800-57)
- [x] Recomendación de curva según tamaño RSA
- [x] Conversión de tipos de curva a string
### Archivos Nuevos/Modificados en Fase 3
| Archivo | Estado | Descripción |
|---------|--------|-------------|
| `sha256.hpp` | **NUEVO** | Header de SHA-256 (FIPS PUB 180-4) |
| `sha256.cpp` | **NUEVO** | Implementación de SHA-256 |
| `ecc.hpp` | **MODIFICADO** | +ECDSA: ECDSASignature, sign, verify, truncate_hash |
| `ecc.cpp` | **MODIFICADO** | +ECDSA: implementación completa |
| `main.cpp` | **MODIFICADO** | +Benchmarks ECDSA, RSA sign/verify, modo CMP |
| `roadmap.md` | **MODIFICADO** | Actualización de progreso |
 
## FASE 4: Análisis Comparativo RSA vs ECC (COMPLETADO)
 
### Objetivos
Comparación exhaustiva de rendimiento y seguridad en tres dimensiones.
 
### Tareas
 
#### 4.1. Benchmarks Comparativos
- [x] Comparar generación de claves (RSA vs ECDSA)
- [x] Comparar firma/verificación
- [x] Comparar tamaños de clave equivalentes en seguridad:
  - RSA-1024 vs ECC-160 (sect163k1)
  - RSA-2048 vs ECC-224/233 (sect233k1/r1)
  - RSA-3072 vs ECC-256 (P-256, secp256k1, sect283k1)
  - RSA-4096 vs ECC-384 (P-384)
#### 4.2. Análisis de Rendimiento
- [x] Gráficas de tiempo vs tamaño de clave (chart_rsa_scaling)
- [x] Speedup de ECC sobre RSA (chart_speedup_ratios)
- [ ] Consumo de memoria (no significativo: KBs para ambos)
- [x] Escalabilidad (RSA super-polynomial vs ECC linear scaling)
#### 4.3. Análisis de Seguridad
- [x] Comparación de niveles de seguridad (NIST SP 800-57 mapping)
- [x] Tamaño de claves equivalentes (tabla en README)
- [ ] Resistencia a ataques conocidos (contenido teórico → memoria TFG)
#### 4.4. Análisis Práctico
- [ ] Uso en protocolos reales (TLS, SSH) (contenido teórico → memoria TFG)
- [x] Ventajas/desventajas de cada uno (documentado en README y optimization_analysis.md)
- [ ] Recomendaciones de uso (contenido teórico → memoria TFG)
### Archivos Nuevos/Modificados en Fase 4
| Archivo | Estado | Descripción |
|---------|--------|-------------|
| `main.cpp` | **MODIFICADO** | 5 modos (-a RSA/ECC/ECCJ/BIN/CMP), ~72 benchmarks |
| `visualize_benchmarks.py` | **MODIFICADO** | 11 gráficos (4 nuevos para Jacobian + binario) |
| `create_collages.py` | **MODIFICADO** | 3 collages (nuevo: coordenadas y campos) |
| `run_benchmarks.sh` | **MODIFICADO** | +ecc_binary.cpp, verificación 4 algoritmos |
| `README.md` | **MODIFICADO** | Sección 6 completa con 3 dimensiones |
| `optimization_analysis.md` | **NUEVO** | Análisis teórico afín vs Jacobiano vs binario |
 
## FASE 5: Optimizaciones Avanzadas (PARCIALMENTE COMPLETADO)
 
### Objetivos
Mejorar rendimiento y añadir características avanzadas.
 
### Tareas
 
#### 5.1. Paralelización
- [ ] Implementar modo paralelo (`-m par`)
- [ ] Usar OpenMP o threads
- [ ] Benchmark paralelo vs secuencial
- [ ] Análisis de speedup
#### 5.2. Optimizaciones RSA
- [ ] Implementar sliding window para exponenciación
- [ ] Optimizar generación de primos
- [ ] Implementar Montgomery multiplication
#### 5.3. Optimizaciones ECC
- [x] Implementar coordenadas Jacobianas (`JacobianPoint`, `jacobian_add`, `jacobian_double`)
- [x] Multiplicación escalar Jacobiana (`ec_scalar_mult_jacobian`)
- [x] Parámetro `use_jacobian` en keygen, ECDSA, ECDH
- [x] Benchmark comparativo afín vs Jacobiano (chart_affine_vs_jacobian)
- [x] Análisis de speedup teórico vs medido (optimization_analysis.md)
- [ ] NAF (Non-Adjacent Form) para multiplicación
- [ ] Precomputación de puntos (ventana fija)
#### 5.4. Curvas sobre Campos Binarios GF(2^m)
- [x] Aritmética de campos binarios (NTL GF2E)
- [x] Conversiones hex → GF2X → GF2E
- [x] Ecuación y^2 + xy = x^3 + ax^2 + b
- [x] Operaciones: suma, doblado, negación, multiplicación escalar
- [x] 5 curvas estándar SEC 2 (sect163k1, sect233k1, sect283k1, sect233r1, sect283r1)
- [x] Generación de claves y ECDH en campos binarios
- [x] Benchmark y visualización (chart_binary_curves, chart_prime_vs_binary)
#### 5.5. Padding Schemes
- [ ] Implementar OAEP para RSA
- [ ] Implementar PSS para firmas RSA
- [ ] Hacer RSA seguro en la práctica
## FASE 6: Exportación y Visualización de Datos (COMPLETADO)
 
### Objetivos
Facilitar análisis externo y presentación de resultados.
 
### Tareas
 
#### 6.1. Exportación de Datos
- [x] Exportar resultados a CSV (summary + raw per-iteration)
- [ ] Exportar resultados a JSON (opcional, no necesario)
- [x] Formato compatible con Excel/Sheets (CSV estándar)
#### 6.2. Visualización Avanzada
- [ ] Dashboard interactivo con Python (Dash/Streamlit) (opcional)
- [x] Gráficas con matplotlib (11 tipos de gráfico)
- [x] Comparaciones visuales RSA vs ECC (collage_comparison)
- [x] Comparación afín vs Jacobiano (chart_affine_vs_jacobian, chart_jacobian_speedup)
- [x] Comparación campo primo vs binario (chart_prime_vs_binary)
- [x] 3 collages (overview, detailed, coordinates_fields)
#### 6.3. Reportes Automáticos
- [ ] Generación automática de informes PDF (opcional)
- [x] Tablas de comparación (chart_summary_table heatmap)
- [x] Gráficas embebidas en README
## FASE 7: Documentación y TFG (EN PROGRESO)
 
### Objetivos
Preparar toda la documentación para el TFG.
 
### Tareas
 
#### 7.1. Documentación Técnica
- [x] README.md completo con 3 dimensiones de comparación
- [x] optimization_analysis.md (análisis teórico)
- [ ] Manual de usuario completo
- [ ] Documentación de API
- [ ] Guía de desarrollador
#### 7.2. Memoria del TFG
- [ ] Introducción y motivación
- [ ] Estado del arte
- [ ] Diseño e implementación
- [ ] Resultados experimentales
- [ ] Análisis de resultados
- [ ] Conclusiones y trabajo futuro
- [ ] Bibliografía
#### 7.3. Presentación
- [ ] Slides de presentación
- [ ] Demo en vivo
- [ ] Preparación de preguntas

## MÉTRICAS DE PROGRESO

| Fase | Estado | Completado | Notas |
|------|--------|-----------|-------|
| 1. Infraestructura Base | COMPLETADO | 100% | RSA + RNG + Docker |
| 2. Validación RNG | COMPLETADO | 100% | Tests estadísticos completos |
| 3. Implementación ECC | COMPLETADO | 100% | Afín, ECDSA, ECDH, SHA-256 |
| 4. Análisis Comparativo | COMPLETADO | 90% | 3 dimensiones; falta contenido teórico |
| 5. Optimizaciones | PARCIAL | 60% | Jacobian + binario hechos; faltan NAF, OAEP |
| 6. Visualización | COMPLETADO | 90% | 11 gráficos + 3 collages; dashboard opcional |
| 7. Documentación TFG | EN PROGRESO | 30% | README listo; memoria LaTeX pendiente |

## RECURSOS ÚTILES

### Para SHA-256
- NIST FIPS PUB 180-4: Secure Hash Standard
- https://csrc.nist.gov/publications/detail/fips/180/4/final
- Test vectors: "abc" → ba7816bf...

### Para ECDSA
- FIPS 186-4: Digital Signature Standard
- SEC 2: Recommended Elliptic Curve Domain Parameters
- https://www.secg.org/sec2-v2.pdf

### Para ECC
- Standards for Efficient Cryptography (SEC)
- NIST FIPS 186-4
- Guía de Bernstein et al. sobre curvas seguras

### Para Benchmarking
- Google Benchmark
- Catch2 (para tests)
- Matplotlib/Seaborn (visualización)