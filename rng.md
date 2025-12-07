# Análisis de Generadores de Números Aleatorios (RNG)

## Introducción

Verificar que el generador de números aleatorios (RNG) basado en NTL produce números de calidad criptográfica mediante un análisis estadístico exhaustivo.

## Tests Implementados

### 1. Test de Media y Varianza
**Qué verifica:** Que la media y varianza observadas coincidan con los valores teóricos de una distribución uniforme.

**Teoría:**
- Para uniforme [0, n]: media = n/2
- Varianza = n²/12

**Interpretación:**
- PASS: La media y varianza son estadísticamente consistentes con uniforme
- FAIL: Hay sesgo en los valores generados


### 2. Test de Uniformidad (Chi-cuadrado)
**Qué verifica:** Que los valores se distribuyan uniformemente en bins.

**Método:**
- Divide el rango en bins
- Compara frecuencias observadas vs esperadas
- Usa test chi-cuadrado

**Interpretación:**
- p > 0.05: Distribución uniforme
- p < 0.05: Distribución no uniforme

### 3. Test de Kolmogorov-Smirnov
**Qué verifica:** Compara la CDF empírica con la CDF teórica uniforme.

**Ventaja:** Más sensible que chi-cuadrado a desviaciones en las colas.

**Interpretación:**
- p > 0.05: Los datos siguen distribución uniforme
- p < 0.05: Desviación significativa de uniforme

### 4. Test de Runs (Rachas)
**Qué verifica:** Detecta patrones en la secuencia.

**Método:**
- Convierte a binario (> mediana = 1, < mediana = 0)
- Cuenta el número de "rachas" (secuencias consecutivas de 0s o 1s)
- Compara con el número esperado

**Interpretación:**
- Muy pocas rachas → números agrupados (no aleatorios)
- Demasiadas rachas → alternancia artificial
- PASS: Número de rachas dentro de lo esperado

### 5. Test de Autocorrelación
**Qué verifica:** Que valores consecutivos sean independientes.

**Método:**
- Calcula correlación entre valores separados por lag
- Verifica que estén dentro del límite de confianza

**Interpretación:**
- Alta autocorrelación → valores predecibles
- PASS: Autocorrelación negligible en todos los lags

### 6. Análisis de Entropía
**Qué verifica:** La cantidad de "sorpresa" o información en los datos.

**Teoría:**
- Entropía de Shannon: H = -Σ p_i log₂(p_i)
- Para uniforme con n bins: H_max = log₂(n)

**Interpretación:**
- H/H_max > 0.95: Alta entropía (buena aleatoriedad)
- H/H_max < 0.95: Baja entropía (patrones predecibles)

## Uso del Sistema

### Generar Datos

```bash
./bin/rng_analysis -n 100000 -m fixedbits -b 1024 -o results/data/rsa1024.csv -v
python3 scripts/analyze_randomness.py results/data/rsa1024.csv results/plots
./bin/rng_analysis -n 1000000 -m fixedbits -b 1024 -o results/data/rsa1024.csv -v
python3 scripts/analyze_randomness.py results/data/rsa1024.csv results/plots
./bin/rng_analysis -n 100000 -m fixedbits -b 2048 -o results/data/rsa2048.csv -v
python3 scripts/analyze_randomness.py results/data/rsa2048.csv results/plots
./bin/rng_analysis -n 1000000 -m fixedbits -b 2048 -o results/data/rsa2048.csv -v
python3 scripts/analyze_randomness.py results/data/rsa2048.csv results/plots
./bin/rng_analysis -n 100000 -m fixedbits -b 4096 -o results/data/rsa4096.csv -v
python3 scripts/analyze_randomness.py results/data/rsa4096.csv results/plots
./bin/rng_analysis -n 1000000 -m fixedbits -b 4096 -o results/data/rsa4096.csv -v
python3 scripts/analyze_randomness.py results/data/rsa4096.csv results/plots
```

## Pruebas realizadas

### RSA

#### RSA 1024 bits

##### Prueba de 100k muestras

<details>
<summary>Mostrar resultados</summary>

