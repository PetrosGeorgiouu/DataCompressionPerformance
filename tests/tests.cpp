#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "huffman/BitWriter.hpp"
#include "huffman/Compression.hpp"
#include "huffman/FrequencyTable.hpp"
#include "huffman/HuffmanTree.hpp"

namespace
{

    namespace fs = std::filesystem;

    const fs::path TEST_OUTPUT_DIRECTORY{
        "build/test_output"};

    // -----------------------------------------------------------------------------
    // Minimal test framework
    // -----------------------------------------------------------------------------

    class TestFailure : public std::runtime_error
    {
    public:
        explicit TestFailure(const std::string &message)
            : std::runtime_error{message}
        {
        }
    };

    void expect(bool condition, const std::string &message)
    {
        if (!condition)
        {
            throw TestFailure{message};
        }
    }

    template <typename Actual, typename Expected>
    void expectEqual(
        const Actual &actual,
        const Expected &expected,
        const std::string &message)
    {
        if (!(actual == expected))
        {
            throw TestFailure{message};
        }
    }

    class TestSuite
    {
    public:
        explicit TestSuite(std::string name)
            : name_{std::move(name)}
        {
        }

        template <typename TestFunction>
        void run(const std::string &testName, TestFunction &&testFunction)
        {
            ++total_;

            try
            {
                testFunction();
                ++passed_;

                std::cout << "  [PASS] " << testName << '\n';
            }
            catch (const std::exception &exception)
            {
                std::cout
                    << "  [FAIL] " << testName
                    << "\n         " << exception.what()
                    << '\n';
            }
            catch (...)
            {
                std::cout
                    << "  [FAIL] " << testName
                    << "\n         Unknown exception"
                    << '\n';
            }
        }

        void printSummary() const
        {
            std::cout
                << '\n'
                << passed_ << '/' << total_
                << " test cases for " << name_ << " passed.\n";
        }

        [[nodiscard]] std::size_t passed() const
        {
            return passed_;
        }

        [[nodiscard]] std::size_t total() const
        {
            return total_;
        }

    private:
        std::string name_;
        std::size_t passed_{0};
        std::size_t total_{0};
    };

    // -----------------------------------------------------------------------------
    // File helpers
    // -----------------------------------------------------------------------------

    void writeTestFile(
        const fs::path &filename,
        const std::string &contents)
    {
        std::ofstream output{
            filename,
            std::ios::binary | std::ios::trunc};

        expect(
            output.is_open(),
            "Could not create test file: " + filename.string());

        output.write(
            contents.data(),
            static_cast<std::streamsize>(contents.size()));

        expect(
            output.good(),
            "Could not write test file: " + filename.string());
    }

    std::vector<uint8_t> readFileBytes(const fs::path &filename)
    {
        std::ifstream input{filename, std::ios::binary};

        expect(
            input.is_open(),
            "Could not open output file: " + filename.string());

        std::vector<uint8_t> bytes;
        char rawByte{};

        while (input.get(rawByte))
        {
            bytes.push_back(
                static_cast<uint8_t>(
                    static_cast<unsigned char>(rawByte)));
        }

        return bytes;
    }

    std::unordered_map<char, uint64_t> countFrequencies(
        const std::string &testName,
        const std::string &contents)
    {
        const fs::path filename{
            TEST_OUTPUT_DIRECTORY / (testName + ".txt")};

        writeTestFile(filename, contents);

        return findFrequencies(filename.string());
    }

    template <typename WriteFunction>
    std::vector<uint8_t> writeWithBitWriter(
        const std::string &testName,
        WriteFunction &&writeFunction)
    {
        const fs::path filename{
            TEST_OUTPUT_DIRECTORY / (testName + ".bin")};

        {
            std::ofstream output{
                filename,
                std::ios::binary | std::ios::trunc};

            expect(
                output.is_open(),
                "Could not create BitWriter output file");

            BitWriter writer{output};
            writeFunction(writer);
            writer.flush();
        }

        return readFileBytes(filename);
    }

    std::vector<uint8_t> writeBits(
        const std::string &testName,
        const std::vector<uint8_t> &bits)
    {
        return writeWithBitWriter(
            testName,
            [&bits](BitWriter &writer)
            {
                for (uint8_t bit : bits)
                {
                    writer.writeBit(bit);
                }
            });
    }

