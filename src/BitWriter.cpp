#include <iostream>
#include <fstream>
#include <ostream>
#include <cassert>
#include <cstdint>
#include "huffman/BitWriter.hpp"

using namespace std;

uint8_t current_byte;
uint8_t current_size;
// ostream& outputFile;

BitWriter::BitWriter(ostream &output)
    : current_byte{0}, current_size{0}, outputFile{output}
{
}

void BitWriter::writeBit(uint8_t bit)
{
    assert(bit == 0 || bit == 1);
    current_byte = static_cast<uint8_t>((current_byte << 1) | bit);
    current_size = (current_size + 1) % 8;
    if (current_size == 0)
    {
        outputFile.put(static_cast<char>(current_byte));
        current_byte = 0;
    }
}

void BitWriter::writeByte(uint8_t byte)
{
    for (int i = 7; i >= 0; i--)
    {
        writeBit((byte >> i) & 1);
    }
}

void BitWriter::flush()
{
    if (current_size > 0)
    {
        for (int i = 0; i < 8 - current_size; i++)
        {
            current_byte = static_cast<uint8_t>((current_byte << 1) | 0);
        }
        outputFile.put(static_cast<char>(current_byte));
        current_byte = 0;
    }
}

BitWriter::~BitWriter()
{
}