```bash
root@18fbc909a7d0:/workspace# ./bin/rng_analysis -n 100000 -m fixedbits -b 1024 -o results/data/rsa1024.csv -v

======================================================================
RNG DATA GENERATOR v3.0
======================================================================
  Output file:      results/data/rsa1024.csv
  Samples:          100000
  Generation mode:  fixedbits
  Bits per number:  1024
  Seed mode:        fixed
  Seed value:       0
  Normalized:       YES
======================================================================

Generating 100000 numbers of 1024 bits (normalized)...
  Generated 100000 / 100000
Generated 100000 normalized numbers

======================================================================
COMPLETED
======================================================================
  Time elapsed:     51 ms
  Rate:             1.96078e+06 samples/sec
  Output file:      results/data/rsa1024.csv
  Data range:       [0.0, 1.0] (normalized)
======================================================================

Next step: Analyze with Python:
  python3 scripts/analyze_randomness.py results/data/rsa1024.csv results/plots

root@18fbc909a7d0:/workspace# python3 scripts/analyze_randomness.py results/data/rsa1024.csv results/plots

======================================================================
  ANÁLISIS ESTADÍSTICO DE ALEATORIEDAD
======================================================================
  Archivo de entrada: results/data/rsa1024.csv

  Cargando datos...
  Cargados 100,000 valores
  Rango: [0.000011, 0.999991] (normalizado)

======================================================================
  SUITE COMPLETA DE TESTS ESTADÍSTICOS
======================================================================

======================================================================
  ANÁLISIS DE MEDIA Y VARIANZA
======================================================================
  Media observada:  0.5001
  Media esperada:   0.5000
  Var. observada:   0.0830
  Var. esperada:    0.0833
  P-value (media):  0.877271
  Resultado:        PASS

======================================================================
  TEST DE UNIFORMIDAD (Chi-cuadrado)
======================================================================
  Chi-cuadrado:     4.5966
  Grados libertad:  9
  P-value:          0.867962
  Resultado:        PASS (alpha=0.05)
  -> Los datos SON uniformes (no rechazo H0)

======================================================================
  TEST DE KOLMOGOROV-SMIRNOV
======================================================================
  Estadístico KS:   0.001954
  P-value:          0.839049
  Resultado:        PASS (alpha=0.05)

======================================================================
  TEST DE RUNS (RACHAS)
======================================================================
  Runs observados:  49892
  Runs esperados:   50001.00
  Z-score:          -0.4875
  P-value:          0.625930
  Resultado:        PASS (alpha=0.05)

======================================================================
  TEST DE AUTOCORRELACIÓN
======================================================================
  Lags analizados:  20
  Límite confianza: ±0.0062
  Violaciones:      2 / 20
  Máximo permitido: 2
  Resultado:        PASS

======================================================================
  ANÁLISIS DE ENTROPÍA
======================================================================
  Entropía:         7.9981 bits
  Entropía máxima:  8.0000 bits
  Ratio:            99.9764%
  Resultado:        PASS

======================================================================
  RESUMEN DE RESULTADOS
======================================================================
  Media y Varianza          PASS
  Uniformidad (Chi²)        PASS
  Kolmogorov-Smirnov        PASS
  Runs (Rachas)             PASS
  Autocorrelación           PASS
  Entropía                  PASS

  Total: 6/6 tests pasados (100.0%)

  EXCELENTE: Todos los tests pasaron
  -> El RNG muestra características de aleatoriedad criptográfica
======================================================================

======================================================================
  GENERANDO GRÁFICAS
======================================================================
  Guardado: results/plots/histogram.png
  Guardado: results/plots/autocorrelation.png
  Guardado: results/plots/consecutive_pairs.png
  Guardado: results/plots/runs.png
  Guardado: results/plots/cdf.png

  Todas las gráficas generadas en: results/plots

======================================================================
  ANÁLISIS COMPLETADO
======================================================================
  Revisa los resultados y las gráficas generadas.
======================================================================

root@18fbc909a7d0:/workspace# 
```

</details>

#### Prueba de 1M muestras

<details>
<summary>Mostrar resultados</summary>

