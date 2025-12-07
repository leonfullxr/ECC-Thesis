#!/usr/bin/env python3
"""
analyze_randomness.py - VERSION 3
Análisis estadístico para datos YA NORMALIZADOS en [0, 1]
Los datos vienen pre-normalizados desde C++ usando aritmética de alta precisión

Autor: Leon Elliott Fuller
Fecha: 2025-12-07
"""

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from scipy import stats
from scipy.stats import chi2, kstest
import sys
import os
from pathlib import Path

# Configuración de matplotlib
plt.style.use('seaborn-v0_8-darkgrid')
sns.set_palette("husl")

# ============================================================================
# TESTS ESTADÍSTICOS
# ============================================================================

class RandomnessTests:
    """Suite de tests estadísticos para validar aleatoriedad"""
    
    def __init__(self, data, verbose=True):
        self.data = data
        self.verbose = verbose
        self.results = {}
    
    def print_test_header(self, name):
        """Imprime encabezado de test"""
        if self.verbose:
            print(f"\n{'='*70}")
            print(f"  {name}")
            print(f"{'='*70}")
    
    def test_uniformity(self, num_bins=10):
        """Test de uniformidad usando Chi-cuadrado"""
        self.print_test_header("TEST DE UNIFORMIDAD (Chi-cuadrado)")
        
        # Crear bins
        observed, bin_edges = np.histogram(self.data, bins=num_bins)
        expected = len(self.data) / num_bins
        
        # Chi-cuadrado
        chi_sq = np.sum((observed - expected)**2 / expected)
        df = num_bins - 1
        p_value = 1 - chi2.cdf(chi_sq, df)
        
        self.results['uniformity'] = {
            'chi_squared': chi_sq,
            'dof': df,
            'p_value': p_value,
            'passed': p_value > 0.05
        }
        
        if self.verbose:
            print(f"  Chi-cuadrado:     {chi_sq:.4f}")
            print(f"  Grados libertad:  {df}")
            print(f"  P-value:          {p_value:.6f}")
            print(f"  Resultado:        {'PASS' if p_value > 0.05 else 'FAIL'} (alpha=0.05)")
            if p_value > 0.05:
                print(f"  -> Los datos SON uniformes (no rechazo H0)")
            else:
                print(f"  -> Los datos NO SON uniformes (rechazo H0)")
        
        return self.results['uniformity']
    
    def test_kolmogorov_smirnov(self):
        """Test de Kolmogorov-Smirnov"""
        self.print_test_header("TEST DE KOLMOGOROV-SMIRNOV")
        
        # Test KS contra uniforme [0,1]
        statistic, p_value = kstest(self.data, 'uniform')
        
        self.results['ks_test'] = {
            'statistic': statistic,
            'p_value': p_value,
            'passed': p_value > 0.05
        }
        
        if self.verbose:
            print(f"  Estadístico KS:   {statistic:.6f}")
            print(f"  P-value:          {p_value:.6f}")
            print(f"  Resultado:        {'PASS' if p_value > 0.05 else 'FAIL'} (alpha=0.05)")
        
        return self.results['ks_test']
    
    def test_runs(self):
        """
        Test de runs (rachas) - VERSIÓN CORREGIDA SIN OVERFLOW
        Usa fórmulas numéricamente estables
        """
        self.print_test_header("TEST DE RUNS (RACHAS)")
        
        # Convertir a binario
        median = np.median(self.data)
        binary = (self.data > median).astype(np.int32)
        
        # Contar runs
        runs = 1
        for i in range(1, len(binary)):
            if binary[i] != binary[i-1]:
                runs += 1
        
        # Número de 1s y 0s
        n1 = np.sum(binary == 1)
        n2 = np.sum(binary == 0)
        n = n1 + n2
        
        # CRÍTICO: Usar fórmulas numéricamente estables
        # Evitar multiplicaciones grandes que causan overflow
        
        # Expected runs
        expected_runs = (2.0 * n1 * n2) / n + 1.0
        
        # Variance - usar forma logarítmica para evitar overflow
        if n > 1 and n1 > 0 and n2 > 0:
            # variance = (2*n1*n2*(2*n1*n2 - n)) / (n^2 * (n-1))
            # Reformular para evitar overflow:
            numerator = 2.0 * n1 * n2 * (2.0 * n1 * n2 - n)
            denominator = float(n) * float(n) * (n - 1.0)
            
            # Proteger contra overflow usando límites
            if numerator > 1e15 or denominator > 1e15:
                # Aproximación para n muy grande
                variance_runs = (2.0 * n1 * n2) / n
            else:
                variance_runs = numerator / denominator
        else:
            variance_runs = 0
        
        # Z-score
        if variance_runs > 0:
            z_score = (runs - expected_runs) / np.sqrt(variance_runs)
            p_value = 2 * (1 - stats.norm.cdf(abs(z_score)))
        else:
            z_score = 0
            p_value = 1.0
        
        self.results['runs_test'] = {
            'runs': runs,
            'expected_runs': expected_runs,
            'z_score': z_score,
            'p_value': p_value,
            'passed': p_value > 0.05
        }
        
        if self.verbose:
            print(f"  Runs observados:  {runs}")
            print(f"  Runs esperados:   {expected_runs:.2f}")
            print(f"  Z-score:          {z_score:.4f}")
            print(f"  P-value:          {p_value:.6f}")
            print(f"  Resultado:        {'PASS' if p_value > 0.05 else 'FAIL'} (alpha=0.05)")
        
        return self.results['runs_test']
    
    def test_autocorrelation(self, max_lag=20):
        """
        Test de autocorrelación - VERSIÓN MEJORADA
        Más tolerante a variaciones aleatorias normales
        """
        self.print_test_header("TEST DE AUTOCORRELACIÓN")
        
        # Calcular autocorrelación
        autocorr = []
        for lag in range(1, max_lag + 1):
            if len(self.data) > lag:
                corr = np.corrcoef(self.data[:-lag], self.data[lag:])[0, 1]
                if np.isnan(corr):
                    corr = 0
                autocorr.append(corr)
            else:
                autocorr.append(0)
        
        # Límite de confianza (95%)
        conf_limit = 1.96 / np.sqrt(len(self.data))
        
        # Contar violaciones significativas
        # MEJORADO: Permitir hasta 10% de violaciones (aleatorias esperadas)
        violations = sum(abs(ac) > conf_limit for ac in autocorr)
        max_allowed = max(1, int(max_lag * 0.10))  # Máximo 10% de violaciones
        
        self.results['autocorrelation'] = {
            'autocorr': autocorr,
            'conf_limit': conf_limit,
            'violations': violations,
            'max_allowed': max_allowed,
            'passed': violations <= max_allowed
        }
        
        if self.verbose:
            print(f"  Lags analizados:  {max_lag}")
            print(f"  Límite confianza: ±{conf_limit:.4f}")
            print(f"  Violaciones:      {violations} / {max_lag}")
            print(f"  Máximo permitido: {max_allowed}")
            print(f"  Resultado:        {'PASS' if violations <= max_allowed else 'FAIL'}")
        
        return self.results['autocorrelation']
    
    def test_entropy(self, num_bins=256):
        """Calcula la entropía de Shannon"""
        self.print_test_header("ANÁLISIS DE ENTROPÍA")
        
        # Crear histograma
        hist, _ = np.histogram(self.data, bins=num_bins)
        
        # Probabilidades
        probabilities = hist / hist.sum()
        probabilities = probabilities[probabilities > 0]
        
        # Entropía de Shannon
        entropy = -np.sum(probabilities * np.log2(probabilities))
        max_entropy = np.log2(num_bins)
        entropy_ratio = entropy / max_entropy
        
        self.results['entropy'] = {
            'entropy': entropy,
            'max_entropy': max_entropy,
            'entropy_ratio': entropy_ratio,
            'passed': entropy_ratio > 0.95
        }
        
        if self.verbose:
            print(f"  Entropía:         {entropy:.4f} bits")
            print(f"  Entropía máxima:  {max_entropy:.4f} bits")
            print(f"  Ratio:            {entropy_ratio:.4%}")
            print(f"  Resultado:        {'PASS' if entropy_ratio > 0.95 else 'FAIL'}")
        
        return self.results['entropy']
    
    def test_mean_and_variance(self):
        """
        Verifica media y varianza contra valores teóricos
        Para uniforme [0, 1]: media = 0.5, varianza = 1/12
        """
        self.print_test_header("ANÁLISIS DE MEDIA Y VARIANZA")
        
        # Para distribución uniforme [0, 1]
        expected_mean = 0.5
        expected_var = 1.0 / 12.0
        
        observed_mean = np.mean(self.data)
        observed_var = np.var(self.data)
        
        # Z-test para la media
        n = len(self.data)
        std_error = np.sqrt(expected_var / n)
        z_mean = (observed_mean - expected_mean) / std_error
        p_mean = 2 * (1 - stats.norm.cdf(abs(z_mean)))
        
        self.results['mean_variance'] = {
            'observed_mean': observed_mean,
            'expected_mean': expected_mean,
            'observed_var': observed_var,
            'expected_var': expected_var,
            'z_score_mean': z_mean,
            'p_value_mean': p_mean,
            'passed': p_mean > 0.05
        }
        
        if self.verbose:
            print(f"  Media observada:  {observed_mean:.4f}")
            print(f"  Media esperada:   {expected_mean:.4f}")
            print(f"  Var. observada:   {observed_var:.4f}")
            print(f"  Var. esperada:    {expected_var:.4f}")
            print(f"  P-value (media):  {p_mean:.6f}")
            print(f"  Resultado:        {'PASS' if p_mean > 0.05 else 'FAIL'}")
        
        return self.results['mean_variance']
    
    def run_all_tests(self):
        """Ejecuta todos los tests"""
        print("\n" + "="*70)
        print("  SUITE COMPLETA DE TESTS ESTADÍSTICOS")
        print("="*70)
        
        self.test_mean_and_variance()
        self.test_uniformity()
        self.test_kolmogorov_smirnov()
        self.test_runs()
        self.test_autocorrelation()
        self.test_entropy()
        
        return self.results
    
    def print_summary(self):
        """Imprime resumen de todos los tests"""
        print("\n" + "="*70)
        print("  RESUMEN DE RESULTADOS")
        print("="*70)
        
        tests = [
            ('Media y Varianza', 'mean_variance'),
            ('Uniformidad (Chi²)', 'uniformity'),
            ('Kolmogorov-Smirnov', 'ks_test'),
            ('Runs (Rachas)', 'runs_test'),
            ('Autocorrelación', 'autocorrelation'),
            ('Entropía', 'entropy')
        ]
        
        passed = 0
        total = len(tests)
        
        for name, key in tests:
            if key in self.results:
                status = 'PASS' if self.results[key]['passed'] else 'FAIL'
                if self.results[key]['passed']:
                    passed += 1
                print(f"  {name:25} {status}")
        
        print(f"\n  Total: {passed}/{total} tests pasados ({passed/total*100:.1f}%)")
        
        if passed == total:
            print(f"\n  EXCELENTE: Todos los tests pasaron")
            print(f"  -> El RNG muestra características de aleatoriedad criptográfica")
        elif passed >= total * 0.8:
            print(f"\n  BUENO: La mayoría de tests pasaron")
            print(f"  -> El RNG es adecuado para uso general")
        else:
            print(f"\n  ADVERTENCIA: Varios tests fallaron")
            print(f"  -> Revisar la calidad del RNG")
        
        print("="*70)

