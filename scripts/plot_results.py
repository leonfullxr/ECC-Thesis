#!/usr/bin/env python3
"""
plot_results.py
Visualización de resultados de benchmarks RSA vs ECC

Autor: Leon Elliott Fuller
Fecha: 2025-01-06
"""

import matplotlib.pyplot as plt
import re
import sys
from pathlib import Path

def parse_benchmark_file(filepath):
    """Parse un archivo de benchmark y extrae los tiempos"""
    results = {}
    
    with open(filepath, 'r') as f:
        content = f.read()
    
    # Buscar operaciones y tiempos
    pattern = r'=== (.+?) ===\s+.*?Average:\s+(\d+)'
    matches = re.findall(pattern, content, re.DOTALL)
    
    for operation, time_us in matches:
        results[operation] = int(time_us) / 1000  # Convertir a ms
    
    return results

def plot_comparison(rsa_file, ecc_file, output_file):
    """Crea gráfica comparativa RSA vs ECC"""
    rsa_data = parse_benchmark_file(rsa_file)
    ecc_data = parse_benchmark_file(ecc_file)
    
    # Crear figura
    fig, ax = plt.subplots(figsize=(12, 6))
    
    # Preparar datos
    operations = []
    rsa_times = []
    ecc_times = []
    
    # Mapeo de operaciones
    op_map = {
        'Key Generation': ['RSA Key Generation', 'ECC Key Generation'],
        'Sign/Encrypt': ['RSA Encryption', 'ECDSA Sign'],
        'Verify/Decrypt': ['RSA Decryption', 'ECDSA Verify'],
    }
    
    for op_label, [rsa_op, ecc_op] in op_map.items():
        # Buscar operación más cercana
        rsa_time = None
        ecc_time = None
        
        for key in rsa_data:
            if rsa_op in key:
                rsa_time = rsa_data[key]
                break
        
        for key in ecc_data:
            if ecc_op in key:
                ecc_time = ecc_data[key]
                break
        
        if rsa_time and ecc_time:
            operations.append(op_label)
            rsa_times.append(rsa_time)
            ecc_times.append(ecc_time)
    
    # Crear gráfica de barras
    x = range(len(operations))
    width = 0.35
    
    bars1 = ax.bar([i - width/2 for i in x], rsa_times, width, 
                   label='RSA', color='#e74c3c', alpha=0.8)
    bars2 = ax.bar([i + width/2 for i in x], ecc_times, width,
                   label='ECC', color='#3498db', alpha=0.8)
    
    # Añadir speedup encima de las barras
    for i in range(len(operations)):
        if ecc_times[i] > 0:
            speedup = rsa_times[i] / ecc_times[i]
            ax.text(i, max(rsa_times[i], ecc_times[i]) * 1.05,
                   f'{speedup:.1f}x',
                   ha='center', va='bottom', fontweight='bold')
    
    # Configuración
    ax.set_ylabel('Tiempo (ms)', fontsize=12)
    ax.set_title('Comparación de Rendimiento: RSA vs ECC', 
                fontsize=14, fontweight='bold')
    ax.set_xticks(x)
    ax.set_xticklabels(operations, fontsize=11)
    ax.legend(fontsize=11)
    ax.grid(axis='y', alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"Gráfica guardada en: {output_file}")
    
    return operations, rsa_times, ecc_times

def plot_keygen_comparison(results_dir):
    """Compara generación de claves para diferentes tamaños"""
    results_dir = Path(results_dir)

    # RSA: comp1-3 with rsa bit size
    rsa_files = sorted(results_dir.glob('comp*_rsa[0-9]*.txt'))

    # ECC: comp1-3 with ecc_p256 or ecc_p384
    ecc_files = sorted(results_dir.glob('comp*_ecc_p[0-9]*.txt'))
    
    if not rsa_files or not ecc_files:
        print("No se encontraron archivos de resultados")
        return
    
    # Extraer datos de generación de claves
    rsa_keygen = {}
    ecc_keygen = {}
    
    for f in rsa_files:
        data = parse_benchmark_file(f)
        for op, time in data.items():
            if 'Key Generation' in op:
                # Extraer bits
                match = re.search(r'(\d+)-bit', op)
                if match:
                    bits = int(match.group(1))
                    rsa_keygen[bits] = time
    
    for f in ecc_files:
        data = parse_benchmark_file(f)
        for op, time in data.items():
            if 'Key Generation' in op:
                # Extraer nombre de curva
                if 'P-256' in op or 'secp256k1' in op:
                    ecc_keygen[256] = time
                elif 'P-384' in op:
                    ecc_keygen[384] = time
    
    # Crear gráfica
    fig, ax = plt.subplots(figsize=(10, 6))
    
    rsa_bits = sorted(rsa_keygen.keys())
    rsa_times = [rsa_keygen[b] for b in rsa_bits]
    
    ecc_bits = sorted(ecc_keygen.keys())
    ecc_times = [ecc_keygen[b] for b in ecc_bits]
    
    ax.plot(rsa_bits, rsa_times, 'o-', label='RSA', 
           color='#e74c3c', linewidth=2, markersize=8)
    ax.plot(ecc_bits, ecc_times, 's-', label='ECC', 
           color='#3498db', linewidth=2, markersize=8)
    
    ax.set_xlabel('Tamaño de clave (bits)', fontsize=12)
    ax.set_ylabel('Tiempo de generación (ms)', fontsize=12)
    ax.set_title('Generación de Claves: RSA vs ECC', 
                fontsize=14, fontweight='bold')
    ax.legend(fontsize=11)
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    output = results_dir / 'keygen_comparison.png'
    plt.savefig(output, dpi=300, bbox_inches='tight')
    print(f"Gráfica guardada en: {output}")

def main():
    if len(sys.argv) < 2:
        print("Uso:")
        print("  python3 plot_results.py <directorio_resultados>")
        print("  python3 plot_results.py <archivo_rsa> <archivo_ecc> <output.png>")
        sys.exit(1)
    
    if len(sys.argv) == 2:
        # Modo: procesar directorio completo
        results_dir = Path(sys.argv[1])
        if not results_dir.exists():
            print(f"Error: {results_dir} no existe")
            sys.exit(1)
        
        print(f"Procesando resultados en: {results_dir}")
        plot_keygen_comparison(results_dir)
        
    elif len(sys.argv) == 4:
        # Modo: comparar dos archivos específicos
        rsa_file = Path(sys.argv[1])
        ecc_file = Path(sys.argv[2])
        output_file = Path(sys.argv[3])
        
        if not rsa_file.exists() or not ecc_file.exists():
            print("Error: Archivos no encontrados")
            sys.exit(1)
        
        plot_comparison(rsa_file, ecc_file, output_file)

if __name__ == '__main__':
    main()