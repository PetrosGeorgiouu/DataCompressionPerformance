#include <iostream>
#include <fstream>
#include <ostream>
#include <cassert>
#include <cstdint>
#include <array>
#include <algorithm>
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
    if (current_size == 0) {
        bufferByte(byte);
        return;
    }
    const uint8_t bits_left = current_size;
    const uint8_t completed_byte = static_cast<uint8_t>((current_byte << (8 - bits_left)) | (byte >> bits_left));
    const uint8_t remain_mask = static_cast<uint8_t>((uint16_t{1} << bits_left) - 1);
    const uint8_t remainder = static_cast<uint8_t>(byte & remain_mask);
    bufferByte(completed_byte);
    current_byte = remainder;
    current_size = bits_left;
}

void BitWriter::writeBits(uint64_t bytes, uint64_t size) {
    assert(size <= 64);
    assert(size == 64 || (bytes >> size) == 0);
     while (size > 0)
    {
        const uint64_t available{
            8ULL - current_size
        };

        const uint64_t bitsToTake{
            min(size, available)
        };

        // Select the next highest meaningful bits.
        const uint64_t shift{
            size - bitsToTake
        };

        const uint64_t mask{
            (uint64_t{1} << bitsToTake) - 1
        };

        const uint64_t chunk{
            (bytes >> shift) & mask
        };

        current_byte = static_cast<uint8_t>(
            (static_cast<uint16_t>(current_byte) << bitsToTake) |
            chunk);

        current_size = static_cast<uint8_t>(
            current_size + bitsToTake);

        size -= bitsToTake;

        if (current_size == 8)
        {
            bufferByte(current_byte);
        }
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