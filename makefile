.ONESHELL:

################################################
#
# MAKEFILE VARIABLES
#
# RSA vs ECC Benchmark Makefile
# Proyecto: Curvas Elipticas y Criptografia
# Autor: Leon Elliott Fuller
# Fecha: 2026-05-29
######################### Compilation
CXX      := g++
PREFIX   ?= /usr/local
# Nivel de optimizacion. -O2 es el estandar del TFG: las cifras de la memoria
# se midieron asi (sin -march=native), por portabilidad y reproducibilidad.
# Sobreescribible, p.ej. para un binario local mas rapido: make OPT="-O2 -march=native".
OPT      ?= -O2
CXXFLAGS := -std=c++17 $(OPT)
# Include paths are kept in a separate variable so that overriding CXXFLAGS
# on the command line (e.g. for sanitizer builds) does not lose -Iinclude.
INCLUDES := -Iinclude \
            -I$(PREFIX)/include
LDFLAGS  := -L$(PREFIX)/lib \
            -Wl,-rpath=$(PREFIX)/lib
LDLIBS   := -lntl -lgmp -pthread

######################### Python benchmark
PYTHON        := python3
PYTHON_SCRIPT := rsa_benchmark.py

######################### Directories
SRC_DIR   := src
BUILD_DIR := build
BIN_DIR   := bin
PYTHON_DIR := scripts
SCRIPTS_DIR := scripts
INCLUDE_DIR := include
RESULTS_DIR := results
IMAGES_DIR := docs/Plantilla_TFG_latex/imagenes
SLIDES_DIR := docs/presentacion_defensa
SLIDES_IMAGES := $(SLIDES_DIR)/imagenes

######################### Source and object files
SOURCES := $(SRC_DIR)/rng.cpp $(SRC_DIR)/rsa.cpp $(SRC_DIR)/ecc.cpp $(SRC_DIR)/ecc_binary.cpp  $(SRC_DIR)/sha256.cpp $(SRC_DIR)/main.cpp
OBJS    := $(BUILD_DIR)/rng.o $(BUILD_DIR)/rsa.o $(BUILD_DIR)/ecc.o $(BUILD_DIR)/ecc_binary.o $(BUILD_DIR)/sha256.o $(BUILD_DIR)/main.o

######################### Parameters override
KEY_SIZE ?= 2048 # RSA key size for test-rsa target
ITERS ?= 1 # Iterations for test targets
# Iteraciones para el benchmark completo (memoria)
BENCH_ITERS ?= 100
# Segundos por operacion en la comparativa OpenSSL
OPENSSL_SECONDS ?= 50
# Iteraciones para la comparativa de flags de compilador
FLAG_ITERS ?= 100
RANDOM_SEED := $(shell bash -c "awk 'BEGIN{srand();print( int(65536*rand()) )}'")
RR := ./
SEED := $(RANDOM_SEED)

