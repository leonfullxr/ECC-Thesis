CXX := g++
CXXFLAGS := -std=c++17 -O2 -march=native -Iinclude
LDFLAGS := -lntl -lgmp            # enlazar NTL/GMP

SRC := $(wildcard src/*.cpp)
OBJ := $(SRC:src/%.cpp=build/%.o)
BENCH_SRC := bench/bench_main.cpp
BENCH_OBJ := build/bench_main.o

.PHONY: all bench clean

all: bench

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/bench_main.o: $(BENCH_SRC) | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

bench: $(OBJ) $(BENCH_OBJ)
	$(CXX) $^ $(LDFLAGS) -o bin/bench

build:
	mkdir -p build bin results

clean:
	rm -rf build bin results/*.csv