    std::vector<uint8_t> serializeTree(
        const std::string &testName,
        std::unordered_map<char, uint64_t> frequencies)
    {
        return writeWithBitWriter(
            testName,
            [&frequencies](BitWriter &writer)
            {
                HuffmanTree tree{frequencies};
                tree.serialize(writer);
            });
    }

    uint64_t readNativeUint64(
        const std::vector<uint8_t> &bytes,
        std::size_t offset)
    {
        expect(
            bytes.size() >= offset + sizeof(uint64_t),
            "Not enough bytes to read uint64_t field");

        uint64_t value{0};
        std::memcpy(&value, bytes.data() + offset, sizeof(value));
        return value;
    }

    void expectCompressionHeader(
        const std::vector<uint8_t> &bytes,
        uint64_t expectedOriginalSize)
    {
        constexpr std::size_t headerSize{
            4 + sizeof(uint8_t) + sizeof(uint64_t)};

        expect(
            bytes.size() >= headerSize,
            "Compressed output is smaller than the fixed header");

        expectEqual(bytes[0], uint8_t{'H'}, "Missing H in HUFF magic bytes");
        expectEqual(bytes[1], uint8_t{'U'}, "Missing U in HUFF magic bytes");
        expectEqual(bytes[2], uint8_t{'F'}, "Missing first F in HUFF magic bytes");
        expectEqual(bytes[3], uint8_t{'F'}, "Missing second F in HUFF magic bytes");
        expectEqual(bytes[4], uint8_t{1}, "Unexpected compressor format version");

        expectEqual(
            readNativeUint64(bytes, 5),
            expectedOriginalSize,
            "Original-size field in compressed header is incorrect");
    }

    std::vector<uint8_t> compressContents(
        const std::string &testName,
        const std::string &contents)
    {
        const fs::path inputPath{
            TEST_OUTPUT_DIRECTORY / (testName + ".txt")};
        const fs::path outputPath{
            TEST_OUTPUT_DIRECTORY / (testName + ".huff")};

        writeTestFile(inputPath, contents);
        compressor(inputPath.string(), outputPath.string());

        return readFileBytes(outputPath);
    }

    // -----------------------------------------------------------------------------
    // Huffman helpers
    // -----------------------------------------------------------------------------

    bool containsOnlyBinaryDigits(const std::string &encoding)
    {
        for (char bit : encoding)
        {
            if (bit != '0' && bit != '1')
            {
                return false;
            }
        }

        return true;
    }

    bool isPrefixOf(
        const std::string &possiblePrefix,
        const std::string &value)
    {
        if (possiblePrefix.size() > value.size())
        {
            return false;
        }

        return value.compare(
                   0,
                   possiblePrefix.size(),
                   possiblePrefix) == 0;
    }

    bool isPrefixFree(
        const std::unordered_map<char, std::string> &encodings)
    {
        for (const auto &[firstCharacter, firstCode] : encodings)
        {
            for (const auto &[secondCharacter, secondCode] : encodings)
            {
                if (firstCharacter == secondCharacter)
                {
                    continue;
                }

                if (isPrefixOf(firstCode, secondCode))
                {
                    return false;
                }
            }
        }

        return true;
    }

    std::unordered_map<char, uint64_t> classicFrequencies()
    {
        return {
            {'a', 45},
            {'b', 13},
            {'c', 12},
            {'d', 16},
            {'e', 9},
            {'f', 5}};
    }

    // -----------------------------------------------------------------------------
    // Frequency-counting tests
    // -----------------------------------------------------------------------------