```bash
root@18fbc909a7d0:/workspace# ./bin/rng_analysis -n 1000000 -m fixedbits -b 1024 -o results/data/rsa1024.csv -v

======================================================================
RNG DATA GENERATOR v3.0
======================================================================
  Output file:      results/data/rsa1024.csv
  Samples:          1000000
  Generation mode:  fixedbits
  Bits per number:  1024
  Seed mode:        fixed
  Seed value:       0
  Normalized:       YES
======================================================================

Generating 1000000 numbers of 1024 bits (normalized)...
  Generated 1000000 / 1000000
Generated 1000000 normalized numbers

======================================================================
COMPLETED
======================================================================
  Time elapsed:     536 ms
  Rate:             1.86567e+06 samples/sec
  Output file:      results/data/rsa1024.csv
  Data range:       [0.0, 1.0] (normalized)
======================================================================

Next step: Analyze with Python:
  python3 scripts/analyze_randomness.py results/data/rsa1024.csv results/plots

root@18fbc909a7d0:/workspace# python3 scripts/analyze_randomness.py results/data/rsa1024.csv results/plots

======================================================================
  ANÁLISIS ESTADÍSTICO DE ALEATORIEDAD
======================================================================
  Archivo de entrada: results/data/rsa1024.csv

  Cargando datos...
  Cargados 1,000,000 valores
  Rango: [0.000000, 1.000000] (normalizado)

======================================================================
  SUITE COMPLETA DE TESTS ESTADÍSTICOS
======================================================================

======================================================================
  ANÁLISIS DE MEDIA Y VARIANZA
======================================================================
  Media observada:  0.5003
  Media esperada:   0.5000
  Var. observada:   0.0832
  Var. esperada:    0.0833
  P-value (media):  0.270888
  Resultado:        PASS

======================================================================
  TEST DE UNIFORMIDAD (Chi-cuadrado)
======================================================================
  Chi-cuadrado:     7.5265
  Grados libertad:  9
  P-value:          0.582472
  Resultado:        PASS (alpha=0.05)
  -> Los datos SON uniformes (no rechazo H0)

======================================================================
  TEST DE KOLMOGOROV-SMIRNOV
======================================================================
  Estadístico KS:   0.001021
  P-value:          0.248496
  Resultado:        PASS (alpha=0.05)

======================================================================
  TEST DE RUNS (RACHAS)
======================================================================
  Runs observados:  500108
  Runs esperados:   500001.00
  Z-score:          0.1513
  P-value:          0.879723
  Resultado:        PASS (alpha=0.05)

======================================================================
  TEST DE AUTOCORRELACIÓN
======================================================================
  Lags analizados:  20
  Límite confianza: ±0.0020
  Violaciones:      1 / 20
  Máximo permitido: 2
  Resultado:        PASS

======================================================================
  ANÁLISIS DE ENTROPÍA
======================================================================
  Entropía:         7.9998 bits
  Entropía máxima:  8.0000 bits
  Ratio:            99.9979%
  Resultado:        PASS

======================================================================
  RESUMEN DE RESULTADOS
======================================================================
  Media y Varianza          PASS
  Uniformidad (Chi²)        PASS
  Kolmogorov-Smirnov        PASS
  Runs (Rachas)             PASS
  Autocorrelación           PASS
  Entropía                  PASS

  Total: 6/6 tests pasados (100.0%)

  EXCELENTE: Todos los tests pasaron
  -> El RNG muestra características de aleatoriedad criptográfica
======================================================================

======================================================================
  GENERANDO GRÁFICAS
======================================================================
  Guardado: results/plots/histogram.png
  Guardado: results/plots/autocorrelation.png
  Guardado: results/plots/consecutive_pairs.png
  Guardado: results/plots/runs.png
  Guardado: results/plots/cdf.png

  Todas las gráficas generadas en: results/plots

======================================================================
  ANÁLISIS COMPLETADO
======================================================================
  Revisa los resultados y las gráficas generadas.
======================================================================

root@18fbc909a7d0:/workspace# 
```

</details>

#### RSA 2048 bits

##### Prueba de 100k muestras

<details>
<summary>Mostrar resultados</summary>

