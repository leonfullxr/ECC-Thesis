# ─────────────────────────────────────────────────────────────────────────────
# Compiler / linker
CXX      := g++
PREFIX   ?= /usr/local
CXXFLAGS := -std=c++17 -O2 -march=native \
            -Iinclude \
            -I$(PREFIX)/include
LDFLAGS  := -L$(PREFIX)/lib \
            -Wl,-rpath=$(PREFIX)/lib \
            -lntl -lgmp -pthread

# ─────────────────────────────────────────────────────────────────────────────
# directory layout
SRC_DIR   := src
BUILD_DIR := build
BIN_DIR   := bin

# find all .cpp and derive .o and final binaries
SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRC_FILES))
BIN_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(BIN_DIR)/%,$(SRC_FILES))

# ─────────────────────────────────────────────────────────────────────────────
.PHONY: all clean
all: $(BIN_FILES)

# ensure build/ and bin/ exist
$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

# compile src/foo.cpp → build/foo.o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# link build/foo.o → bin/foo
$(BIN_DIR)/%: $(BUILD_DIR)/%.o | $(BIN_DIR)
	$(CXX) $< $(LDFLAGS) -o $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)/*
