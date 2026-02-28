#!/bin/bash
# compare_rsa_ecc.sh
# Comparación directa RSA vs ECC con seguridad equivalente
#
# Autor: Leon Elliott Fuller
# Fecha: 2026-01-04

set -e

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuración
ITERATIONS_RSA=50
ITERATIONS_ECC=100
OUTPUT_DIR="results/comparisons"
EXECUTABLE="./bin/bench"

# Crear directorio de resultados
mkdir -p "$OUTPUT_DIR"

echo -e "${BLUE}================================================================${NC}"
echo -e "${BLUE}   COMPARACIÓN RSA vs ECC (Seguridad Equivalente)${NC}"
echo -e "${BLUE}================================================================${NC}"
echo ""
echo "Iteraciones RSA: $ITERATIONS_RSA"
echo "Iteraciones ECC: $ITERATIONS_ECC"
echo "Directorio de salida: $OUTPUT_DIR"
echo ""

# Verificar que el ejecutable existe
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${RED}Error: No se encontró $EXECUTABLE${NC}"
    echo "Ejecuta 'make' primero"
    exit 1
fi

# Timestamp para los archivos
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

# Función para ejecutar y cronometrar
run_and_time() {
    local algo=$1
    local param=$2
    local iters=$3
    local output_file=$4
    local label=$5
    
    echo -e "${GREEN}>> $label${NC}"
    echo "   Archivo: $output_file"
    
    if [ "$algo" == "RSA" ]; then
        $EXECUTABLE -a RSA -b "$param" -i "$iters" > "$output_file" 2>&1
    else
        $EXECUTABLE -a ECC -c "$param" -i "$iters" > "$output_file" 2>&1
    fi
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN} Completado${NC}"
    else
        echo -e "${RED} Error${NC}"
    fi
    echo ""
}

# Comparación 1: RSA-2048 vs ECC P-256
echo -e "${YELLOW}======= Comparación 1: Seguridad Estándar =======${NC}"
echo "RSA-2048 bits = ECC P-256 (256 bits)"
echo ""

run_and_time "RSA" "2048" "$ITERATIONS_RSA" \
    "$OUTPUT_DIR/comp1_rsa2048_${TIMESTAMP}.txt" \
    "RSA-2048"

run_and_time "ECC" "P-256" "$ITERATIONS_ECC" \
    "$OUTPUT_DIR/comp1_ecc_p256_${TIMESTAMP}.txt" \
    "ECC P-256"

echo ""

# Comparación 2: RSA-3072 vs ECC P-256
echo -e "${YELLOW}======= Comparación 2: Seguridad Mejorada =======${NC}"
echo "RSA-3072 bits = ECC P-256 (256 bits)"
echo ""

run_and_time "RSA" "3072" "$ITERATIONS_RSA" \
    "$OUTPUT_DIR/comp2_rsa3072_${TIMESTAMP}.txt" \
    "RSA-3072"

run_and_time "ECC" "P-256" "$ITERATIONS_ECC" \
    "$OUTPUT_DIR/comp2_ecc_p256_${TIMESTAMP}.txt" \
    "ECC P-256 (repetido)"

echo ""

# Comparación 3: RSA-4096 vs ECC P-384
echo -e "${YELLOW}======= Comparación 3: Alta Seguridad =======${NC}"
echo "RSA-4096 bits = ECC P-384 (384 bits)"
echo ""

run_and_time "RSA" "4096" "$ITERATIONS_RSA" \
    "$OUTPUT_DIR/comp3_rsa4096_${TIMESTAMP}.txt" \
    "RSA-4096"

run_and_time "ECC" "P-384" "$ITERATIONS_ECC" \
    "$OUTPUT_DIR/comp3_ecc_p384_${TIMESTAMP}.txt" \
    "ECC P-384"

echo ""

# Comparación especial: secp256k1 (Bitcoin)
echo -e "${YELLOW}======= Extra: Bitcoin (secp256k1) =======${NC}"
echo "Seguridad ≈ RSA-3072"
echo ""

run_and_time "ECC" "secp256k1" "$ITERATIONS_ECC" \
    "$OUTPUT_DIR/extra_secp256k1_${TIMESTAMP}.txt" \
    "secp256k1"

echo ""

# Resumen
echo -e "${BLUE}================================================================${NC}"
echo -e "${GREEN}COMPARACIONES COMPLETADAS${NC}"
echo -e "${BLUE}================================================================${NC}"
echo ""
echo "Resultados guardados en: $OUTPUT_DIR"
echo ""
echo "Archivos generados:"
ls -lh "$OUTPUT_DIR"/*${TIMESTAMP}*.txt
echo ""
echo -e "${YELLOW}Equivalencias de seguridad:${NC}"
echo "  RSA-1024  =  ECC-160  (Débil, no recomendado)"
echo "  RSA-2048  =  ECC-224/256  (Mínimo actual)"
echo "  RSA-3072  =  ECC-256  (Recomendado)"
echo "  RSA-4096  =  ECC-384  (Alta seguridad)"
echo ""
echo -e "${GREEN}Para visualizar resultados, ejecuta:${NC}"
echo "  python3 scripts/plot_results.py $OUTPUT_DIR"