```bash
root@18fbc909a7d0:/workspace# ./bin/rng_analysis -n 100000 -m fixedbits -b 2048 -o results/data/rsa2048.csv -v

======================================================================
RNG DATA GENERATOR v3.0
======================================================================
  Output file:      results/data/rsa2048.csv
  Samples:          100000
  Generation mode:  fixedbits
  Bits per number:  2048
  Seed mode:        fixed
  Seed value:       0
  Normalized:       YES
======================================================================

Generating 100000 numbers of 2048 bits (normalized)...
  Generated 100000 / 100000
Generated 100000 normalized numbers

======================================================================
COMPLETED
======================================================================
  Time elapsed:     62 ms
  Rate:             1.6129e+06 samples/sec
  Output file:      results/data/rsa2048.csv
  Data range:       [0.0, 1.0] (normalized)
======================================================================

Next step: Analyze with Python:
  python3 scripts/analyze_randomness.py results/data/rsa2048.csv results/plots

root@18fbc909a7d0:/workspace# python3 scripts/analyze_randomness.py results/data/rsa2048.csv results/plots

======================================================================
  ANÁLISIS ESTADÍSTICO DE ALEATORIEDAD
======================================================================
  Archivo de entrada: results/data/rsa2048.csv

  Cargando datos...
  Cargados 100,000 valores
  Rango: [0.000022, 0.999998] (normalizado)

======================================================================
  SUITE COMPLETA DE TESTS ESTADÍSTICOS
======================================================================

======================================================================
  ANÁLISIS DE MEDIA Y VARIANZA
======================================================================
  Media observada:  0.5004
  Media esperada:   0.5000
  Var. observada:   0.0833
  Var. esperada:    0.0833
  P-value (media):  0.630015
  Resultado:        PASS

======================================================================
  TEST DE UNIFORMIDAD (Chi-cuadrado)
======================================================================
  Chi-cuadrado:     8.9348
  Grados libertad:  9
  P-value:          0.443314
  Resultado:        PASS (alpha=0.05)
  -> Los datos SON uniformes (no rechazo H0)

======================================================================
  TEST DE KOLMOGOROV-SMIRNOV
======================================================================
  Estadístico KS:   0.002676
  P-value:          0.470284
  Resultado:        PASS (alpha=0.05)

======================================================================
  TEST DE RUNS (RACHAS)
======================================================================
  Runs observados:  49542
  Runs esperados:   50001.00
  Z-score:          -2.0527
  P-value:          0.040101
  Resultado:        FAIL (alpha=0.05)

======================================================================
  TEST DE AUTOCORRELACIÓN
======================================================================
  Lags analizados:  20
  Límite confianza: ±0.0062
  Violaciones:      3 / 20
  Máximo permitido: 2
  Resultado:        FAIL

======================================================================
  ANÁLISIS DE ENTROPÍA
======================================================================
  Entropía:         7.9977 bits
  Entropía máxima:  8.0000 bits
  Ratio:            99.9716%
  Resultado:        PASS

======================================================================
  RESUMEN DE RESULTADOS
======================================================================
  Media y Varianza          PASS
  Uniformidad (Chi²)        PASS
  Kolmogorov-Smirnov        PASS
  Runs (Rachas)             FAIL
  Autocorrelación           FAIL
  Entropía                  PASS

  Total: 4/6 tests pasados (66.7%)

  ADVERTENCIA: Varios tests fallaron
  -> Revisar la calidad del RNG
======================================================================

======================================================================
  GENERANDO GRÁFICAS
======================================================================
  Guardado: results/plots/histogram.png
  Guardado: results/plots/autocorrelation.png
  Guardado: results/plots/consecutive_pairs.png
  Guardado: results/plots/runs.png
  Guardado: results/plots/cdf.png

  Todas las gráficas generadas en: results/plots

======================================================================
  ANÁLISIS COMPLETADO
======================================================================
  Revisa los resultados y las gráficas generadas.
======================================================================

root@18fbc909a7d0:/workspace# 
```

</details>

##### Prueba de 1M muestras

<details>
<summary>Mostrar resultados</summary>

