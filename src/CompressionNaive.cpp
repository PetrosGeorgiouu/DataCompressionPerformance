#include <ostream>
#include "huffman/FrequencyTable.hpp"
#include "huffman/HuffmanTree.hpp"
#include "huffman/BitWriterNaive.hpp"
#include "huffman/CompressionNaive.hpp"
#include <unordered_map>
#include <fstream>

using namespace std;

void compressorNaive(const string &txtPath) {
    // output is the compressed file.
    std::ofstream outputFile("output" + txtPath + ".huff", std::ios::binary);

    if (!outputFile)
    {
        throw std::runtime_error("Failed to open output.huff");
    }
    
    // Find the character frequencies in the .txt file
    unordered_map<char, uint64_t> freqs = findFrequenciesNaive(txtPath);
    // Obtain the Huffman tree and encodings
    HuffmanTreeNaive tree(freqs);
    unordered_map<char, string> encodings = tree.getEncodings();

    // Initialize Bitwriter
    BitWriterNaive writer(outputFile);

    // 1. Write magic bytes 2. version number 3.frequencies 3. original size in total number of characters;
    outputFile.write("HUFF", 4);
    const std::uint8_t version = 1;
    outputFile.write(reinterpret_cast<const char*>(&version), sizeof(version));
    uint64_t total = 0;
    int numFreqs = freqs.size();
    outputFile.write(reinterpret_cast<const char*>(&numFreqs), sizeof(numFreqs));
    for (const auto& kv : freqs) {
        outputFile.write(reinterpret_cast<const char*>(&kv.first), sizeof(kv.first));
        outputFile.write(reinterpret_cast<const char*>(&kv.second), sizeof(kv.second));
        total += kv.second;
    }
    outputFile.write(reinterpret_cast<const char*>(&total), sizeof(total));

    // Begin writing encoded data

    ifstream file(txtPath, ios::binary);
    char c;
    while (file.get(c))
    {
        string encoded = encodings[c];
        for (char bit : encoded) {
            if (bit == '0') {
                writer.writeBit(0);
            }
            else {
                writer.writeBit(1);
            }
        }
    }
    writer.flush();
}