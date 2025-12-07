.ONESHELL:

################################################
#
# MAKEFILE VARIABLES
#
# RSA vs ECC Benchmark Makefile
# Proyecto: Curvas Elípticas y Criptografía
# Autor: Leon Elliott Fuller
# Fecha: 2025-01-06
######################### Compilation
CXX      := g++
PREFIX   ?= /usr/local
CXXFLAGS := -std=c++17 -O2 -march=native \
            -Iinclude \
            -I$(PREFIX)/include
LDFLAGS  := -L$(PREFIX)/lib \
            -Wl,-rpath=$(PREFIX)/lib \
            -lntl -lgmp -pthread

######################### Python benchmark
PYTHON        := python3
PYTHON_SCRIPT := rsa_benchmark.py

######################### Directories
SRC_DIR   := src
BUILD_DIR := build
BIN_DIR   := bin
PYTHON_DIR := scripts
INCLUDE_DIR := include

######################### Source and object files
SOURCES := $(SRC_DIR)/rng.cpp $(SRC_DIR)/rsa.cpp $(SRC_DIR)/ecc.cpp $(SRC_DIR)/main.cpp
OBJS    := $(BUILD_DIR)/rng.o $(BUILD_DIR)/rsa.o $(BUILD_DIR)/ecc.o $(BUILD_DIR)/main.o

######################### Parameters override
KEY_SIZE ?= 2048 # RSA key size for test-rsa target
ITERS ?= 1 # Iterations for test targets
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
	@echo "$(PBG)*$(EC)           Year 2024-2025           $(PBG)*$(EC)"
	@echo "$(PBG)*---------------------------------------------*$(EC)"
	@echo "$(PBG)*$(EC)        ECC and RSA Benchmarks      $(PBG)*$(EC)"
	@echo "$(PBG)*---------------------------------------------*$(EC)"
	@echo "$(PBG)*$(EC)     Author: Leon Elliott Fuller    $(PBG)*$(EC)"
	@echo "$(PBG)***********************************************$(EC)"

show-info:
	@echo  "\n$(IFG)Available targets:                            $(EC)"
	@echo ""
	@echo "$(IFG) -  make$(EC)             Build bench executable"
	@echo "$(IFG) -  make test-rsa$(EC)    Run RSA benchmark ($(KEY_SIZE)-bit, $(ITERS) iteration)"
	@echo "$(IFG) -  make test-rsa-2k$(EC) Run RSA benchmark (2048-bit, $(ITERS) iteration)"
	@echo "$(IFG) -  make test-ecc$(EC)    Run ECC benchmark ($(ITERS) iteration)"
	@echo "$(IFG) -  make test-rsa-py$(EC) Run RSA Python benchmark ($(KEY_SIZE)-bit, $(ITERS) iteration)"
	@echo "$(IFG) -  make all$(EC)         Alias for build"
	@echo "$(IFG) -  make clean$(EC)       Clean build artifacts$(EC)"
	@echo ""

# Create directories if they don't exist
$(BUILD_DIR) $(BIN_DIR):
	@mkdir -p $@

# Pattern rule for object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo -e "$(LGFG)Compiling $<…$(EC)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Main executable
$(BIN_DIR)/bench: $(OBJS) | $(BIN_DIR)
	@echo -e "$(LGFG)Linking bench…$(EC)"
	@$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

# RNG analysis tool
$(BIN_DIR)/rng_analysis: $(SRC_DIR)/rng_analysis.cpp $(BUILD_DIR)/rng.o | $(BIN_DIR)
	@echo -e "$(LGFG)Compiling RNG analysis tool…$(EC)"
	@$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

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

clean:
	@echo -e "$(LRFG)Cleaning build artifacts...$(EC)"
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "All clean!"
	@echo "--------------------------------------------------------------------------------\n"

.PHONY: all clean preamble show-info test-rsa test-rsa-2k test-ecc test-rsa-py