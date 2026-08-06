#pragma once
#include <cstdint>
#include <ostream>

using namespace std;

class BitWriterNaive {

    public:
        explicit BitWriterNaive(ostream& output);
        ~BitWriterNaive();
        void writeBit(uint8_t bit);
        void writeByte(uint8_t byte);
        void flush();

    private:
        uint8_t current_byte;
        uint8_t current_size;
        ostream& outputFile;

};