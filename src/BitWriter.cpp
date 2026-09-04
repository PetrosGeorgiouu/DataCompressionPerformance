#include <iostream>
#include <fstream>
#include <ostream>
#include <cassert>
#include <cstdint>
#include <array>
#include "huffman/BitWriter.hpp"

using namespace std;

void BitWriter::bufferFlush() {
    if (buffer_bytes == 0) {
        return;
    }
    outputFile.write(reinterpret_cast<const char*>(buffer), buffer_bytes);
    buffer_bytes = 0;
}

void BitWriter::bufferByte(uint8_t byte) {
    buffer[buffer_bytes++] = byte;
    if (buffer_bytes == 4096) {
        bufferFlush();
    }
    current_byte = 0;
    current_size = 0;
}

BitWriter::BitWriter(ostream &output)
    : current_byte{0}, current_size{0}, outputFile{output},buffer{{}}, buffer_bytes{0}
{
}

void BitWriter::writeBit(uint8_t bit)
{
    assert(bit == 0 || bit == 1);
    current_byte = static_cast<uint8_t>((current_byte << 1) | bit);
    current_size++;
    if (current_size == 8)
    {
        bufferByte(current_byte);
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
        bufferByte(current_byte);
    }
    bufferFlush();
}

BitWriter::~BitWriter()
{
}