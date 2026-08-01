CXX := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -g -Iinclude

APP := apps/huff.cpp

SOURCES := src/FrequencyTable.cpp \
           src/HuffmanTree.cpp \
           src/CompressionNaive.cpp \
           src/BitWriter.cpp

HEADERS := include/huffman/FrequencyTable.hpp \
           include/huffman/HuffmanTree.hpp \
           include/huffman/CompressionNaive.hpp \
           include/huffman/BitWriterNaive.hpp

TESTS := tests/testnaive.cpp

build/huff: $(APP) $(SOURCES) $(HEADERS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(APP) $(SOURCES) -o build/huff

build/testnaive: $(TESTS) $(SOURCES) $(HEADERS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(TESTS) $(SOURCES) -o build/testnaive

build: build/huff

run: build/huff
	./build/huff compress input.txt output.huff

tests: build/testnaive
	mkdir -p build/test_output
	./build/testnaive

clean:
	rm -rf build

.PHONY: build run tests clean