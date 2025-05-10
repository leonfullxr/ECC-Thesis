# ─────────────────────────────────────────────────────────────────────────────
# Compiler / linker
CXX      := g++
# allow overriding (e.g. for non-default installs)
PREFIX   ?= /usr/local

# project headers in ./include, plus NTL/GMP in $(PREFIX)
CXXFLAGS := -std=c++17 -O2 -march=native \
            -Iinclude \
            -I$(PREFIX)/include

# link against NTL & GMP in $(PREFIX)/lib, embed rpath, plus pthread
LDFLAGS  := -L$(PREFIX)/lib \
            -Wl,-rpath=$(PREFIX)/lib \
            -lntl -lgmp -pthread

# ─────────────────────────────────────────────────────────────────────────────
# directory layout
SRC_DIR   := src
BUILD_DIR := build
BIN_DIR   := bin

# sources & objects
SRC    := $(wildcard $(SRC_DIR)/*.cpp)
OBJ    := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRC))

BENCH_SRC := src/my_ecc.cpp
BENCH_OBJ := $(BUILD_DIR)/my_ecc.o

# ─────────────────────────────────────────────────────────────────────────────
.PHONY: all bench clean

all: bench

# compile every src/%.cpp → build/%.o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR) $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# compile bench/bench_main.cpp
$(BENCH_OBJ): $(BENCH_SRC) | $(BUILD_DIR) $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# link bench binary
bench: $(OBJ) $(BENCH_OBJ)
	$(CXX) $^ $(LDFLAGS) -o $(BIN_DIR)/bench

# ensure build/ and bin/ exist
$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)/*