```bash
root@18fbc909a7d0:/workspace# ./bin/rng_analysis -n 1000000 -m fixedbits -b 2048 -o results/data/rsa2048.csv -v

======================================================================
RNG DATA GENERATOR v3.0
======================================================================
  Output file:      results/data/rsa2048.csv
  Samples:          1000000
  Generation mode:  fixedbits
  Bits per number:  2048
  Seed mode:        fixed
  Seed value:       0
  Normalized:       YES
======================================================================

Generating 1000000 numbers of 2048 bits (normalized)...
  Generated 1000000 / 1000000
Generated 1000000 normalized numbers

======================================================================
COMPLETED
======================================================================
  Time elapsed:     601 ms
  Rate:             1.66389e+06 samples/sec
  Output file:      results/data/rsa2048.csv
  Data range:       [0.0, 1.0] (normalized)
======================================================================

Next step: Analyze with Python:
  python3 scripts/analyze_randomness.py results/data/rsa2048.csv results/plots

root@18fbc909a7d0:/workspace# python3 scripts/analyze_randomness.py results/data/rsa2048.csv results/plots

======================================================================
  ANÁLISIS ESTADÍSTICO DE ALEATORIEDAD
======================================================================
  Archivo de entrada: results/data/rsa2048.csv

  Cargando datos...
  Cargados 1,000,000 valores
  Rango: [0.000000, 1.000000] (normalizado)

======================================================================
  SUITE COMPLETA DE TESTS ESTADÍSTICOS
======================================================================

======================================================================
  ANÁLISIS DE MEDIA Y VARIANZA
======================================================================
  Media observada:  0.4999
  Media esperada:   0.5000
  Var. observada:   0.0833
  Var. esperada:    0.0833
  P-value (media):  0.613836
  Resultado:        PASS

======================================================================
  TEST DE UNIFORMIDAD (Chi-cuadrado)
======================================================================
  Chi-cuadrado:     5.3805
  Grados libertad:  9
  P-value:          0.799957
  Resultado:        PASS (alpha=0.05)
  -> Los datos SON uniformes (no rechazo H0)

======================================================================
  TEST DE KOLMOGOROV-SMIRNOV
======================================================================
  Estadístico KS:   0.000765
  P-value:          0.602302
  Resultado:        PASS (alpha=0.05)

======================================================================
  TEST DE RUNS (RACHAS)
======================================================================
  Runs observados:  499137
  Runs esperados:   500001.00
  Z-score:          -1.2219
  P-value:          0.221753
  Resultado:        PASS (alpha=0.05)

======================================================================
  TEST DE AUTOCORRELACIÓN
======================================================================
  Lags analizados:  20
  Límite confianza: ±0.0020
  Violaciones:      4 / 20
  Máximo permitido: 2
  Resultado:        FAIL

======================================================================
  ANÁLISIS DE ENTROPÍA
======================================================================
  Entropía:         7.9998 bits
  Entropía máxima:  8.0000 bits
  Ratio:            99.9978%
  Resultado:        PASS

======================================================================
  RESUMEN DE RESULTADOS
======================================================================
  Media y Varianza          PASS
  Uniformidad (Chi²)        PASS
  Kolmogorov-Smirnov        PASS
  Runs (Rachas)             PASS
  Autocorrelación           FAIL
  Entropía                  PASS

  Total: 5/6 tests pasados (83.3%)

  BUENO: La mayoría de tests pasaron
  -> El RNG es adecuado para uso general
======================================================================

======================================================================
  GENERANDO GRÁFICAS
======================================================================
  Guardado: results/plots/histogram.png
  Guardado: results/plots/autocorrelation.png
  Guardado: results/plots/consecutive_pairs.png
  Guardado: results/plots/runs.png
  Guardado: results/plots/cdf.png

  Todas las gráficas generadas en: results/plots

======================================================================
  ANÁLISIS COMPLETADO
======================================================================
  Revisa los resultados y las gráficas generadas.
======================================================================

root@18fbc909a7d0:/workspace# 
```

</details>

#### RSA 4096 bits

##### Prueba de 100k muestras

<details>
<summary>Mostrar resultados</summary>