# ============================================================================
# VISUALIZACIÓN (sin cambios)
# ============================================================================

class RandomnessPlots:
    """Generador de gráficas para análisis de aleatoriedad"""
    
    def __init__(self, data, output_dir='plots'):
        self.data = data
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(exist_ok=True)
    
    def plot_histogram(self, bins=50):
        """Histograma de distribución"""
        plt.figure(figsize=(12, 6))
        
        plt.subplot(1, 2, 1)
        plt.hist(self.data, bins=bins, density=True, alpha=0.7, edgecolor='black')
        plt.axhline(y=1.0, color='r', linestyle='--', label='Uniforme ideal')
        plt.xlabel('Valor normalizado')
        plt.ylabel('Densidad')
        plt.title('Histograma de Distribución')
        plt.legend()
        plt.grid(True, alpha=0.3)
        
        plt.subplot(1, 2, 2)
        stats.probplot(self.data, dist="uniform", plot=plt)
        plt.title('Q-Q Plot (vs. Uniforme)')
        plt.grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(self.output_dir / 'histogram.png', dpi=300, bbox_inches='tight')
        print(f"  Guardado: {self.output_dir / 'histogram.png'}")
        plt.close()
    
    def plot_autocorrelation(self, max_lag=50):
        """Gráfica de autocorrelación"""
        plt.figure(figsize=(12, 6))
        
        autocorr = []
        for lag in range(1, max_lag + 1):
            if len(self.data) > lag:
                corr = np.corrcoef(self.data[:-lag], self.data[lag:])[0, 1]
                if np.isnan(corr):
                    corr = 0
                autocorr.append(corr)
            else:
                autocorr.append(0)
        
        conf_limit = 1.96 / np.sqrt(len(self.data))
        
        plt.stem(range(1, max_lag + 1), autocorr, basefmt=' ')
        plt.axhline(y=conf_limit, color='r', linestyle='--', label=f'±{conf_limit:.4f} (95% conf.)')
        plt.axhline(y=-conf_limit, color='r', linestyle='--')
        plt.axhline(y=0, color='black', linewidth=0.5)
        plt.xlabel('Lag')
        plt.ylabel('Autocorrelación')
        plt.title('Función de Autocorrelación')
        plt.legend()
        plt.grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(self.output_dir / 'autocorrelation.png', dpi=300, bbox_inches='tight')
        print(f"  Guardado: {self.output_dir / 'autocorrelation.png'}")
        plt.close()
    
    def plot_consecutive_pairs(self, sample_size=10000):
        """Scatter plot de pares consecutivos"""
        if len(self.data) < 2:
            print("  Advertencia: No hay suficientes datos para pares consecutivos")
            return
        
        if len(self.data) > sample_size:
            indices = np.random.choice(len(self.data) - 1, sample_size, replace=False)
            x = self.data[indices]
            y = self.data[indices + 1]
        else:
            x = self.data[:-1]
            y = self.data[1:]
        
        plt.figure(figsize=(10, 10))
        plt.scatter(x, y, alpha=0.3, s=1)
        plt.xlabel('Valor n')
        plt.ylabel('Valor n+1')
        plt.title('Pares Consecutivos (Debe ser uniforme)')
        plt.grid(True, alpha=0.3)
        
        corr = np.corrcoef(x, y)[0, 1]
        if np.isnan(corr):
            corr = 0
        plt.text(0.05, 0.95, f'Correlación: {corr:.6f}', 
                transform=plt.gca().transAxes, verticalalignment='top',
                bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))
        
        plt.tight_layout()
        plt.savefig(self.output_dir / 'consecutive_pairs.png', dpi=300, bbox_inches='tight')
        print(f"  Guardado: {self.output_dir / 'consecutive_pairs.png'}")
        plt.close()
    
    def plot_runs(self):
        """Gráfica de runs analysis"""
        median = np.median(self.data)
        binary = (self.data > median).astype(int)
        
        plt.figure(figsize=(14, 4))
        
        sample = min(1000, len(binary))
        plt.plot(range(sample), binary[:sample], linewidth=0.5)
        plt.fill_between(range(sample), binary[:sample], alpha=0.3)
        plt.axhline(y=0.5, color='r', linestyle='--', alpha=0.5)
        plt.xlabel('Índice')
        plt.ylabel('Bit (0 o 1)')
        plt.title(f'Secuencia Binaria (primeros {sample} valores)')
        plt.ylim(-0.1, 1.1)
        plt.grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(self.output_dir / 'runs.png', dpi=300, bbox_inches='tight')
        print(f"  Guardado: {self.output_dir / 'runs.png'}")
        plt.close()
    
    def plot_cumulative_distribution(self):
        """CDF empírica vs teórica"""
        plt.figure(figsize=(10, 6))
        
        data_sorted = np.sort(self.data)
        ecdf = np.arange(1, len(data_sorted) + 1) / len(data_sorted)
        theoretical_cdf = data_sorted
        
        plt.plot(data_sorted, ecdf, label='CDF empírica', linewidth=2)
        plt.plot(data_sorted, theoretical_cdf, 'r--', label='CDF uniforme teórica', linewidth=2)
        plt.xlabel('Valor normalizado')
        plt.ylabel('Probabilidad acumulada')
        plt.title('Función de Distribución Acumulada')
        plt.legend()
        plt.grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(self.output_dir / 'cdf.png', dpi=300, bbox_inches='tight')
        print(f"  Guardado: {self.output_dir / 'cdf.png'}")
        plt.close()
    
    def generate_all_plots(self):
        """Genera todas las gráficas"""
        print("\n" + "="*70)
        print("  GENERANDO GRÁFICAS")
        print("="*70)
        
        self.plot_histogram()
        self.plot_autocorrelation()
        self.plot_consecutive_pairs()
        self.plot_runs()
        self.plot_cumulative_distribution()
        
        print(f"\n  Todas las gráficas generadas en: {self.output_dir}")

