#!/usr/bin/env bash
# run_benchmarks.sh
# Master script for RSA vs ECC comparative benchmarking
# Compiles the project, runs all benchmarks, generates visualizations
# Author: Leon Elliott Fuller
# Date: 2026-02-28
set -euo pipefail

# ============================================================================
# CONFIGURATION
# ============================================================================

ITERATIONS=${1:-10}
SEED_MODE="fixed"
RESULTS_DIR="results"
BIN="./bin/bench"
SRC_FILES="main.cpp rng.cpp rsa.cpp ecc.cpp sha256.cpp"
CXX_FLAGS="-std=c++17 -O2"
LIBS="-lntl -lgmp -lpthread"

# ============================================================================
# SETUP
# ============================================================================

echo "============================================================"
echo "  RSA vs ECC Comparative Benchmark Suite"
echo "============================================================"
echo ""
echo "  Iterations per test:  $ITERATIONS"
echo "  Seed mode:            $SEED_MODE"
echo "  Results directory:    $RESULTS_DIR"
echo ""

mkdir -p "$RESULTS_DIR" bin

# ============================================================================
# COMPILE
# ============================================================================

echo "[1/4] Compiling benchmark engine..."

NEEDS_COMPILE=0
if [ ! -f "$BIN" ]; then
    NEEDS_COMPILE=1
else
    for src in $SRC_FILES; do
        if [ "$src" -nt "$BIN" ] 2>/dev/null; then
            NEEDS_COMPILE=1
            break
        fi
    done
fi

if [ "$NEEDS_COMPILE" -eq 1 ]; then
    g++ $CXX_FLAGS -o "$BIN" $SRC_FILES $LIBS
    echo "  Compiled successfully."
else
    echo "  Binary up to date, skipping."
fi
echo ""

# ============================================================================
# RUN BENCHMARKS
# ============================================================================

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
SUMMARY="$RESULTS_DIR/summary_${TIMESTAMP}.csv"
RAW="$RESULTS_DIR/raw_${TIMESTAMP}.csv"
LOG="$RESULTS_DIR/log_${TIMESTAMP}.txt"

echo "[2/4] Running full comparison benchmark..."
echo "  This may take several minutes depending on iterations."
echo ""

# Run benchmark: CSV to stdout (file), verbose progress to stderr (log)
$BIN -a CMP -i "$ITERATIONS" -s "$SEED_MODE" -r "$RAW" -v \
    > "$SUMMARY" \
    2>"$LOG"

# Show progress from log
cat "$LOG"

echo ""
echo "  Summary CSV:  $SUMMARY ($(wc -l < "$SUMMARY") rows)"
echo "  Raw CSV:      $RAW ($(wc -l < "$RAW") rows)"
echo "  Log:          $LOG"
echo ""

# Create stable symlinks for easy access
ln -sf "summary_${TIMESTAMP}.csv" "$RESULTS_DIR/summary_latest.csv"
ln -sf "raw_${TIMESTAMP}.csv" "$RESULTS_DIR/raw_latest.csv"

# ============================================================================
# VERIFY DATA
# ============================================================================

echo "[3/4] Verifying benchmark data..."
ROW_COUNT=$(tail -n +2 "$SUMMARY" | wc -l)
echo "  $ROW_COUNT benchmark results collected."

# Quick sanity check
RSA_COUNT=$(grep -c "^RSA," "$SUMMARY" || true)
ECC_COUNT=$(grep -c "^ECC," "$SUMMARY" || true)
echo "  RSA benchmarks: $RSA_COUNT"
echo "  ECC benchmarks: $ECC_COUNT"
echo ""

# ============================================================================
# GENERATE VISUALIZATIONS
# ============================================================================

echo "[4/4] Generating visualizations..."

if command -v python3 &> /dev/null; then
    python3 scripts/visualize_benchmarks.py "$SUMMARY" "$RAW" "$RESULTS_DIR"
    echo ""
    CHART_COUNT=$(ls "$RESULTS_DIR"/chart_*.png 2>/dev/null | wc -l)
    echo "  $CHART_COUNT charts generated."
else
    echo "  WARNING: python3 not found. Skipping visualizations."
    echo "  Install: pip install pandas matplotlib numpy"
    echo "  Run manually: python3 scripts/visualize_benchmarks.py $SUMMARY $RAW $RESULTS_DIR"
fi

echo ""
echo "============================================================"
echo "  Benchmark complete!"
echo "============================================================"
echo ""
echo "  Output files:"
echo "    $SUMMARY"
echo "    $RAW"
echo "    $LOG"
ls "$RESULTS_DIR"/chart_*.png 2>/dev/null | sed 's/^/    /' || true
echo ""
echo "  Quick access (symlinks):"
echo "    $RESULTS_DIR/summary_latest.csv"
echo "    $RESULTS_DIR/raw_latest.csv"
echo ""