    TestSuite runFrequencyTests()
    {
        TestSuite suite{"Frequency Counting"};

        std::cout << "\n=== Frequency Counting ===\n";

        suite.run("empty file produces an empty table", []
                  {
        const auto frequencies{
            countFrequencies("frequency_empty", "")
        };

        expect(
            frequencies.empty(),
            "Expected no entries for an empty file"
        ); });

        suite.run("single character is counted once", []
                  {
        const auto frequencies{
            countFrequencies("frequency_single", "x")
        };

        expectEqual(
            frequencies.size(),
            std::size_t{1},
            "Expected exactly one unique character"
        );

        expectEqual(
            frequencies.at('x'),
            uint64_t{1},
            "Expected x to have frequency 1"
        ); });

        suite.run("repeated character is counted correctly", []
                  {
        const auto frequencies{
            countFrequencies("frequency_repeated", "aaaaaaa")
        };

        expectEqual(
            frequencies.size(),
            std::size_t{1},
            "Expected exactly one unique character"
        );

        expectEqual(
            frequencies.at('a'),
            uint64_t{7},
            "Expected a to have frequency 7"
        ); });

        suite.run("multiple character frequencies are correct", []
                  {
        const auto frequencies{
            countFrequencies("frequency_multiple", "aaabbc")
        };

        expectEqual(
            frequencies.size(),
            std::size_t{3},
            "Expected three unique characters"
        );

        expectEqual(
            frequencies.at('a'),
            uint64_t{3},
            "Expected a to have frequency 3"
        );

        expectEqual(
            frequencies.at('b'),
            uint64_t{2},
            "Expected b to have frequency 2"
        );

        expectEqual(
            frequencies.at('c'),
            uint64_t{1},
            "Expected c to have frequency 1"
        ); });

        suite.run("spaces and tabs are counted", []
                  {
        const auto frequencies{
            countFrequencies(
                "frequency_whitespace",
                "a a\tb"
            )
        };

        expectEqual(
            frequencies.at('a'),
            uint64_t{2},
            "Expected a to have frequency 2"
        );

        expectEqual(
            frequencies.at(' '),
            uint64_t{1},
            "Expected one space"
        );

        expectEqual(
            frequencies.at('\t'),
            uint64_t{1},
            "Expected one tab"
        );

        expectEqual(
            frequencies.at('b'),
            uint64_t{1},
            "Expected b to have frequency 1"
        ); });

        suite.run("uppercase, digits, and punctuation are distinct", []
                  {
        const auto frequencies{
            countFrequencies(
                "frequency_punctuation",
                "A1!A1?"
            )
        };

        expectEqual(
            frequencies.size(),
            std::size_t{4},
            "Expected four unique characters"
        );

        expectEqual(
            frequencies.at('A'),
            uint64_t{2},
            "Expected A to have frequency 2"
        );

        expectEqual(
            frequencies.at('1'),
            uint64_t{2},
            "Expected 1 to have frequency 2"
        );

        expectEqual(
            frequencies.at('!'),
            uint64_t{1},
            "Expected ! to have frequency 1"
        );

        expectEqual(
            frequencies.at('?'),
            uint64_t{1},
            "Expected ? to have frequency 1"
        ); });

        suite.run("newline characters are preserved and counted", []
                  {
        const auto frequencies{
            countFrequencies(
                "frequency_newlines",
                "a\nb\n"
            )
        };

        expectEqual(
            frequencies.at('a'),
            uint64_t{1},
            "Expected a to have frequency 1"
        );

        expectEqual(
            frequencies.at('b'),
            uint64_t{1},
            "Expected b to have frequency 1"
        );

        expect(
            frequencies.find('\n') != frequencies.end(),
            "Newline character was removed from the input"
        );

        expectEqual(
            frequencies.at('\n'),
            uint64_t{2},
            "Expected two newline characters"
        ); });

        suite.printSummary();
        return suite;
    }

    // -----------------------------------------------------------------------------
    // Huffman-encoding tests
    // -----------------------------------------------------------------------------