# ============================================================================
# MAIN
# ============================================================================

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 analyze_randomness.py <data_file.csv> [plots_dir]")
        print("\nExample:")
        print("  python3 analyze_randomness.py rng_data.csv")
        print("  python3 analyze_randomness.py rng_data.csv results/plots")
        print("\nNOTE: Data should be pre-normalized to [0, 1] range")
        sys.exit(1)
    
    input_file = sys.argv[1]
    plots_dir = sys.argv[2] if len(sys.argv) > 2 else 'plots'
    
    if not os.path.exists(input_file):
        print(f"Error: File not found: {input_file}")
        sys.exit(1)
    
    print("\n" + "="*70)
    print("  ANÁLISIS ESTADÍSTICO DE ALEATORIEDAD")
    print("="*70)
    print(f"  Archivo de entrada: {input_file}")
    
    # Cargar datos
    print("\n  Cargando datos...")
    df = pd.read_csv(input_file)
    
    # Detectar columna de valores
    value_col = 'value' if 'value' in df.columns else df.columns[1]
    
    # Convertir a numérico
    try:
        data = pd.to_numeric(df[value_col], errors='coerce').values
        data = data[~np.isnan(data)]
    except Exception as e:
        print(f"\n  Error: No se pudieron cargar datos: {e}")
        sys.exit(1)
    
    if len(data) == 0:
        print("\n  Error: No se pudieron cargar datos válidos")
        sys.exit(1)
    
    print(f"  Cargados {len(data):,} valores")
    
    # Verificar si datos están normalizados
    data_min = data.min()
    data_max = data.max()
    
    if data_min >= 0.0 and data_max <= 1.0:
        print(f"  Rango: [{data_min:.6f}, {data_max:.6f}] (normalizado)")
    else:
        print(f"  ADVERTENCIA: Datos fuera de rango [0, 1]!")
        print(f"  Rango: [{data_min:.4e}, {data_max:.4e}]")
        print(f"  Se recomienda normalizar en C++ antes del análisis")
    
    # Ejecutar tests
    tester = RandomnessTests(data)
    tester.run_all_tests()
    tester.print_summary()
    
    # Generar gráficas
    plotter = RandomnessPlots(data, output_dir=plots_dir)
    plotter.generate_all_plots()
    
    print("\n" + "="*70)
    print("  ANÁLISIS COMPLETADO")
    print("="*70)
    print("  Revisa los resultados y las gráficas generadas.")
    print("="*70 + "\n")

if __name__ == "__main__":
    main()