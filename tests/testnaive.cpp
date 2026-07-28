// tests/test_bit_writer.cpp

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include "huffman/BitWriterNaive.hpp"

using namespace std;

void testBitWriter() {
const std::string filename{
        "build/test_output/bitwriter_multiple_bytes.bin"
    };

    {
        std::ofstream output{filename, std::ios::binary};

        assert(output.is_open() && "Could not create test file");

        BitWriterNaive writer{output};

        // First byte: 11110000 = 0xF0
        writer.writeBit(1);
        writer.writeBit(1);
        writer.writeBit(1);
        writer.writeBit(1);
        writer.writeBit(0);
        writer.writeBit(0);
        writer.writeBit(0);
        writer.writeBit(0);

        // Second byte: 10101010 = 0xAA
        writer.writeBit(1);
        writer.writeBit(0);
        writer.writeBit(1);
        writer.writeBit(0);
        writer.writeBit(1);
        writer.writeBit(0);
        writer.writeBit(1);
        writer.writeBit(0);

        // git push

        writer.flush();
    }

    std::ifstream input{filename, std::ios::binary};

    assert(input.is_open());

    char firstRaw{};
    char secondRaw{};

    input.get(firstRaw);
    input.get(secondRaw);

    assert(input.gcount() == 1);
    assert(input.peek() == std::ifstream::traits_type::eof());

    const uint8_t first = static_cast<uint8_t>(
        static_cast<unsigned char>(firstRaw)
    );

    const uint8_t second = static_cast<uint8_t>(
        static_cast<unsigned char>(secondRaw)
    );

    assert(first == 0b11110000);
    assert(second == 0b10101010);
    cout << "BitWriter Passed";
}

int main() {
    testBitWriter();
    return 0;
}