```bash
root@18fbc909a7d0:/workspace# ./bin/rng_analysis -n 100000 -m fixedbits -b 4096 -o results/data/rsa4096.csv -v

======================================================================
RNG DATA GENERATOR v3.0
======================================================================
  Output file:      results/data/rsa4096.csv
  Samples:          100000
  Generation mode:  fixedbits
  Bits per number:  4096
  Seed mode:        fixed
  Seed value:       0
  Normalized:       YES
======================================================================

Generating 100000 numbers of 4096 bits (normalized)...
  Generated 100000 / 100000
Generated 100000 normalized numbers

======================================================================
COMPLETED
======================================================================
  Time elapsed:     76 ms
  Rate:             1.31579e+06 samples/sec
  Output file:      results/data/rsa4096.csv
  Data range:       [0.0, 1.0] (normalized)
======================================================================

Next step: Analyze with Python:
  python3 scripts/analyze_randomness.py results/data/rsa4096.csv results/plots

root@18fbc909a7d0:/workspace# python3 scripts/analyze_randomness.py results/data/rsa4096.csv results/plots

======================================================================
  ANÁLISIS ESTADÍSTICO DE ALEATORIEDAD
======================================================================
  Archivo de entrada: results/data/rsa4096.csv

  Cargando datos...
  Cargados 100,000 valores
  Rango: [0.000013, 1.000000] (normalizado)

======================================================================
  SUITE COMPLETA DE TESTS ESTADÍSTICOS
======================================================================

======================================================================
  ANÁLISIS DE MEDIA Y VARIANZA
======================================================================
  Media observada:  0.4995
  Media esperada:   0.5000
  Var. observada:   0.0834
  Var. esperada:    0.0833
  P-value (media):  0.613574
  Resultado:        PASS

======================================================================
  TEST DE UNIFORMIDAD (Chi-cuadrado)
======================================================================
  Chi-cuadrado:     9.7848
  Grados libertad:  9
  P-value:          0.368186
  Resultado:        PASS (alpha=0.05)
  -> Los datos SON uniformes (no rechazo H0)

======================================================================
  TEST DE KOLMOGOROV-SMIRNOV
======================================================================
  Estadístico KS:   0.002377
  P-value:          0.623701
  Resultado:        PASS (alpha=0.05)

======================================================================
  TEST DE RUNS (RACHAS)
======================================================================
  Runs observados:  50083
  Runs esperados:   50001.00
  Z-score:          0.3667
  P-value:          0.713832
  Resultado:        PASS (alpha=0.05)

======================================================================
  TEST DE AUTOCORRELACIÓN
======================================================================
  Lags analizados:  20
  Límite confianza: ±0.0062
  Violaciones:      1 / 20
  Máximo permitido: 2
  Resultado:        PASS

======================================================================
  ANÁLISIS DE ENTROPÍA
======================================================================
  Entropía:         7.9979 bits
  Entropía máxima:  8.0000 bits
  Ratio:            99.9742%
  Resultado:        PASS

======================================================================
  RESUMEN DE RESULTADOS
======================================================================
  Media y Varianza          PASS
  Uniformidad (Chi²)        PASS
  Kolmogorov-Smirnov        PASS
  Runs (Rachas)             PASS
  Autocorrelación           PASS
  Entropía                  PASS

  Total: 6/6 tests pasados (100.0%)

  EXCELENTE: Todos los tests pasaron
  -> El RNG muestra características de aleatoriedad criptográfica
======================================================================

======================================================================
  GENERANDO GRÁFICAS
======================================================================
  Guardado: results/plots/histogram.png
  Guardado: results/plots/autocorrelation.png
  Guardado: results/plots/consecutive_pairs.png
  Guardado: results/plots/runs.png
  Guardado: results/plots/cdf.png

  Todas las gráficas generadas en: results/plots

======================================================================
  ANÁLISIS COMPLETADO
======================================================================
  Revisa los resultados y las gráficas generadas.
======================================================================

root@18fbc909a7d0:/workspace# 
```

</details>

##### Prueba de 1M muestras

<details>
<summary>Mostrar resultados</summary>

