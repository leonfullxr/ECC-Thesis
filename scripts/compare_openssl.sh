#!/usr/bin/env bash
# compare_openssl.sh
# Compares our RSA/ECC implementation against OpenSSL's optimized routines.
# Produces a CSV file with OpenSSL operation timings for reference.
# Author: Leon Elliott Fuller
# Date: 2026-02-28
#
# Dependencies: openssl (no bc required, uses awk for math)
set -euo pipefail

RESULTS_DIR="${1:-results}"
DURATION="${2:-10}"

mkdir -p "$RESULTS_DIR"

echo "============================================================"
echo "  OpenSSL Baseline Comparison"
echo "============================================================"
echo ""
echo "  OpenSSL version: $(openssl version)"
echo "  Test duration:   ${DURATION}s per operation"
echo ""

OUTFILE="$RESULTS_DIR/openssl_baseline.csv"
echo "algorithm,operation,params,ops_per_sec,us_per_op" > "$OUTFILE"

# Helper: convert ops/sec to us/op using awk (no bc dependency)
us_from_rate() {
    echo "$1" | awk '{printf "%.1f", 1000000.0 / $1}'
}

# ============================================================================
# RSA Sign/Verify
# ============================================================================

echo "[RSA] Running OpenSSL speed tests..."

for bits in 1024 2048 3072 4096; do
    line=$(openssl speed -seconds "$DURATION" "rsa${bits}" 2>/dev/null \
        | grep "rsa ${bits} bits" | tail -1)

    if [ -n "$line" ]; then
        sign_rate=$(echo "$line" | awk '{print $(NF-1)}')
        verify_rate=$(echo "$line" | awk '{print $NF}')
        sign_us=$(us_from_rate "$sign_rate")
        verify_us=$(us_from_rate "$verify_rate")

        printf "  RSA-%-4s  sign=%7sus  verify=%6sus  (%s sign/s, %s verify/s)\n" \
            "$bits" "$sign_us" "$verify_us" "$sign_rate" "$verify_rate"
        echo "RSA,sign,${bits}-bit,${sign_rate},${sign_us}" >> "$OUTFILE"
        echo "RSA,verify,${bits}-bit,${verify_rate},${verify_us}" >> "$OUTFILE"
    else
        echo "  RSA-${bits}: SKIPPED (not available)"
    fi
done
echo ""

# ============================================================================
# ECDSA Sign/Verify
# ============================================================================

echo "[ECDSA] Running OpenSSL speed tests..."

# P-256
line=$(openssl speed -seconds "$DURATION" ecdsap256 2>/dev/null \
    | grep "256 bits ecdsa" | tail -1)
if [ -n "$line" ]; then
    sign_rate=$(echo "$line" | awk '{print $(NF-1)}')
    verify_rate=$(echo "$line" | awk '{print $NF}')
    sign_us=$(us_from_rate "$sign_rate")
    verify_us=$(us_from_rate "$verify_rate")
    printf "  P-256     sign=%7sus  verify=%6sus  (%s sign/s, %s verify/s)\n" \
        "$sign_us" "$verify_us" "$sign_rate" "$verify_rate"
    echo "ECC,sign,P-256,${sign_rate},${sign_us}" >> "$OUTFILE"
    echo "ECC,verify,P-256,${verify_rate},${verify_us}" >> "$OUTFILE"
else
    echo "  P-256:    SKIPPED"
fi

# P-384
line=$(openssl speed -seconds "$DURATION" ecdsap384 2>/dev/null \
    | grep "384 bits ecdsa" | tail -1)
if [ -n "$line" ]; then
    sign_rate=$(echo "$line" | awk '{print $(NF-1)}')
    verify_rate=$(echo "$line" | awk '{print $NF}')
    sign_us=$(us_from_rate "$sign_rate")
    verify_us=$(us_from_rate "$verify_rate")
    printf "  P-384     sign=%7sus  verify=%6sus  (%s sign/s, %s verify/s)\n" \
        "$sign_us" "$verify_us" "$sign_rate" "$verify_rate"
    echo "ECC,sign,P-384,${sign_rate},${sign_us}" >> "$OUTFILE"
    echo "ECC,verify,P-384,${verify_rate},${verify_us}" >> "$OUTFILE"
else
    echo "  P-384:    SKIPPED"
fi
echo ""

# ============================================================================
# ECDH
# ============================================================================

echo "[ECDH] Running OpenSSL speed tests..."

# ECDH P-256
line=$(openssl speed -seconds "$DURATION" ecdhp256 2>/dev/null \
    | grep "256 bits ecdh\|256-bits ECDH" | tail -1)
if [ -n "$line" ]; then
    rate=$(echo "$line" | awk '{print $NF}')
    us_per=$(us_from_rate "$rate")
    printf "  P-256     ecdh=%7sus  (%s ops/s)\n" "$us_per" "$rate"
    echo "ECC,ecdh,P-256,${rate},${us_per}" >> "$OUTFILE"
else
    echo "  P-256:    SKIPPED"
fi

# ECDH P-384
line=$(openssl speed -seconds "$DURATION" ecdhp384 2>/dev/null \
    | grep "384 bits ecdh\|384-bits ECDH" | tail -1)
if [ -n "$line" ]; then
    rate=$(echo "$line" | awk '{print $NF}')
    us_per=$(us_from_rate "$rate")
    printf "  P-384     ecdh=%7sus  (%s ops/s)\n" "$us_per" "$rate"
    echo "ECC,ecdh,P-384,${rate},${us_per}" >> "$OUTFILE"
else
    echo "  P-384:    SKIPPED"
fi

echo ""
echo "Results saved to: $OUTFILE"
echo ""
echo "============================================================"
echo "  Notes"
echo "============================================================"
echo ""
echo "  OpenSSL uses assembly-optimized code, constant-time"
echo "  implementations, and platform-specific SIMD instructions."
echo "  Our implementation is a pure C++/NTL educational baseline."
echo ""
echo "  Expected performance gaps:"
echo "    RSA:  1-2x  (both use GMP for modular exponentiation)"
echo "    ECC:  5-50x (OpenSSL uses hand-tuned asm for P-256)"
echo ""
echo "  To compare against our implementation, run both on the"
echo "  same machine and compare the CSV outputs:"
echo ""
echo "    bash scripts/run_benchmarks.sh 20"
echo "    bash scripts/compare_openssl.sh results 10"
echo ""