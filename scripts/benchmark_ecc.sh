#!/bin/bash
# benchmark_ecc.sh
# Benchmark completo de ECC con todas las curvas
#
# Autor: Leon Elliott Fuller
# Fecha: 2026-01-04

set -e

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuración
ITERATIONS=100
OUTPUT_DIR="results/benchmarks"
EXECUTABLE="./bin/bench"

# Crear directorio de resultados
mkdir -p "$OUTPUT_DIR"

echo -e "${BLUE}================================================${NC}"
echo -e "${BLUE}   BENCHMARK COMPLETO DE ECC${NC}"
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
    local curve=$1
    local output_file=$2
    
    echo -e "${GREEN}>> Benchmark: $curve${NC}"
    echo "   Archivo: $output_file"
    
    $EXECUTABLE -a ECC -c "$curve" -i $ITERATIONS > "$output_file" 2>&1
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN} Completado${NC}"
    else
        echo -e "${RED} Error${NC}"
    fi
    echo ""
}

# Timestamp para los archivos
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

# Ejecutar benchmarks para cada curva
echo -e "${BLUE}------ NIST P-256 ------${NC}"
run_benchmark "P-256" "$OUTPUT_DIR/ecc_p256_${TIMESTAMP}.txt"

echo -e "${BLUE}------ NIST P-384 ------${NC}"
run_benchmark "P-384" "$OUTPUT_DIR/ecc_p384_${TIMESTAMP}.txt"

echo -e "${BLUE}------ secp256k1 (Bitcoin) ------${NC}"
run_benchmark "secp256k1" "$OUTPUT_DIR/ecc_secp256k1_${TIMESTAMP}.txt"

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
echo -e "${GREEN}Para comparar con RSA, ejecuta: ./scripts/compare_rsa_ecc.sh${NC}"