```bash
root@18fbc909a7d0:/workspace# ./bin/rng_analysis -n 1000000 -m fixedbits -b 4096 -o results/data/rsa4096.csv -v

======================================================================
RNG DATA GENERATOR v3.0
======================================================================
  Output file:      results/data/rsa4096.csv
  Samples:          1000000
  Generation mode:  fixedbits
  Bits per number:  4096
  Seed mode:        fixed
  Seed value:       0
  Normalized:       YES
======================================================================

Generating 1000000 numbers of 4096 bits (normalized)...
  Generated 1000000 / 1000000
Generated 1000000 normalized numbers

======================================================================
COMPLETED
======================================================================
  Time elapsed:     759 ms
  Rate:             1.31752e+06 samples/sec
  Output file:      results/data/rsa4096.csv
  Data range:       [0.0, 1.0] (normalized)
======================================================================

Next step: Analyze with Python:
  python3 scripts/analyze_randomness.py results/data/rsa4096.csv results/plots

root@18fbc909a7d0:/workspace# python3 scripts/analyze_randomness.py results/data/rsa4096.csv results/plots

======================================================================
  ANÁLISIS ESTADÍSTICO DE ALEATORIEDAD
======================================================================
  Archivo de entrada: results/data/rsa4096.csv

  Cargando datos...
  Cargados 1,000,000 valores
  Rango: [0.000001, 1.000000] (normalizado)

======================================================================
  SUITE COMPLETA DE TESTS ESTADÍSTICOS
======================================================================

======================================================================
  ANÁLISIS DE MEDIA Y VARIANZA
======================================================================
  Media observada:  0.5001
  Media esperada:   0.5000
  Var. observada:   0.0833
  Var. esperada:    0.0833
  P-value (media):  0.763279
  Resultado:        PASS

======================================================================
  TEST DE UNIFORMIDAD (Chi-cuadrado)
======================================================================
  Chi-cuadrado:     11.1204
  Grados libertad:  9
  P-value:          0.267546
  Resultado:        PASS (alpha=0.05)
  -> Los datos SON uniformes (no rechazo H0)

======================================================================
  TEST DE KOLMOGOROV-SMIRNOV
======================================================================
  Estadístico KS:   0.000679
  P-value:          0.745534
  Resultado:        PASS (alpha=0.05)

======================================================================
  TEST DE RUNS (RACHAS)
======================================================================
  Runs observados:  499692
  Runs esperados:   500001.00
  Z-score:          -0.4370
  P-value:          0.662117
  Resultado:        PASS (alpha=0.05)

======================================================================
  TEST DE AUTOCORRELACIÓN
======================================================================
  Lags analizados:  20
  Límite confianza: ±0.0020
  Violaciones:      0 / 20
  Máximo permitido: 2
  Resultado:        PASS

======================================================================
  ANÁLISIS DE ENTROPÍA
======================================================================
  Entropía:         7.9998 bits
  Entropía máxima:  8.0000 bits
  Ratio:            99.9976%
  Resultado:        PASS

======================================================================
  RESUMEN DE RESULTADOS
======================================================================
  Media y Varianza          PASS
  Uniformidad (Chi²)        PASS
  Kolmogorov-Smirnov        PASS
  Runs (Rachas)             PASS
  Autocorrelación           PASS
  Entropía                  PASS

  Total: 6/6 tests pasados (100.0%)

  EXCELENTE: Todos los tests pasaron
  -> El RNG muestra características de aleatoriedad criptográfica
======================================================================

======================================================================
  GENERANDO GRÁFICAS
======================================================================
  Guardado: results/plots/histogram.png
  Guardado: results/plots/autocorrelation.png
  Guardado: results/plots/consecutive_pairs.png
  Guardado: results/plots/runs.png
  Guardado: results/plots/cdf.png

  Todas las gráficas generadas en: results/plots

======================================================================
  ANÁLISIS COMPLETADO
======================================================================
  Revisa los resultados y las gráficas generadas.
======================================================================

root@18fbc909a7d0:/workspace# 
```

</details>

## VALIDACIÓN

### Expectativas para RNG Criptográfico

| Test | P-value mínimo | Criterio |
|------|----------------|----------|
| Media y Varianza | > 0.05 | ~0.5 ± 0.01 |
| Uniformidad (Chi²) | > 0.05 | Distribución plana |
| Kolmogorov-Smirnov | > 0.05 | CDF uniforme |
| Runs | > 0.05 | Sin patrones |
| Autocorrelación | ≤10% violaciones | Independencia |
| Entropía | > 95% | Alta aleatoriedad |