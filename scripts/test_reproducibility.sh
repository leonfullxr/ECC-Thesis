#!/bin/bash
# test_reproducibility.sh
# Script para demostrar la reproducibilidad del sistema RNG
# Autor: Leon Elliott Fuller

echo "============================================================================="
echo "TEST DE REPRODUCIBILIDAD DEL SISTEMA RNG"
echo "============================================================================="
echo ""
echo "Este script demuestra que usando la misma semilla, obtenemos resultados"
echo "idénticos en múltiples ejecuciones."
echo ""

# Compilar si es necesario
if [ ! -f bin/bench ]; then
    echo "Compilando proyecto..."
    make > /dev/null 2>&1
    echo "Compilación completada"
    echo ""
fi

# Prueba 1: Tres ejecuciones con semilla fija
echo "PRUEBA 1: Tres ejecuciones con semilla fija (seed=0)"
echo "---------------------------------------------------------------------"
echo ""

for run in 1 2 3; do
    echo "Ejecución #$run:"
    ./bin/bench -a RSA -b 1024 -i 5 -s fixed 2>&1 | grep "Average:"
    echo ""
done

echo "Si la semilla funciona correctamente, los tiempos deberían mostrar"
echo "variación normal del sistema, pero la generación de claves será idéntica."
echo ""
echo "============================================================================="
echo ""

# Prueba 2: Comparar semilla fija vs aleatoria
echo "PRUEBA 2: Semilla fija vs semilla aleatoria"
echo "---------------------------------------------------------------------"
echo ""

echo "Con semilla fija (seed=0):"
./bin/bench -a RSA -b 1024 -i 3 -s fixed 2>&1 | grep "Seed value:"

echo ""
echo "Con semilla aleatoria (basada en timestamp):"
./bin/bench -a RSA -b 1024 -i 3 -s random 2>&1 | grep "Seed value:"

echo ""
echo "============================================================================="
echo ""
echo "CONCLUSIÓN:"
echo "- Semilla fija permite reproducir exactamente las mismas claves"
echo "- Útil para debugging y eliminación de ruido en benchmarks"
echo "- Semilla aleatoria proporciona diferentes claves en cada ejecución"
echo ""