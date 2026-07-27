CXX := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -g -Iinclude

SOURCES := apps/huff.cpp \
           src/FrequencyTable.cpp \
           src/HuffmanTree.cpp \
           src/Compressor.cpp

HEADERS := include/huffman/FrequencyTable.hpp \
           include/huffman/HuffmanTree.hpp \
           include/huffman/Compressor.hpp


build/huff: $(SOURCES) $(HEADERS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) apps/huff.cpp src/*.cpp -o build/huff

build: build/huff

run: build/huff
	./build/huff compress input.txt output.huff

clean:
	rm -rf build

.PHONY: build run clean