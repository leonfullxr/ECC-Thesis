#!/usr/bin/env bash
# compare_compiler_flags.sh
# Mide el rendimiento de ECC (secp256k1) compilando el motor de benchmarking
# con distintos conjuntos de opciones de optimizacion de g++.
#
# Uso:
#   bash scripts/compare_compiler_flags.sh [ITERACIONES]
# Ejemplo:
#   bash scripts/compare_compiler_flags.sh 100
#
# Genera: results/compiler_flags.csv  con una fila por conjunto de flags.
#
# Autor: Leon Elliott Fuller
# Date: 2026-05-28
set -euo pipefail

# ============================================================================
# CONFIGURACION  (mantener en sincronia con run_benchmarks.sh)
# ============================================================================

ITERATIONS=${1:-100}
SEED_MODE="fixed"
CURVE="secp256k1"
RESULTS_DIR="results"
BIN_DIR="bin"
SRC_DIR="src" 

SRC_FILES="$SRC_DIR/main.cpp $SRC_DIR/rng.cpp $SRC_DIR/rsa.cpp $SRC_DIR/ecc.cpp $SRC_DIR/ecc_binary.cpp $SRC_DIR/sha256.cpp"
LIBS="-lntl -lgmp -lpthread"
BASE_STD="-std=c++17"

# NUEVO: Le decimos a g++ que busque los .hpp en la carpeta include
INCLUDES="-Iinclude"

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
echo "  Iteraciones: $ITERATIONS"
echo ""

# Funcion auxiliar: extrae la mediana de una operacion del CSV verboso del binario.
# El binario (-a ECC -v) imprime el resumen CSV por stdout; usamos -r para el raw.
# Aqui parseamos la salida resumen: columnas operation y median_us.
extract_median() {
    local csv="$1" op="$2"
    # formato: algorithm,operation,params,security_bits,iterations,avg_us,median_us,...
    awk -F',' -v op="$op" -v curve="$CURVE" \
        '$1=="ECC" && $2==op && $3==curve { print $7 }' "$csv"
}

for entry in "${FLAG_SETS[@]}"; do
    tag="${entry%%:*}"
    flags="${entry#*:}"
    bin="$BIN_DIR/bench_${tag}"

    echo "[*] Compilando con: $flags"
    # shellcheck disable=SC2086
    g++ $BASE_STD $INCLUDES $flags $SRC_FILES -o "$bin" $LIBS

    echo "    Ejecutando ECC ($CURVE, $ITERATIONS iteraciones)..."
    tmp_summary="$RESULTS_DIR/_tmp_${tag}.csv"
    # -a ECC: curvas primas en coordenadas afines; -c fija la curva
    "./$bin" -a ECC -c "$CURVE" -i "$ITERATIONS" -s "$SEED_MODE" \
        -r "$RESULTS_DIR/_tmp_${tag}_raw.csv" -v \
        > "$tmp_summary" 2>/dev/null

    sm=$(extract_median "$tmp_summary" "scalar_mult")
    dh=$(extract_median "$tmp_summary" "ecdh")
    echo "    scalar_mult=${sm}us  ecdh=${dh}us"
    echo "${flags},${sm},${dh}" >> "$OUT"

    rm -f "$tmp_summary" "$RESULTS_DIR/_tmp_${tag}_raw.csv"
    echo ""
done

echo "============================================================"
echo "  Resultados guardados en: $OUT"
echo "============================================================"
cat "$OUT"
echo ""
echo "Nota: -march=native produce un binario no portable (solo ejecutable"
echo "en CPUs con el mismo conjunto de instrucciones que la de compilacion)."