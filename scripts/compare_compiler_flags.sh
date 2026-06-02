#!/usr/bin/env bash
# compare_compiler_flags.sh
# Mide el rendimiento de ECC (secp256k1) compilando el motor de benchmarking
# con distintos conjuntos de opciones de optimizacion de g++, y genera la
# grafica correspondiente.
#
# Uso:
#   bash scripts/compare_compiler_flags.sh [ITERACIONES]
# Ejemplo:
#   bash scripts/compare_compiler_flags.sh 100
#
# Genera:
#   results/compiler_flags.csv      (una fila por conjunto de flags)
#   results/chart_compiler_flags.png (grafica, via visualize_compiler_flags.py)
#
# Autor: Leon Elliott Fuller
# Date: 2026-05-29
set -euo pipefail

# ============================================================================
# CONFIGURACION
# ============================================================================

ITERATIONS=${1:-100}
SEED_MODE="fixed"
CURVE="secp256k1"
RESULTS_DIR="results"
BIN_DIR="bin"
LIBS="-lntl -lgmp -lpthread"
BASE_STD="-std=c++17"

# Deteccion automatica del layout de fuentes:
#   - repo con src/ e include/ (igual que el Makefile)
#   - o layout plano (todos los .cpp en el directorio actual)
if [ -d "src" ]; then
    SRC_DIR="src"
    INCLUDES="-Iinclude"
else
    SRC_DIR="."
    INCLUDES=""
fi

# Lista de fuentes (coincide con SOURCES del Makefile, incluye ecc_binary.cpp)
SRC_FILES="$SRC_DIR/rng.cpp $SRC_DIR/rsa.cpp $SRC_DIR/ecc.cpp $SRC_DIR/ecc_binary.cpp $SRC_DIR/sha256.cpp $SRC_DIR/main.cpp"

# Conjuntos de flags a comparar (etiqueta:flags)
declare -a FLAG_SETS=(
    "O0:-O0"
    "O1:-O1"
    "O2:-O2"
    "O3:-O3"
    "Ofast:-Ofast"
    "O2-native:-O2 -march=native"
)

mkdir -p "$RESULTS_DIR" "$BIN_DIR"
OUT="$RESULTS_DIR/compiler_flags.csv"
echo "flags,scalar_mult_us,ecdh_us" > "$OUT"

echo "============================================================"
echo "  Comparativa de opciones de compilacion (ECC $CURVE)"
echo "============================================================"
echo "  Iteraciones:    $ITERATIONS"
echo "  Layout fuentes: $SRC_DIR"
echo ""

# Extrae la mediana de una operacion del CSV resumen del binario
extract_median() {
    local csv="$1" op="$2"
    awk -F',' -v op="$op" -v curve="$CURVE" \
        '$1=="ECC" && $2==op && $3==curve { print $7 }' "$csv"
}

for entry in "${FLAG_SETS[@]}"; do
    tag="${entry%%:*}"
    flags="${entry#*:}"
    bin="$BIN_DIR/bench_${tag}"

    echo "[*] Compilando con: $flags"
    # shellcheck disable=SC2086
    g++ $BASE_STD $flags $INCLUDES -o "$bin" $SRC_FILES $LIBS

    echo "    Ejecutando ECC ($CURVE, $ITERATIONS iteraciones)..."
    tmp_summary="$RESULTS_DIR/_tmp_${tag}.csv"
    "./$bin" -a ECC -c "$CURVE" -i "$ITERATIONS" -s "$SEED_MODE" \
        -r "$RESULTS_DIR/_tmp_${tag}_raw.csv" -v \
        > "$tmp_summary" 2>/dev/null

    sm=$(extract_median "$tmp_summary" "scalar_mult")
    dh=$(extract_median "$tmp_summary" "ecdh")
    echo "    scalar_mult=${sm}us  ecdh=${dh}us"
    echo "${flags},${sm},${dh}" >> "$OUT"

    rm -f "$tmp_summary" "$RESULTS_DIR/_tmp_${tag}_raw.csv" "$bin"
    echo ""
done

echo "============================================================"
echo "  Resultados guardados en: $OUT"
echo "============================================================"
cat "$OUT"
echo ""

# Generar la grafica (si esta disponible el script de visualizacion)
SCRIPT_DIR_SELF="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VIZ="$SCRIPT_DIR_SELF/visualize_compiler_flags.py"
if [ -f "$VIZ" ]; then
    echo "Generando grafica..."
    python3 "$VIZ" "$OUT" "$RESULTS_DIR"
fi

echo ""
echo "Nota: -march=native produce un binario no portable (solo ejecutable"
echo "en CPUs con el mismo conjunto de instrucciones que la de compilacion)."