######################### Colors
PBG := \e[1;33m   # Preamble color
IFG := \e[1;33m   # Indexes color
LGFG := \e[1;32m  # Success messages
LRFG := \e[1;31m  # Error messages
EC  := \e[0m      # End color

all: preamble show-info $(BIN_DIR)/bench ${BIN_DIR}/rng_analysis

.DEFAULT_GOAL := all

preamble:
	@echo "$(PBG)***********************************************$(EC)"
	@echo "$(PBG)*$(EC)         Final Thesis Project       $(PBG)*$(EC)"
	@echo "$(PBG)*$(EC)           Year 2024-2026           $(PBG)*$(EC)"
	@echo "$(PBG)*---------------------------------------------*$(EC)"
	@echo "$(PBG)*$(EC)        ECC and RSA Benchmarks      $(PBG)*$(EC)"
	@echo "$(PBG)*---------------------------------------------*$(EC)"
	@echo "$(PBG)*$(EC)     Author: Leon Elliott Fuller    $(PBG)*$(EC)"
	@echo "$(PBG)***********************************************$(EC)"

show-info:
	@echo  "\n$(IFG)Available targets:                            $(EC)"
	@echo ""
	@echo "$(IFG)  Build:$(EC)"
	@echo "$(IFG) -  make$(EC)                    Build bench + rng_analysis"
	@echo "$(IFG) -  make clean$(EC)              Clean build artifacts"
	@echo ""
	@echo "$(IFG)  Quick tests:$(EC)"
	@echo "$(IFG) -  make test-rsa$(EC)           RSA benchmark ($(KEY_SIZE)-bit, $(ITERS) iter)"
	@echo "$(IFG) -  make test-rsa-2k$(EC)        RSA benchmark (2048-bit, $(ITERS) iter)"
	@echo "$(IFG) -  make test-ecc$(EC)           ECC benchmark ($(ITERS) iter)"
	@echo "$(IFG) -  make test-rsa-py$(EC)        RSA Python benchmark"
	@echo ""
	@echo "$(IFG)  Full experiments (memoria):$(EC)"
	@echo "$(IFG) -  make benchmark$(EC)          Full RSA vs ECC suite ($(BENCH_ITERS) iter) + charts"
	@echo "$(IFG) -  make compare-openssl$(EC)    OpenSSL baseline ($(OPENSSL_SECONDS)s/op)"
	@echo "$(IFG) -  make compare-compiler-flags$(EC)  ECC vs compiler flags + chart"
	@echo "$(IFG) -  make rng-full$(EC)           Full RNG statistical analysis"
	@echo "$(IFG) -  make rng-visualize$(EC)      RNG visualizations only -> results/plots/"
	@echo ""
	@echo "$(IFG)  Figures / reproducibility:$(EC)"
	@echo "$(IFG) -  make visualize$(EC)          Regenerate all charts + collages"
	@echo "$(IFG) -  make figures$(EC)            Regenerate charts and copy to imagenes/ (memoria + slides)"
	@echo "$(IFG) -  make illustrations$(EC)      Regenerate static slide figures (curve shapes, group law)"
	@echo "$(IFG) -  make slides$(EC)             Compile the Beamer defense presentation (PDF)"
	@echo "$(IFG) -  make reproduce$(EC)          Full pipeline: benchmark+openssl+flags+figures"
	@echo ""
	@echo "$(IFG)  Variables: OPT, BENCH_ITERS, OPENSSL_SECONDS, FLAG_ITERS, KEY_SIZE, ITERS$(EC)"
	@echo "$(IFG)  Ej.: make benchmark OPT=-O2 BENCH_ITERS=100$(EC)"
	@echo ""

# Create directories if they don't exist
$(BUILD_DIR) $(BIN_DIR):
	@mkdir -p $@

# Pattern rule for object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo -e "$(LGFG)Compiling $<...$(EC)"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Main executable
$(BIN_DIR)/bench: $(OBJS) | $(BIN_DIR)
	@echo -e "$(LGFG)Linking bench...$(EC)"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $^ $(LDFLAGS) $(LDLIBS) -o $@

# RNG analysis tool
$(BIN_DIR)/rng_analysis: $(SRC_DIR)/rng_analysis.cpp $(BUILD_DIR)/rng.o | $(BIN_DIR)
	@echo -e "$(LGFG)Compiling RNG analysis tool...$(EC)"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $^ $(LDFLAGS) $(LDLIBS) -o $@

# Dependencies (explicit)
$(BUILD_DIR)/main.o: $(SRC_DIR)/main.cpp $(INCLUDE_DIR)/common.hpp $(INCLUDE_DIR)/rng.hpp $(INCLUDE_DIR)/rsa.hpp $(INCLUDE_DIR)/ecc.hpp
$(BUILD_DIR)/rsa.o: $(SRC_DIR)/rsa.cpp $(INCLUDE_DIR)/rsa.hpp $(INCLUDE_DIR)/common.hpp $(INCLUDE_DIR)/rng.hpp
$(BUILD_DIR)/ecc.o: $(SRC_DIR)/ecc.cpp $(INCLUDE_DIR)/ecc.hpp $(INCLUDE_DIR)/common.hpp $(INCLUDE_DIR)/rng.hpp
$(BUILD_DIR)/rng.o: $(SRC_DIR)/rng.cpp $(INCLUDE_DIR)/rng.hpp $(INCLUDE_DIR)/common.hpp

# Analysis targets
.PHONY: rng-analysis analyze-rng

rng-analysis: $(BIN_DIR)/rng_analysis
	@echo -e "$(LGFG)RNG analysis tool ready$(EC)"

analyze-rng: $(BIN_DIR)/rng_analysis
	@echo -e "$(LGFG)Running RNG analysis...$(EC)"
	@mkdir -p data plots
	@$(BIN_DIR)/rng_analysis -n 1000000 -r 1000 -s fixed -o data/rng_data.csv -v
	@python3 scripts/analyze_randomness.py data/rng_data.csv

# Test targets
test-rsa: $(BIN_DIR)/bench
	@echo -e "$(LGFG)Executing RSA ($(KEY_SIZE)-bit, $(ITERS) iteration(s))...$(EC)"
	@$(BIN_DIR)/bench -a RSA -b $(KEY_SIZE) -i $(ITERS) -s fixed

test-rsa-2k: $(BIN_DIR)/bench
	@echo -e "$(LGFG)Executing RSA (2048-bit, $(ITERS) iteration(s))...$(EC)"
	@$(BIN_DIR)/bench -a RSA -b 2048 -i $(ITERS) -s fixed

test-ecc: $(BIN_DIR)/bench
	@echo -e "$(LGFG)Executing ECC ($(ITERS) iteration(s))...$(EC)"
	@$(BIN_DIR)/bench -a ECC -i $(ITERS) -s fixed

# Python benchmark target
.PHONY: test-rsa-py
test-rsa-py:
	@echo -e "$(LGFG)Executing RSA Python benchmark ($(KEY_SIZE)-bit, $(ITERS) iteration(s), seed=$(SEED))...$(EC)"
	@$(PYTHON) $(PYTHON_DIR)/$(PYTHON_SCRIPT) -k $(KEY_SIZE) -i $(ITERS) -s $(SEED)

# ============================================================================
# Full experiments and reproducibility (memoria)
# ============================================================================
.PHONY: benchmark compare-openssl compare-compiler-flags rng-full rng-visualize visualize figures illustrations slides reproduce

# Full RSA vs ECC comparative benchmark (3 axes) + charts
benchmark: $(BIN_DIR)/bench
	@mkdir -p $(RESULTS_DIR)
	@echo -e "$(LGFG)Running full CMP benchmark ($(BENCH_ITERS) iterations)...$(EC)"
	@$(BIN_DIR)/bench -a CMP -i $(BENCH_ITERS) -s fixed -r $(RESULTS_DIR)/raw.csv -v > $(RESULTS_DIR)/summary.csv
	@cp -f $(RESULTS_DIR)/summary.csv $(RESULTS_DIR)/summary_latest.csv
	@cp -f $(RESULTS_DIR)/raw.csv $(RESULTS_DIR)/raw_latest.csv
	@echo -e "$(LGFG)Generating charts...$(EC)"
	@$(PYTHON) $(SCRIPTS_DIR)/visualize_benchmarks.py $(RESULTS_DIR)/summary_latest.csv $(RESULTS_DIR)/raw_latest.csv $(RESULTS_DIR) || true
	@$(PYTHON) $(SCRIPTS_DIR)/visualize_summary.py $(RESULTS_DIR)/summary_latest.csv $(RESULTS_DIR)
	@$(PYTHON) $(SCRIPTS_DIR)/create_collages.py $(RESULTS_DIR) $(RESULTS_DIR) || true
	@echo -e "$(LGFG)Benchmark complete. Results in $(RESULTS_DIR)/$(EC)"

# OpenSSL baseline comparison
compare-openssl:
	@echo -e "$(LGFG)Running OpenSSL baseline ($(OPENSSL_SECONDS)s per operation)...$(EC)"
	@bash $(SCRIPTS_DIR)/compare_openssl.sh $(RESULTS_DIR) $(OPENSSL_SECONDS)

# ECC performance vs compiler optimization flags (+ chart)
compare-compiler-flags:
	@echo -e "$(LGFG)Comparing compiler flags ($(FLAG_ITERS) iterations)...$(EC)"
	@bash $(SCRIPTS_DIR)/compare_compiler_flags.sh $(FLAG_ITERS)

# Full RNG statistical analysis pipeline
rng-full:
	@echo -e "$(LGFG)Running full RNG analysis...$(EC)"
	@bash $(SCRIPTS_DIR)/run_rng_analysis.sh

# RNG visualizations only: one dataset -> plots in results/plots/
# (lighter than rng-full: no multi-seed sweep, no text report)
rng-visualize: $(BIN_DIR)/rng_analysis
	@echo -e "$(LGFG)Generating RNG visualizations...$(EC)"
	@mkdir -p $(RESULTS_DIR)/data $(RESULTS_DIR)/plots
	@$(BIN_DIR)/rng_analysis -n 1000000 -r 1000 -s fixed -o $(RESULTS_DIR)/data/bounded.csv -v
	@$(PYTHON) $(SCRIPTS_DIR)/analyze_randomness.py $(RESULTS_DIR)/data/bounded.csv $(RESULTS_DIR)/plots
	@echo -e "$(LGFG)RNG plots in $(RESULTS_DIR)/plots/$(EC)"

# Regenerate all charts and collages from the latest results
visualize:
	@echo -e "$(LGFG)Regenerating charts and collages...$(EC)"
	@$(PYTHON) $(SCRIPTS_DIR)/visualize_benchmarks.py $(RESULTS_DIR)/summary_latest.csv $(RESULTS_DIR)/raw_latest.csv $(RESULTS_DIR) || true
	@$(PYTHON) $(SCRIPTS_DIR)/visualize_summary.py $(RESULTS_DIR)/summary_latest.csv $(RESULTS_DIR)
	@$(PYTHON) $(SCRIPTS_DIR)/create_collages.py $(RESULTS_DIR) $(RESULTS_DIR) || true

# Regenerate figures and copy them into the LaTeX imagenes/ folders
# (thesis memoria + defense presentation share the same charts)
figures: visualize
	@mkdir -p $(IMAGES_DIR) $(SLIDES_IMAGES)
	@cp -f $(RESULTS_DIR)/chart_*.png $(IMAGES_DIR)/ 2>/dev/null || true
	@cp -f $(RESULTS_DIR)/collage_*.png $(IMAGES_DIR)/ 2>/dev/null || true
	@cp -f $(RESULTS_DIR)/chart_*.png $(SLIDES_IMAGES)/ 2>/dev/null || true
	@echo -e "$(LGFG)Figures copied to $(IMAGES_DIR)/ and $(SLIDES_IMAGES)/$(EC)"

# Regenerate the static slide illustrations (curve shapes + group law).
# These are analytic figures (no benchmark data), saved into the slides imagenes/.
illustrations:
	@echo -e "$(LGFG)Generating slide illustrations (ec_shape, ec_addition)...$(EC)"
	@$(PYTHON) $(SCRIPTS_DIR)/generate_slide_illustrations.py $(SLIDES_IMAGES)

# Compile the Beamer defense presentation (two passes for TOC/progress bar)
slides:
	@echo -e "$(LGFG)Compiling defense presentation...$(EC)"
	@cd $(CURDIR)/$(SLIDES_DIR) && pdflatex -interaction=nonstopmode -halt-on-error presentacion_defensa.tex >/dev/null
	@cd $(CURDIR)/$(SLIDES_DIR) && pdflatex -interaction=nonstopmode -halt-on-error presentacion_defensa.tex >/dev/null
	@echo -e "$(LGFG)Presentation built: $(SLIDES_DIR)/presentacion_defensa.pdf$(EC)"

# Full reproducible pipeline (serial order guaranteed)
reproduce:
	@$(MAKE) benchmark
	@$(MAKE) compare-openssl
	@$(MAKE) compare-compiler-flags
	@$(MAKE) figures
	@echo -e "$(LGFG)Full pipeline complete.$(EC)"

clean:
	@echo -e "$(LRFG)Cleaning build artifacts...$(EC)"
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "All clean!"
	@echo "--------------------------------------------------------------------------------\n"

.PHONY: all clean preamble show-info test-rsa test-rsa-2k test-ecc test-rsa-py