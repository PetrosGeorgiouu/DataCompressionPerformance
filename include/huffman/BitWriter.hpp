#pragma once
#include <cstdint>
#include <ostream>

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
};