    TestSuite runHuffmanEncodingTests()
    {
        TestSuite suite{"Huffman Encodings"};

        std::cout << "\n=== Huffman Encodings ===\n";

        suite.run("two symbols both receive encodings", []
                  {
        std::unordered_map<char, uint64_t> frequencies{
            {'a', 1},
            {'b', 1}
        };

        HuffmanTree tree{frequencies};
        const auto encodings{tree.getEncodings()};

        expectEqual(
            encodings.size(),
            std::size_t{2},
            "Expected two encodings"
        );

        expect(
            encodings.find('a') != encodings.end(),
            "Missing encoding for a"
        );

        expect(
            encodings.find('b') != encodings.end(),
            "Missing encoding for b"
        );

        expect(
            !encodings.at('a').empty(),
            "Encoding for a should not be empty"
        );

        expect(
            !encodings.at('b').empty(),
            "Encoding for b should not be empty"
        ); });

        suite.run("encodings contain only zeroes and ones", []
                  {
        auto frequencies{classicFrequencies()};

        HuffmanTree tree{frequencies};
        const auto encodings{tree.getEncodings()};

        for (const auto& [character, encoding] : encodings) {
            static_cast<void>(character);

            expect(
                containsOnlyBinaryDigits(encoding),
                "An encoding contained a character other than 0 or 1"
            );
        } });

        suite.run("encodings are prefix-free", []
                  {
        auto frequencies{classicFrequencies()};

        HuffmanTree tree{frequencies};
        const auto encodings{tree.getEncodings()};

        expect(
            isPrefixFree(encodings),
            "One Huffman encoding is a prefix of another"
        ); });

        suite.run("zero-frequency symbols are ignored", []
                  {
        std::unordered_map<char, uint64_t> frequencies{
            {'a', 3},
            {'b', 2},
            {'z', 0}
        };

        HuffmanTree tree{frequencies};
        const auto encodings{tree.getEncodings()};

        expectEqual(
            encodings.size(),
            std::size_t{2},
            "Expected only positive-frequency symbols"
        );

        expect(
            encodings.find('z') == encodings.end(),
            "Zero-frequency symbol z should not receive an encoding"
        ); });

        suite.run("common symbols are not longer than rare symbols", []
                  {
        auto frequencies{classicFrequencies()};

        HuffmanTree tree{frequencies};
        const auto encodings{tree.getEncodings()};

        expect(
            encodings.at('a').size() <= encodings.at('f').size(),
            "Most-common symbol received a longer code than rarest symbol"
        ); });

        suite.run("classic frequencies have optimal weighted cost", []
                  {
        auto frequencies{classicFrequencies()};

        HuffmanTree tree{frequencies};
        const auto encodings{tree.getEncodings()};

        uint64_t weightedCost{0};

        for (const auto& [character, frequency] : frequencies) {
            weightedCost += frequency * encodings.at(character).size();
        }

        expectEqual(
            weightedCost,
            uint64_t{224},
            "Unexpected weighted encoding cost"
        ); });

        suite.run("single-symbol input receives a usable code", []
                  {
        std::unordered_map<char, uint64_t> frequencies{
            {'x', 10}
        };

        HuffmanTree tree{frequencies};
        const auto encodings{tree.getEncodings()};

        expectEqual(
            encodings.size(),
            std::size_t{1},
            "Expected exactly one encoding"
        );

        expect(
            encodings.find('x') != encodings.end(),
            "Missing encoding for x"
        );

        expect(
            !encodings.at('x').empty(),
            "A single-symbol alphabet must receive a non-empty code"
        );

        expect(
            containsOnlyBinaryDigits(encodings.at('x')),
            "Single-symbol encoding must contain only 0 and 1"
        ); });

        suite.printSummary();
        return suite;
    }

    // -----------------------------------------------------------------------------
    // Huffman-serialization tests
    // -----------------------------------------------------------------------------

    TestSuite runHuffmanSerializationTests()
    {
        TestSuite suite{"Huffman Serialization"};

        std::cout << "\n=== Huffman Serialization ===\n";

        suite.run("empty tree serializes to no bytes", []
                  {
        std::unordered_map<char, uint64_t> frequencies;

        const auto bytes{
            serializeTree("serialize_empty", frequencies)
        };

        expect(
            bytes.empty(),
            "Expected an empty Huffman tree to serialize to no bytes"
        ); });

        suite.run("single leaf writes marker followed by unaligned character byte", []
                  {
        std::unordered_map<char, uint64_t> frequencies{
            {'a', 3}
        };

        const auto bytes{
            serializeTree("serialize_single_leaf", frequencies)
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0xB0, 0x80},
            "Expected serialized bits 1|01100001, padded to B0 80"
        ); });

