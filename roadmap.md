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
- [ ] Implementar `rng_analysis.cpp` para generar datasets
- [ ] Soportar múltiples semillas (fijas y aleatorias)
- [ ] Generar millones de números para análisis
- [ ] Exportar a formato CSV para análisis

#### 2.2. Análisis Estadístico
- [ ] Implementar `analyze_randomness.py` con:
  - [ ] Test de uniformidad (Chi-cuadrado)
  - [ ] Test de autocorrelación
  - [ ] Test de runs
  - [ ] Análisis de entropía
  - [ ] Distribución de bits
  - [ ] Test de cumpleaños (birthday paradox)
  - [ ] Test de poker

#### 2.3. Visualización
- [ ] Histogramas de distribución
- [ ] Gráficas de autocorrelación
- [ ] Heatmaps de bits
- [ ] Scatter plots de números consecutivos
- [ ] Gráficas de entropía

#### 2.4. Documentación
- [ ] Informe de resultados estadísticos
- [ ] Comparación con generadores estándar
- [ ] Conclusiones sobre calidad criptográfica

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

## FASE 4: Análisis Comparativo RSA vs ECC (SIGUIENTE)

### Objetivos
Comparación exhaustiva de rendimiento y seguridad.

### Tareas

#### 4.1. Benchmarks Comparativos
- [ ] Comparar generación de claves (RSA vs ECDSA)
- [ ] Comparar firma/verificación
- [ ] Comparar tamaños de clave equivalentes en seguridad:
  - RSA-1024 vs ECC-160
  - RSA-2048 vs ECC-224
  - RSA-3072 vs ECC-256
  - RSA-4096 vs ECC-384

#### 4.2. Análisis de Rendimiento
- [ ] Gráficas de tiempo vs tamaño de clave
- [ ] Speedup de ECC sobre RSA
- [ ] Consumo de memoria
- [ ] Escalabilidad

#### 4.3. Análisis de Seguridad
- [ ] Comparación de niveles de seguridad
- [ ] Tamaño de claves equivalentes
- [ ] Resistencia a ataques conocidos

#### 4.4. Análisis Práctico
- [ ] Uso en protocolos reales (TLS, SSH)
- [ ] Ventajas/desventajas de cada uno
- [ ] Recomendaciones de uso

## FASE 5: Optimizaciones Avanzadas

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
- [ ] Implementar coordenadas proyectivas (Jacobian)
- [ ] NAF (Non-Adjacent Form) para multiplicación
- [ ] Precomputación de puntos (ventana fija)
- [ ] Asumir que la los puntos son válidos para acelerar verificación

#### 5.4. Padding Schemes
- [ ] Implementar OAEP para RSA
- [ ] Implementar PSS para firmas RSA
- [ ] Hacer RSA seguro en la práctica

## FASE 6: Exportación y Visualización de Datos

### Objetivos
Facilitar análisis externo y presentación de resultados.

### Tareas

#### 6.1. Exportación de Datos
- [ ] Exportar resultados a CSV
- [ ] Exportar resultados a JSON
- [ ] Formato compatible con Excel/Sheets

#### 6.2. Visualización Avanzada
- [ ] Dashboard interactivo con Python (Dash/Streamlit)
- [ ] Gráficas con matplotlib/seaborn
- [ ] Comparaciones visuales RSA vs ECC

#### 6.3. Reportes Automáticos
- [ ] Generación automática de informes PDF
- [ ] Tablas de comparación
- [ ] Gráficas embebidas

## FASE 7: Documentación y TFG (FINAL)

### Objetivos
Preparar toda la documentación para el TFG.

### Tareas

#### 7.1. Documentación Técnica
- [ ] Manual de usuario completo
- [ ] Documentación de API
- [ ] Guía de desarrollador
- [ ] Casos de uso

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

| Fase | Estado | Completado | Tiempo Estimado |
|------|--------|-----------|-----------------|
| 1. Infraestructura Base | ✅ COMPLETADO | 100% | 1-2 semanas |
| 2. Validación RNG | ✅ COMPLETADO | 100% | 1-2 semanas |
| 3. Implementación ECC | ✅ COMPLETADO | 100% | 3-4  semanas |
| 4. Análisis Comparativo | ⏳ PENDIENTE | 0% | 2-3 semanas |
| 5. Optimizaciones | ⏳ OPCIONAL | 0% | 2-3 semanas |
| 6. Visualización | ⏳ PENDIENTE | 0% | 1-2 semanas |
| 7. Documentación TFG | ⏳ PENDIENTE | 0% | 3-4 semanas |

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