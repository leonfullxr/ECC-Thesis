#!/bin/bash
# quick_demo.sh
# Demo rápida del sistema de benchmarking
# Autor: Leon Elliott Fuller

echo ""
echo "╔════════════════════════════════════════════════════════════════════╗"
echo "║                    RSA vs ECC BENCHMARK - DEMO                     ║"
echo "║                   Leon Elliott Fuller - 2025                       ║"
echo "╚════════════════════════════════════════════════════════════════════╝"
echo ""

# Compilar
echo " Compilando proyecto..."
make clean > /dev/null 2>&1
make > /dev/null 2>&1

if [ $? -eq 0 ]; then
    echo " Compilación exitosa"
else
    echo " Error en compilación"
    exit 1
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  DEMO 1: Generación de claves RSA (1024-bit, 5 iteraciones)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

./bin/bench -a RSA -b 1024 -i 5 -s fixed

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  DEMO 2: Comparación de cifrado vs descifrado (con y sin CRT)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

./bin/bench -a RSA -b 2048 -i 5 -s fixed 2>&1 | grep -E "(Encryption|Decryption|Average:)"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  DEMO COMPLETADA"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo " Características demostradas:"
echo "   Generación de claves RSA con diferentes tamaños"
echo "   Cifrado y descifrado RSA"
echo "   Optimización CRT (4x más rápido)"
echo "   Sistema de semillas reproducible"
echo "   Estadísticas detalladas (avg, min, max)"
echo ""
echo " Para más información, consulte:"
echo "   - README.md para documentación completa"
echo "   - ./bin/bench -h para opciones disponibles"
echo "   - make test-rsa para tests predefinidos"
echo ""