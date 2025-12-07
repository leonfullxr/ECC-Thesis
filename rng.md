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
# Generar 1M números en [0, 1000) con semilla fija
./bin/rng_analysis -n 1000000 -r 1000 -s fixed -o data/rng_bounded.csv -v

# Generar 10M bits aleatorios
./bin/rng_analysis -n 10000000 -m bits -s fixed -o data/rng_bits.csv -v

# Generar números de 2048 bits
./bin/rng_analysis -n 1000000 -m fixedbits -b 2048 -o data/rng_2048bit.csv -v

# Generar pares para correlación
./bin/rng_analysis -n 100000 -m pairs -r 10000 -o data/rng_pairs.csv -v
```

## Ejemplo de Salida

### Tests Estadísticos

```
root@18fbc909a7d0:/workspace# ./bin/rng_analysis -n 10000 -r 1000 -s fixed -o results/data/test.csv

======================================================================
RNG DATA GENERATOR
======================================================================
  Output file:      results/data/test.csv
  Samples:          10000
  Generation mode:  bounded
  Range:            [0, 1000)
  Seed mode:        fixed
  Seed value:       0
======================================================================


======================================================================
COMPLETED
======================================================================
  Time elapsed:     1 ms
  Rate:             1e+07 samples/sec
  Output file:      results/data/test.csv
======================================================================

Next step: Analyze with Python:
  python3 analyze_randomness.py results/data/test.csv

root@18fbc909a7d0:/workspace# python3 scripts/analyze_randomness.py results/data/test.csv results/plots

======================================================================
  ANÁLISIS ESTADÍSTICO DE ALEATORIEDAD
======================================================================
  Archivo de entrada: results/data/test.csv

  Cargando datos...
  Cargados 10,000 valores
  Rango original: [0, 999]
  Datos normalizados a: [0.0, 1.0]

======================================================================
  SUITE COMPLETA DE TESTS ESTADÍSTICOS
======================================================================

======================================================================
  ANÁLISIS DE MEDIA Y VARIANZA
======================================================================
  Media observada:  0.5019
  Media esperada:   0.5000
  Var. observada:   0.0834
  Var. esperada:    0.0833
  P-value (media):  0.507109
  Resultado:        PASS

======================================================================
  TEST DE UNIFORMIDAD (Chi-cuadrado)
======================================================================
  Chi-cuadrado:     5.6460
  Grados libertad:  9
  P-value:          0.774758
  Resultado:        PASS (alpha=0.05)
  -> Los datos SON uniformes (no rechazo H0)

======================================================================
  TEST DE KOLMOGOROV-SMIRNOV
======================================================================
  Estadístico KS:   0.007742
  P-value:          0.583929
  Resultado:        PASS (alpha=0.05)

======================================================================
  TEST DE RUNS (RACHAS)
======================================================================
  Runs observados:  5026
  Runs esperados:   5001.00
  Z-score:          0.5000
  P-value:          0.617055
  Resultado:        PASS (alpha=0.05)

======================================================================
  TEST DE AUTOCORRELACIÓN
======================================================================
  Lags analizados:  20
  Límite confianza: ±0.0196
  Violaciones:      0 / 20
  Resultado:        PASS

======================================================================
  ANÁLISIS DE ENTROPÍA
======================================================================
  Entropía:         7.9791 bits
  Entropía máxima:  8.0000 bits
  Ratio:            99.7385%
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

root@18fbc909a7d0:/workspace# ls results/plots/*.png
results/plots/autocorrelation.png  results/plots/cdf.png  results/plots/consecutive_pairs.png  results/plots/histogram.png  results/plots/runs.png

```

### Gráficas Generadas

Ver carpeta `plots/` para visualizaciones.

## Interpretación de Resultados

### Escenario Ideal
- ✅ Todos los tests pasan (p > 0.05)
- ✅ Entropía > 95%
- ✅ Autocorrelación negligible
- ✅ Histograma plano
- ✅ Scatter de pares uniforme

**Conclusión:** RNG de calidad criptográfica

### Escenario Bueno
- ✅ 5/6 tests pasan
- ✅ Entropía > 90%
- ⚠️ Alguna autocorrelación menor

**Conclusión:** RNG adecuado para uso general

### Escenario Problemático
- ❌ < 4/6 tests pasan
- ❌ Entropía < 85%
- ❌ Autocorrelación significativa
- ❌ Patrones visibles en gráficas

**Conclusión:** RNG de baja calidad, NO usar para criptografía

## Comparación con Otros RNGs

Para validar aún más, puedes comparar con:

### Generadores Estándar
```python
import random
import numpy as np

# Python random (Mersenne Twister)
data_mt = [random.randint(0, 999) for _ in range(1000000)]

# NumPy random
data_np = np.random.randint(0, 1000, size=1000000)

# Guardar y analizar
```

### /dev/urandom (Linux)
```bash
# Generar bytes aleatorios del sistema
python3 -c "
import struct
data = []
with open('/dev/urandom', 'rb') as f:
    for _ in range(1000000):
        data.append(struct.unpack('H', f.read(2))[0] % 1000)
with open('urandom_data.csv', 'w') as out:
    out.write('index,value\n')
    for i, v in enumerate(data):
        out.write(f'{i},{v}\n')
"

# Analizar
python3 scripts/analyze_randomness.py urandom_data.csv
```

## Qué hace este script

El script ./run_rng_analysis.sh:

   1. Compila la herramienta de análisis
   2. Genera 4 datasets diferentes:
      - 1M números en [0, 1000)
      - 5M bits individuales
      - 100K números de 32 bits
      - 100K pares consecutivos
   
   3. Ejecuta 6 tests estadísticos:
      ✓ Test de uniformidad (Chi-cuadrado)
      ✓ Test de Kolmogorov-Smirnov
      ✓ Test de runs (rachas)
      ✓ Test de autocorrelación
      ✓ Análisis de entropía
      ✓ Test de media y varianza
   
   4. Genera gráficas en plots/:
      - Histogramas
      - Autocorrelación
      - Pares consecutivos
      - CDF empírica
      - Análisis de runs
   
   5. Crea reporte en reports/