        suite.run("two-leaf tree serializes in preorder", []
                  {
        std::unordered_map<char, uint64_t> frequencies{
            {'a', 1},
            {'b', 2}
        };

        const auto bytes{
            serializeTree("serialize_two_leaf", frequencies)
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0x58, 0x6C, 0x40},
            "Expected preorder serialization 0|1a|1b"
        ); });

        suite.printSummary();
        return suite;
    }

    // -----------------------------------------------------------------------------
    // BitWriter tests
    // -----------------------------------------------------------------------------

    TestSuite runBitWriterTests()
    {
        TestSuite suite{"BitWriter"};

        std::cout << "\n=== BitWriter ===\n";

        suite.run("flushing without bits creates an empty file", []
                  {
        const auto bytes{
            writeBits("bitwriter_empty", {})
        };

        expect(
            bytes.empty(),
            "Expected an empty output file"
        ); });

        suite.run("eight zero bits produce 0x00", []
                  {
        const auto bytes{
            writeBits(
                "bitwriter_zero_byte",
                {0, 0, 0, 0, 0, 0, 0, 0}
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0x00},
            "Expected exactly one 0x00 byte"
        ); });

        suite.run("eight one bits produce 0xFF", []
                  {
        const auto bytes{
            writeBits(
                "bitwriter_one_byte",
                {1, 1, 1, 1, 1, 1, 1, 1}
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0xFF},
            "Expected exactly one 0xFF byte"
        ); });

        suite.run("11110000 produces 0xF0", []
                  {
        const auto bytes{
            writeBits(
                "bitwriter_mixed_byte",
                {1, 1, 1, 1, 0, 0, 0, 0}
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0xF0},
            "Expected exactly one 0xF0 byte"
        ); });

        suite.run("multiple complete bytes are written correctly", []
                  {
        const auto bytes{
            writeBits(
                "bitwriter_multiple_bytes",
                {
                    1, 1, 1, 1, 0, 0, 0, 0,
                    1, 0, 1, 0, 1, 0, 1, 0
                }
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0xF0, 0xAA},
            "Expected bytes 0xF0 and 0xAA"
        ); });

        suite.run("partial byte is padded on the right", []
                  {
        const auto bytes{
            writeBits(
                "bitwriter_partial_byte",
                {1, 0, 1}
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0xA0},
            "Expected partial byte 101 to become 10100000"
        ); });

        suite.run("bits crossing a byte boundary are preserved", []
                  {
        const auto bytes{
            writeBits(
                "bitwriter_cross_boundary",
                {
                    1, 0, 1, 0, 1, 0, 1, 0,
                    1
                }
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0xAA, 0x80},
            "Expected bytes 0xAA and 0x80"
        ); });

        suite.run("aligned writeByte writes the byte unchanged", []
                  {
        const auto bytes{
            writeWithBitWriter(
                "bitwriter_aligned_byte",
                [](BitWriter &writer)
                {
                    writer.writeByte(0xA5);
                }
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0xA5},
            "Expected aligned byte 0xA5"
        ); });

        suite.run("writeByte works after one pending bit", []
                  {
        const auto bytes{
            writeWithBitWriter(
                "bitwriter_unaligned_one_bit",
                [](BitWriter &writer)
                {
                    writer.writeBit(1);
                    writer.writeByte(0x61);
                }
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0xB0, 0x80},
            "Expected bit stream 1|01100001 to cross the byte boundary correctly"
        ); });

        suite.run("writeByte works after three pending bits", []
                  {
        const auto bytes{
            writeWithBitWriter(
                "bitwriter_unaligned_three_bits",
                [](BitWriter &writer)
                {
                    writer.writeBit(1);
                    writer.writeBit(0);
                    writer.writeBit(1);
                    writer.writeByte(0xA5);
                }
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0xB4, 0xA0},
            "Expected bit stream 101|10100101 to remain contiguous"
        ); });

        suite.run("bit writes continue at the correct offset after writeByte", []
                  {
        const auto bytes{
            writeWithBitWriter(
                "bitwriter_unaligned_then_bit",
                [](BitWriter &writer)
                {
                    writer.writeBit(1);
                    writer.writeByte(0x00);
                    writer.writeBit(1);
                }
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0x80, 0x40},
            "Expected trailing bit to follow the unaligned byte without a gap"
        ); });

        suite.run("multiple bytes can be written while unaligned", []
                  {
        const auto bytes{
            writeWithBitWriter(
                "bitwriter_multiple_unaligned_bytes",
                [](BitWriter &writer)
                {
                    writer.writeBit(1);
                    writer.writeBit(0);
                    writer.writeBit(1);
                    writer.writeByte(0xF0);
                    writer.writeByte(0x0F);
                }
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0xBE, 0x01, 0xE0},
            "Expected consecutive unaligned bytes to preserve every bit"
        ); });

        suite.printSummary();
        return suite;
    }

    // -----------------------------------------------------------------------------
    // Compressor tests
    // -----------------------------------------------------------------------------

    TestSuite runCompressorTests()
    {
        TestSuite suite{"Compressor"};

        std::cout << "\n=== Compressor ===\n";

        suite.run("empty input writes only the fixed header", []
                  {
        const auto bytes{
            compressContents("compress_empty", "")
        };

        expectCompressionHeader(bytes, 0);

        expectEqual(
            bytes.size(),
            std::size_t{4 + sizeof(uint8_t) + sizeof(uint64_t)},
            "Empty input should have no serialized tree or encoded payload"
        ); });

        suite.run("single-symbol input writes header tree and payload", []
                  {
        const auto bytes{
            compressContents("compress_single_symbol", "aaa")
        };

        expectCompressionHeader(bytes, 3);

        const std::vector<uint8_t> body{
            bytes.begin() + static_cast<std::ptrdiff_t>(
                4 + sizeof(uint8_t) + sizeof(uint64_t)),
            bytes.end()
        };

        expectEqual(
            body,
            std::vector<uint8_t>{0xB0, 0x80},
            "Expected serialized leaf 1|a followed by three 0 payload bits"
        ); });

        suite.run("two-symbol input keeps serialized tree and payload contiguous", []
                  {
        const auto bytes{
            compressContents("compress_two_symbols", "abb")
        };

        expectCompressionHeader(bytes, 3);

        const std::vector<uint8_t> body{
            bytes.begin() + static_cast<std::ptrdiff_t>(
                4 + sizeof(uint8_t) + sizeof(uint64_t)),
            bytes.end()
        };

        expectEqual(
            body,
            std::vector<uint8_t>{0x58, 0x6C, 0x4C},
            "Expected preorder tree 0|1a|1b followed immediately by payload 011"
        ); });

        suite.run("compressor records the exact original byte count", []
                  {
        const std::string contents{"a\n b\t!"};
        const auto bytes{
            compressContents("compress_original_size", contents)
        };

        expectCompressionHeader(
            bytes,
            static_cast<uint64_t>(contents.size())
        ); });

        suite.printSummary();
        return suite;
    }

} // namespace

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main()
{
    try
    {
        fs::create_directories(TEST_OUTPUT_DIRECTORY);
    }
    catch (const fs::filesystem_error &exception)
    {
        std::cerr
            << "Could not create test-output directory: "
            << exception.what()
            << '\n';

        return 1;
    }

    std::cout
        << "============================================\n"
        << " Huffman Compression Lab - Naive Test Suite\n"
        << "============================================\n";

    const TestSuite frequencySuite{
        runFrequencyTests()};

    const TestSuite huffmanSuite{
        runHuffmanEncodingTests()};

    const TestSuite serializationSuite{
        runHuffmanSerializationTests()};

    const TestSuite bitWriterSuite{
        runBitWriterTests()};

    const TestSuite compressorSuite{
        runCompressorTests()};

    const std::size_t totalPassed{
        frequencySuite.passed() +
        huffmanSuite.passed() +
        serializationSuite.passed() +
        bitWriterSuite.passed() +
        compressorSuite.passed()};

    const std::size_t totalTests{
        frequencySuite.total() +
        huffmanSuite.total() +
        serializationSuite.total() +
        bitWriterSuite.total() +
        compressorSuite.total()};

    std::cout
        << "\n============================================\n"
        << " Final Result: "
        << totalPassed << '/' << totalTests
        << " test cases passed\n"
        << "============================================\n";

    if (totalPassed == totalTests)
    {
        std::cout << "ALL TESTS PASSED\n";
        return 0;
    }

    std::cout << "SOME TESTS FAILED\n";
    return 1;
}