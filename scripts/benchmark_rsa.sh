#!/bin/bash
# benchmark_rsa.sh
# Benchmark completo de RSA con diferentes tamaños de clave
#
# Autor: Leon Elliott Fuller
# Fecha: 2025-01-04

set -e

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuración
ITERATIONS=50  # Menos iteraciones porque RSA es más lento
OUTPUT_DIR="results/benchmarks"
EXECUTABLE="./bin/bench"

# Crear directorio de resultados
mkdir -p "$OUTPUT_DIR"

echo -e "${BLUE}================================================${NC}"
echo -e "${BLUE}   BENCHMARK COMPLETO DE RSA${NC}"
echo -e "${BLUE}================================================${NC}"
echo ""
echo "Iteraciones por test: $ITERATIONS"
echo "Directorio de salida: $OUTPUT_DIR"
echo ""

# Verificar que el ejecutable existe
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${RED}Error: No se encontró $EXECUTABLE${NC}"
    echo "Ejecuta 'make' primero"
    exit 1
fi

# Función para ejecutar benchmark
run_benchmark() {
    local bits=$1
    local output_file=$2
    
    echo -e "${GREEN}>> Benchmark: RSA-$bits${NC}"
    echo "   Archivo: $output_file"
    
    $EXECUTABLE -a RSA -b "$bits" -i $ITERATIONS > "$output_file" 2>&1
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN} Completado${NC}"
    else
        echo -e "${RED} Error${NC}"
    fi
    echo ""
}

# Timestamp para los archivos
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

# Ejecutar benchmarks para cada tamaño de clave
echo -e "${BLUE}------ RSA-1024 ------${NC}"
run_benchmark "1024" "$OUTPUT_DIR/rsa_1024_${TIMESTAMP}.txt"

echo -e "${BLUE}------ RSA-2048 ------${NC}"
run_benchmark "2048" "$OUTPUT_DIR/rsa_2048_${TIMESTAMP}.txt"

echo -e "${BLUE}------ RSA-3072 ------${NC}"
run_benchmark "3072" "$OUTPUT_DIR/rsa_3072_${TIMESTAMP}.txt"

echo -e "${BLUE}------ RSA-4096 ------${NC}"
run_benchmark "4096" "$OUTPUT_DIR/rsa_4096_${TIMESTAMP}.txt"

# Resumen
echo -e "${BLUE}================================================${NC}"
echo -e "${GREEN}BENCHMARKS COMPLETADOS${NC}"
echo -e "${BLUE}================================================${NC}"
echo ""
echo "Resultados guardados en: $OUTPUT_DIR"
echo ""
echo "Archivos generados:"
ls -lh "$OUTPUT_DIR"/*${TIMESTAMP}*.txt
echo ""
echo -e "${GREEN}Para comparar con ECC, ejecuta: ./scripts/compare_rsa_ecc.sh${NC}"