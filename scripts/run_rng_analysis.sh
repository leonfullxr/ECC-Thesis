#!/bin/bash
# run_rng_analysis.sh
# Script completo para análisis estadístico del RNG
# Autor: Leon Elliott Fuller
# Fecha: 2025-12-07

set -e  # Exit on error

echo ""
echo "===================================================================="
echo "         ANÁLISIS ESTADÍSTICO COMPLETO DEL RNG                     "
echo "===================================================================="
echo ""

# Configuración
DATA_DIR="results/data"
PLOTS_DIR="results/plots"
REPORT_DIR="results/reports"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
REPORT_FILE="$REPORT_DIR/rng_analysis_$TIMESTAMP.txt"

# Crear directorios
mkdir -p $DATA_DIR $PLOTS_DIR $REPORT_DIR

# ============================================================================
# PASO 1: Compilar herramienta de análisis
# ============================================================================

echo "===================================================================="
echo "  PASO 1: Compilando herramienta de análisis"
echo "===================================================================="
echo ""

if [ ! -f "bin/rng_analysis" ]; then
    echo "  Compilando rng_analysis..."
    make rng-analysis
    echo ""
else
    echo "  rng_analysis ya compilado"
    echo ""
fi

# ============================================================================
# PASO 2: Verificar dependencias Python
# ============================================================================

echo "===================================================================="
echo "  PASO 2: Verificando dependencias Python"
echo "===================================================================="
echo ""

# Verificar Python 3
if ! command -v python3 &> /dev/null; then
    echo "  Python 3 no encontrado"
    exit 1
fi

echo "  Python version: $(python3 --version)"
echo ""

# Verificar/instalar paquetes
echo "  Verificando paquetes Python..."
python3 -c "import pandas, numpy, matplotlib, seaborn, scipy" 2>/dev/null || {
    echo "  Instalando paquetes faltantes..."
    pip3 install --quiet pandas numpy matplotlib seaborn scipy
}
echo "  Todos los paquetes instalados"
echo ""

# ============================================================================
# PASO 3: Generar datasets
# ============================================================================

echo "===================================================================="
echo "  PASO 3: Generando datasets de números aleatorios"
echo "===================================================================="
echo ""

# Iniciar reporte
echo "====================================================================" > $REPORT_FILE
echo "  REPORTE DE ANÁLISIS ESTADÍSTICO DEL RNG" >> $REPORT_FILE
echo "  Fecha: $(date)" >> $REPORT_FILE
echo "====================================================================" >> $REPORT_FILE
echo "" >> $REPORT_FILE

# Dataset 1: Números acotados (para tests de uniformidad)
echo "  [1/4] Generando números acotados (1M samples)..."
./bin/rng_analysis -n 1000000 -r 1000 -s fixed -o $DATA_DIR/bounded.csv -v
echo ""

# Dataset 2: Bits individuales (para análisis de bits)
echo "  [2/4] Generando bits individuales (5M samples)..."
./bin/rng_analysis -n 5000000 -m bits -s fixed -o $DATA_DIR/bits.csv -v
echo ""

# Dataset 3: Números de 32 bits (para entropía)
echo "  [3/4] Generando números de 32 bits (100K samples)..."
./bin/rng_analysis -n 100000 -m fixedbits -b 32 -s fixed -o $DATA_DIR/fixedbits32.csv -v
echo ""

# Dataset 4: Pares consecutivos (para autocorrelación)
echo "  [4/4] Generando pares consecutivos (100K samples)..."
./bin/rng_analysis -n 100000 -m pairs -r 10000 -s fixed -o $DATA_DIR/pairs.csv -v
echo ""

# ============================================================================
# PASO 4: Análisis estadístico
# ============================================================================

echo "===================================================================="
echo "  PASO 4: Ejecutando análisis estadístico"
echo "===================================================================="
echo ""

# Análisis del dataset principal
echo "  Analizando dataset principal (números acotados)..."
echo "" >> $REPORT_FILE
echo "====================================================================" >> $REPORT_FILE
echo "  DATASET 1: Números Acotados [0, 1000)" >> $REPORT_FILE
echo "====================================================================" >> $REPORT_FILE
python3 scripts/analyze_randomness.py $DATA_DIR/bounded.csv $PLOTS_DIR | tee -a $REPORT_FILE
echo ""

# ============================================================================
# PASO 5: Análisis con diferentes semillas
# ============================================================================

echo "===================================================================="
echo "  PASO 5: Análisis con diferentes semillas"
echo "===================================================================="
echo ""

echo "" >> $REPORT_FILE
echo "====================================================================" >> $REPORT_FILE
echo "  COMPARACIÓN CON DIFERENTES SEMILLAS" >> $REPORT_FILE
echo "====================================================================" >> $REPORT_FILE

for seed in 0 12345 99999; do
    echo "  Probando con semilla = $seed..."
    ./bin/rng_analysis -n 100000 -r 1000 -s fixed -S $seed -o $DATA_DIR/seed_$seed.csv > /dev/null
    
    echo "" >> $REPORT_FILE
    echo "--- Semilla: $seed ---" >> $REPORT_FILE
    python3 scripts/analyze_randomness.py $DATA_DIR/seed_$seed.csv $PLOTS_DIR 2>&1 | \
        grep -E "(Test de|P-value|Resultado|Total:|EXCELENTE|BUENO|ADVERTENCIA)" >> $REPORT_FILE
    echo ""
done

echo ""

# ============================================================================
# PASO 6: Resumen y conclusiones
# ============================================================================

echo "===================================================================="
echo "  RESUMEN Y CONCLUSIONES"
echo "===================================================================="
echo ""

echo "" >> $REPORT_FILE
echo "====================================================================" >> $REPORT_FILE
echo "  ARCHIVOS GENERADOS" >> $REPORT_FILE
echo "====================================================================" >> $REPORT_FILE
echo "" >> $REPORT_FILE
echo "Datasets:" >> $REPORT_FILE
ls -lh $DATA_DIR/*.csv 2>/dev/null | awk '{print "  " $9 " - " $5}' >> $REPORT_FILE
echo "" >> $REPORT_FILE
echo "Gráficas:" >> $REPORT_FILE
ls -lh $PLOTS_DIR/*.png 2>/dev/null | awk '{print "  " $9}' >> $REPORT_FILE || echo "  (No gráficas generadas)" >> $REPORT_FILE
echo "" >> $REPORT_FILE

# Mostrar resumen
cat << EOF

ANÁLISIS COMPLETADO

Archivos generados:
  - Reporte completo: $REPORT_FILE
  - Datasets:         $DATA_DIR/
  - Gráficas:         $PLOTS_DIR/

Siguiente paso:
  1. Revisar el reporte: cat $REPORT_FILE
  2. Ver gráficas en: $PLOTS_DIR/
  3. Incluir resultados en el TFG

EOF

echo "===================================================================="
echo ""

# Resumen rápido en pantalla
echo "RESUMEN DE TESTS (ver reporte completo para detalles):"
echo ""
grep -A 10 "RESUMEN DE RESULTADOS" $REPORT_FILE | head -15

echo ""
echo "Para más detalles: cat $REPORT_FILE"
echo ""