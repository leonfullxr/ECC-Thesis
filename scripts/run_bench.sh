#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<EOF
Uso: $0 [opciones]
  -a ALGO      RSA o ECC          (default: RSA)
  -b BITS      para RSA, tamaño  (default: 2048)
  -c CURVE     para ECC           (default: secp256k1)
  -i ITERS     total iteraciones  (default: 50)
  -m MODE      seq | par          (default: seq)
  -p NCPUS     núm. de CPUs       (default: nproc)
  -s SEED      fixed | random     (default: fixed)
  -h           muestra este help
Ejemplos:
  # 50 iter secuenciales RSA-2048, semillas fijas
  ./run_bench.sh -a RSA -b 2048 -i 50 -m seq -s fixed
  # 100 iter paralelas ECC,secp256k1, semillas aleatorias en 8 CPUs
  ./run_bench.sh -a ECC -c secp256k1 -i 100 -m par -p 8 -s random
EOF
  exit 1
}

# valores por defecto
ALGO="RSA"
BITS=2048
CURVE="secp256k1"
TOTAL_ITERS=50
MODE="seq"
NCPUS=$(nproc)
SEED_MODE="fixed"

# parsear opciones
while getopts "a:b:c:i:m:p:s:h" opt; do
  case $opt in
    a) ALGO=$OPTARG ;;
    b) BITS=$OPTARG ;;
    c) CURVE=$OPTARG ;;
    i) TOTAL_ITERS=$OPTARG ;;
    m) MODE=$OPTARG ;;
    p) NCPUS=$OPTARG ;;
    s) SEED_MODE=$OPTARG ;;
    *) usage ;;
  esac
done

# directorio de resultados
RESULTS_DIR=results
mkdir -p $RESULTS_DIR

# bandera de semilla
SEED_FLAG=""
if [ "$SEED_MODE" = "random" ]; then
  SEED_FLAG="--seed random"
fi

# fichero de salida
if [ "$ALGO" = "RSA" ]; then
  OUTFILE="${RESULTS_DIR}/rsa_${BITS}.csv"
  ALGOPARAM="--algo RSA --bits $BITS"
else
  OUTFILE="${RESULTS_DIR}/ecc_${CURVE}.csv"
  ALGOPARAM="--algo ECC --curve $CURVE"
fi

echo "=== WARM-UP (${ALGO}) — descartando 5 iteraciones ==="
for j in $(seq 1 5); do
  taskset -c $((j%NCPUS)) nice -n19 \
    ./bin/bench $ALGOPARAM --iters 1 $SEED_FLAG \
    >/dev/null
done

echo "=== BENCHMARK (${ALGO}) mode=$MODE iters=$TOTAL_ITERS seed=$SEED_MODE ==="
case "$MODE" in
  seq)
    echo "Secuencial en 1 CPU..."
    taskset -c 0 nice -n19 \
      ./bin/bench $ALGOPARAM --iters $TOTAL_ITERS $SEED_FLAG \
      >> $OUTFILE
    ;;
  par)
    echo "Paralelo en $NCPUS CPUs..."
    # repartir iteraciones
    per=$(( TOTAL_ITERS / NCPUS ))
    rem=$(( TOTAL_ITERS % NCPUS ))
    for cpu in $(seq 0 $((NCPUS-1))); do
      iters=$per
      if [ $cpu -lt $rem ]; then
        iters=$((iters+1))
      fi
      echo "  CPU $cpu → $iters iter"
      taskset -c $cpu nice -n19 \
        ./bin/bench $ALGOPARAM --iters $iters $SEED_FLAG \
        >> $OUTFILE &
    done
    wait
    ;;
  *)
    echo "Modo desconocido: $MODE"; exit 1
    ;;
esac

echo "Resultados en: $OUTFILE"
