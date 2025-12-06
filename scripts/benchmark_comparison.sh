#!/bin/bash
# benchmark_comparison.sh
# Script para comparar el rendimiento de diferentes tamaños de clave RSA
# Autor: Leon Elliott Fuller

# Configuración
ITERATIONS=10
SEED_MODE="fixed"
OUTPUT_FILE="benchmark_results_$(date +%Y%m%d_%H%M%S).txt"

echo "============================================================================="
echo "BENCHMARK COMPARATIVO RSA - DIFERENTES TAMAÑOS DE CLAVE"
echo "============================================================================="
echo ""
echo "Configuración:"
echo "  - Iteraciones por tamaño: $ITERATIONS"
echo "  - Modo de semilla: $SEED_MODE"
echo "  - Archivo de salida: $OUTPUT_FILE"
echo ""
echo "============================================================================="
echo ""

# Compilar si es necesario
if [ ! -f bin/bench ]; then
    echo "Compilando proyecto..."
    make > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo "Compilación exitosa"
    else
        echo "Error en compilación"
        exit 1
    fi
    echo ""
fi

# Array de tamaños de clave a probar
KEY_SIZES=(1024 2048 3072 4096)

# Inicializar archivo de resultados
echo "Benchmark Results - $(date)" > $OUTPUT_FILE
echo "Iterations: $ITERATIONS" >> $OUTPUT_FILE
echo "Seed mode: $SEED_MODE" >> $OUTPUT_FILE
echo "" >> $OUTPUT_FILE

# Ejecutar benchmarks
for size in "${KEY_SIZES[@]}"; do
    echo "Ejecutando benchmark para RSA-$size bits..."
    echo "---------------------------------------------------------------------"
    echo ""
    
    echo "=== RSA-$size bits ===" >> $OUTPUT_FILE
    ./bin/bench -a RSA -b $size -i $ITERATIONS -s $SEED_MODE 2>&1 | tee -a $OUTPUT_FILE
    echo "" >> $OUTPUT_FILE
    
    echo "Completado RSA-$size"
    echo ""
done

echo "============================================================================="
echo "BENCHMARK COMPLETADO"
echo "============================================================================="
echo ""
echo "Resultados guardados en: $OUTPUT_FILE"
echo ""

# Extraer y mostrar resumen
echo "RESUMEN DE TIEMPOS PROMEDIO (Key Generation):"
echo "---------------------------------------------------------------------"
grep -A 4 "Key Generation" $OUTPUT_FILE | grep "Average:" | \
    awk -v OFS='\t' '{print $0}' | \
    nl -w2 -s". " | \
    sed 's/^/  /'
echo ""

echo "Para análisis detallado, consulte: $OUTPUT_FILE"
echo ""