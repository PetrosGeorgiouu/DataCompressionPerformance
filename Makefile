CXX := clang++

# Optimized build settings for compression benchmarks.
CXXFLAGS := -std=c++17 \
            -O2 \
            -g \
            -fno-omit-frame-pointer \
            -DNDEBUG \
            -Wall \
            -Wextra \
            -Wpedantic \
            -Iinclude

SOURCES := src/FrequencyTable.cpp \
           src/HuffmanTree.cpp \
           src/Compression.cpp \
           src/BitWriter.cpp

HEADERS := include/huffman/FrequencyTable.hpp \
           include/huffman/HuffmanTree.hpp \
           include/huffman/Compression.hpp \
           include/huffman/BitWriter.hpp

TESTS := tests/tests.cpp

FILE ?=
CORPUS_DIR ?= data/corpus
BENCH_ARGS ?=

BENCHMARK_INCLUDE := $(HOME)/benchmark/include
BENCHMARK_LIB := $(HOME)/benchmark/build-release/src/libbenchmark.a

build/fileperf: apps/fileperf.cpp $(SOURCES) $(HEADERS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) -I$(BENCHMARK_INCLUDE) apps/fileperf.cpp $(SOURCES) \
		$(BENCHMARK_LIB) -lpthread \
		-o build/fileperf

build/corpus: apps/corpus.cpp $(SOURCES) $(HEADERS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) apps/corpus.cpp $(SOURCES) -o build/corpus

build/tests: $(TESTS) $(SOURCES) $(HEADERS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(TESTS) $(SOURCES) -o build/tests

build: build/fileperf build/corpus

fileperf: build/fileperf
	@if [ -z "$(FILE)" ]; then \
		echo "Usage: make fileperf FILE=path/to/input.txt"; \
		exit 1; \
	fi
	./build/fileperf "$(FILE)" $(BENCH_ARGS)

corpus: build/corpus
	./build/corpus "$(CORPUS_DIR)"

tests: build/tests
	mkdir -p build/test_output
	./build/tests

clean:
	rm -rf build

.PHONY: build fileperf corpus tests clean