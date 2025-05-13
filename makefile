ONESHELL:
#SHELL := /usr/bin/env bash

################################################
#
# MAKEFILE VARIABLES
#
# RSA vs ECC Benchmark Makefile
# Proyecto: Curvas Elípticas y Criptografía
# Autor: Leon Elliott Fuller
# Fecha: 2025-05-11
######################### Compilation
CXX      := g++
PREFIX   ?= /usr/local
CXXFLAGS := -std=c++17 -O2 -march=native \
            -Iinclude \
            -I$(PREFIX)/include
LDFLAGS  := -L$(PREFIX)/lib \
            -Wl,-rpath=$(PREFIX)/lib \
            -lntl -lgmp -pthread

######################### Directories
SRC_DIR   := src
BUILD_DIR := build
BIN_DIR   := bin

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

all: preamble show-info $(BIN_DIR)/bench

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
	@echo "$(IFG) -  make all$(EC)         Alias for build"
	@echo "$(IFG) -  make clean$(EC)       Clean build artifacts$(EC)"
	@echo ""

$(BUILD_DIR) $(BIN_DIR):
	@mkdir -p $@

bin/bench: $(OBJS)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo -e "$(LGFG)Compiling $<…$(EC)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BIN_DIR)/bench: $(BUILD_DIR)/rsa.o $(BUILD_DIR)/ecc.o $(BUILD_DIR)/main.o
	@echo -e "$(LGFG)Linking bench…$(EC)"
	@$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

# Basic test targets with output

test-rsa: $(BIN_DIR)/bench
	@echo -e "$(LGFG)Executing RSA ($(KEY_SIZE)-bit, $(ITERS) iteration(s))...$(EC)"
	@$(BIN_DIR)/bench -a RSA -b $(KEY_SIZE) -i $(ITERS) -s fixed

test-rsa-2k: $(BIN_DIR)/bench
	@echo -e "$(LGFG)Executing RSA (2048-bit, $(ITERS) iteration(s))...$(EC)"
	@$(BIN_DIR)/bench -a RSA -b 2048 -i $(ITERS) -s fixed

test-ecc: $(BIN_DIR)/bench
	@echo -e "$(LGFG)Executing ECC ($(ITERS) iteration(s))...$(EC)"
	@$(BIN_DIR)/bench -a ECC -i $(ITERS) -s fixed

clean:
	@echo -e "$(LRFG)Cleaning build artifacts...$(EC)"
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "All clean!"
	@echo "--------------------------------------------------------------------------------\n"

.PHONY: all clean