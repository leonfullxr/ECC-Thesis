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

## FASE 3: Implementación Completa de ECC (EN CURSO)

### Objetivos
Implementar ECC con las mismas características que RSA.

### Tareas

#### 3.1. Aritmética de Curvas Elípticas
- [x] Estructura de punto en curva elíptica (`ECPoint`)
- [x] Operaciones de punto (suma, doblado, negación)
- [x] Multiplicación escalar (algoritmo double-and-add)
- [x] Curvas estándar: secp256k1, P-256, P-384
- [x] Validación de parámetros de curva
- [x] Verificación de puntos en curva

#### 3.2. Generación de Claves ECC
- [x] Generar clave privada (escalar aleatorio en [1, n-1])
- [x] Calcular clave pública (Q = d*G)
- [x] Validación de puntos en curva
- [x] Reproducibilidad con RNG

#### 3.3. ECDSA (Firma Digital)
- [ ] Implementar firma ECDSA
- [ ] Implementar verificación ECDSA
- [ ] Hash de mensajes (SHA-256 o similar)

#### 3.4. ECDH (Intercambio de Claves)
- [ ] Implementar ECDH
- [ ] Benchmark de acuerdo de claves

#### 3.5. Benchmarking ECC
- [ ] Benchmark de generación de claves
- [ ] Benchmark de firma ECDSA
- [ ] Benchmark de verificación ECDSA
- [ ] Benchmark de ECDH

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
  - RSA-15360 vs ECC-512

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

## FASE 5: Optimizaciones Avanzadas (OPCIONAL)

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
- [ ] Implementar proyective coordinates
- [ ] NAF (Non-Adjacent Form) para multiplicación
- [ ] Precomputación de puntos

#### 5.4. Padding Schemes
- [ ] Implementar OAEP para RSA
- [ ] Implementar PSS para firmas RSA
- [ ] Hacer RSA seguro en la práctica

## FASE 6: Exportación y Visualización de Datos (OPCIONAL)

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
| 1. Infraestructura Base | ✅ COMPLETADO | 100% | - |
| 2. Validación RNG | ✅ COMPLETADO | 100% | 1-2 semanas |
| 3. Implementación ECC | 🔄 EN CURSO | 0% | 2-3 semanas |
| 4. Análisis Comparativo | ⏳ PENDIENTE | 0% | 2-3 semanas |
| 5. Optimizaciones | ⏳ OPCIONAL | 0% | 2-3 semanas |
| 6. Visualización | ⏳ OPCIONAL | 0% | 1-2 semanas |
| 7. Documentación TFG | ⏳ PENDIENTE | 0% | 3-4 semanas |

## RECURSOS ÚTILES

### Para RNG
- NIST Statistical Test Suite
- Diehard tests
- TestU01 suite

### Para ECC
- Standards for Efficient Cryptography (SEC)
- NIST FIPS 186-4
- Guía de Bernstein et al. sobre curvas seguras

### Para Benchmarking
- Google Benchmark
- Catch2 (para tests)
- Matplotlib/Seaborn (visualización)