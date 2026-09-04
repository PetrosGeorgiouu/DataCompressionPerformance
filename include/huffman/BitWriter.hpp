#pragma once
#include <cstdint>
#include <ostream>
#include <array>

using namespace std;

class BitWriter
{

public:
    explicit BitWriter(ostream &output);
    ~BitWriter();
    void writeBit(uint8_t bit);
    void writeByte(uint8_t byte);
    void flush();

private:
    uint8_t current_byte;
    uint8_t current_size;
    ostream &outputFile;
    uint8_t buffer[4096];
    size_t buffer_bytes;

    void bufferFlush();
    void bufferByte(uint8_t byte);
};