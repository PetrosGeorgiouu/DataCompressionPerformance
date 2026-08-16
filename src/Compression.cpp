#include <ostream>
#include <unordered_map>
#include <fstream>
#include <stdexcept>
#include <array>
#include "huffman/FrequencyTable.hpp"
#include "huffman/HuffmanTree.hpp"
#include "huffman/BitWriter.hpp"
#include "huffman/Compression.hpp"

using namespace std;

void compressor(const string &inputPath, const string &outputPath)
{
    ofstream outputFile(outputPath, ios::binary | ios::trunc);

    if (!outputFile)
    {
        throw runtime_error(
            "Failed to open output file: " + outputPath);
    }

    // Find the character frequencies in the .txt file
    array<uint64_t, 256> freqs = findFrequencies(inputPath);
    // Obtain the Huffman tree and encodings
    HuffmanTree tree(freqs);
    array<string, 256> encodings = tree.getEncodings();

    // Initialize Bitwriter
    BitWriter writer(outputFile);

    // 1. Write magic bytes 2. version number 3. original size in bytes 4. Serialized tree
    outputFile.write("HUFF", 4);
    const std::uint8_t version = 1;
    outputFile.write(reinterpret_cast<const char *>(&version), sizeof(version));
    uint64_t total = 0;
    for (int i = 0; i < 256; i++)
    {
        total += freqs[i];
    }
    outputFile.write(reinterpret_cast<const char *>(&total), sizeof(total));
    tree.serialize(writer);

    // Check if writing headerwas successful

    if (!outputFile)
    {
        throw std::runtime_error("Failed while writing Huffman header");
    }

    // Begin writing encoded data
    ifstream file(inputPath, ios::binary);
    char c;
    while (file.get(c))
    {
        const string &encoded = encodings[static_cast<unsigned char>(c)];
        for (char bit : encoded)
        {
            if (bit == '0')
            {
                writer.writeBit(0);
            }
            else
            {
                writer.writeBit(1);
            }
        }
    }
    writer.flush();
    outputFile.flush();

    if (!outputFile)
    {
        throw runtime_error(
            "Failed while writing output file: " + outputPath);
    }

    outputFile.close();

    if (outputFile.fail())
    {
        throw runtime_error(
            "Failed to close output file: " + outputPath);
    }
}