
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "huffman/BitWriterNaive.hpp"
#include "huffman/FrequencyTable.hpp"
#include "huffman/HuffmanTree.hpp"

namespace {

namespace fs = std::filesystem;

const fs::path TEST_OUTPUT_DIRECTORY{
    "build/test_output"
};

// -----------------------------------------------------------------------------
// Minimal test framework
// -----------------------------------------------------------------------------

class TestFailure : public std::runtime_error {
public:
    explicit TestFailure(const std::string& message)
        : std::runtime_error{message} {
    }
};

void expect(bool condition, const std::string& message) {
    if (!condition) {
        throw TestFailure{message};
    }
}

template <typename Actual, typename Expected>
void expectEqual(
    const Actual& actual,
    const Expected& expected,
    const std::string& message
) {
    if (!(actual == expected)) {
        throw TestFailure{message};
    }
}

class TestSuite {
public:
    explicit TestSuite(std::string name)
        : name_{std::move(name)} {
    }

    template <typename TestFunction>
    void run(const std::string& testName, TestFunction&& testFunction) {
        ++total_;

        try {
            testFunction();
            ++passed_;

            std::cout << "  [PASS] " << testName << '\n';
        } catch (const std::exception& exception) {
            std::cout
                << "  [FAIL] " << testName
                << "\n         " << exception.what()
                << '\n';
        } catch (...) {
            std::cout
                << "  [FAIL] " << testName
                << "\n         Unknown exception"
                << '\n';
        }
    }

    void printSummary() const {
        std::cout
            << '\n'
            << passed_ << '/' << total_
            << " test cases for " << name_ << " passed.\n";
    }

    [[nodiscard]] std::size_t passed() const {
        return passed_;
    }

    [[nodiscard]] std::size_t total() const {
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
    const fs::path& filename,
    const std::string& contents
) {
    std::ofstream output{
        filename,
        std::ios::binary | std::ios::trunc
    };

    expect(
        output.is_open(),
        "Could not create test file: " + filename.string()
    );

    output.write(
        contents.data(),
        static_cast<std::streamsize>(contents.size())
    );

    expect(
        output.good(),
        "Could not write test file: " + filename.string()
    );
}

std::vector<uint8_t> readFileBytes(const fs::path& filename) {
    std::ifstream input{filename, std::ios::binary};

    expect(
        input.is_open(),
        "Could not open output file: " + filename.string()
    );

    std::vector<uint8_t> bytes;
    char rawByte{};

    while (input.get(rawByte)) {
        bytes.push_back(
            static_cast<uint8_t>(
                static_cast<unsigned char>(rawByte)
            )
        );
    }

    return bytes;
}

std::unordered_map<char, uint64_t> countFrequencies(
    const std::string& testName,
    const std::string& contents
) {
    const fs::path filename{
        TEST_OUTPUT_DIRECTORY / (testName + ".txt")
    };

    writeTestFile(filename, contents);

    return findFrequenciesNaive(filename.string());
}

std::vector<uint8_t> writeBits(
    const std::string& testName,
    const std::vector<uint8_t>& bits
) {
    const fs::path filename{
        TEST_OUTPUT_DIRECTORY / (testName + ".bin")
    };

    {
        std::ofstream output{
            filename,
            std::ios::binary | std::ios::trunc
        };

        expect(
            output.is_open(),
            "Could not create BitWriter output file"
        );

        BitWriterNaive writer{output};

        for (uint8_t bit : bits) {
            writer.writeBit(bit);
        }

        writer.flush();
    }

    return readFileBytes(filename);
}

// -----------------------------------------------------------------------------
// Huffman helpers
// -----------------------------------------------------------------------------

bool containsOnlyBinaryDigits(const std::string& encoding) {
    for (char bit : encoding) {
        if (bit != '0' && bit != '1') {
            return false;
        }
    }

    return true;
}

bool isPrefixOf(
    const std::string& possiblePrefix,
    const std::string& value
) {
    if (possiblePrefix.size() > value.size()) {
        return false;
    }

    return value.compare(
        0,
        possiblePrefix.size(),
        possiblePrefix
    ) == 0;
}

bool isPrefixFree(
    const std::unordered_map<char, std::string>& encodings
) {
    for (const auto& [firstCharacter, firstCode] : encodings) {
        for (const auto& [secondCharacter, secondCode] : encodings) {
            if (firstCharacter == secondCharacter) {
                continue;
            }

            if (isPrefixOf(firstCode, secondCode)) {
                return false;
            }
        }
    }

    return true;
}

std::unordered_map<char, uint64_t> classicFrequencies() {
    return {
        {'a', 45},
        {'b', 13},
        {'c', 12},
        {'d', 16},
        {'e', 9},
        {'f', 5}
    };
}

// -----------------------------------------------------------------------------
// Frequency-counting tests
// -----------------------------------------------------------------------------

TestSuite runFrequencyTests() {
    TestSuite suite{"Frequency Counting"};

    std::cout << "\n=== Frequency Counting ===\n";

    suite.run("empty file produces an empty table", [] {
        const auto frequencies{
            countFrequencies("frequency_empty", "")
        };

        expect(
            frequencies.empty(),
            "Expected no entries for an empty file"
        );
    });

    suite.run("single character is counted once", [] {
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
        );
    });

    suite.run("repeated character is counted correctly", [] {
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
        );
    });

    suite.run("multiple character frequencies are correct", [] {
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
        );
    });

    suite.run("spaces and tabs are counted", [] {
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
        );
    });

    suite.run("uppercase, digits, and punctuation are distinct", [] {
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
        );
    });

    suite.run("newline characters are preserved and counted", [] {
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
        );
    });

    suite.printSummary();
    return suite;
}

// -----------------------------------------------------------------------------
// Huffman-encoding tests
// -----------------------------------------------------------------------------

TestSuite runHuffmanEncodingTests() {
    TestSuite suite{"Huffman Encodings"};

    std::cout << "\n=== Huffman Encodings ===\n";

    suite.run("two symbols both receive encodings", [] {
        std::unordered_map<char, uint64_t> frequencies{
            {'a', 1},
            {'b', 1}
        };

        HuffmanTreeNaive tree{frequencies};
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
        );
    });

    suite.run("encodings contain only zeroes and ones", [] {
        auto frequencies{classicFrequencies()};

        HuffmanTreeNaive tree{frequencies};
        const auto encodings{tree.getEncodings()};

        for (const auto& [character, encoding] : encodings) {
            static_cast<void>(character);

            expect(
                containsOnlyBinaryDigits(encoding),
                "An encoding contained a character other than 0 or 1"
            );
        }
    });

    suite.run("encodings are prefix-free", [] {
        auto frequencies{classicFrequencies()};

        HuffmanTreeNaive tree{frequencies};
        const auto encodings{tree.getEncodings()};

        expect(
            isPrefixFree(encodings),
            "One Huffman encoding is a prefix of another"
        );
    });

    suite.run("zero-frequency symbols are ignored", [] {
        std::unordered_map<char, uint64_t> frequencies{
            {'a', 3},
            {'b', 2},
            {'z', 0}
        };

        HuffmanTreeNaive tree{frequencies};
        const auto encodings{tree.getEncodings()};

        expectEqual(
            encodings.size(),
            std::size_t{2},
            "Expected only positive-frequency symbols"
        );

        expect(
            encodings.find('z') == encodings.end(),
            "Zero-frequency symbol z should not receive an encoding"
        );
    });

    suite.run("common symbols are not longer than rare symbols", [] {
        auto frequencies{classicFrequencies()};

        HuffmanTreeNaive tree{frequencies};
        const auto encodings{tree.getEncodings()};

        expect(
            encodings.at('a').size() <= encodings.at('f').size(),
            "Most-common symbol received a longer code than rarest symbol"
        );
    });

    suite.run("classic frequencies have optimal weighted cost", [] {
        auto frequencies{classicFrequencies()};

        HuffmanTreeNaive tree{frequencies};
        const auto encodings{tree.getEncodings()};

        uint64_t weightedCost{0};

        for (const auto& [character, frequency] : frequencies) {
            weightedCost += frequency * encodings.at(character).size();
        }

        expectEqual(
            weightedCost,
            uint64_t{224},
            "Unexpected weighted encoding cost"
        );
    });

    suite.run("single-symbol input receives a usable code", [] {
        std::unordered_map<char, uint64_t> frequencies{
            {'x', 10}
        };

        HuffmanTreeNaive tree{frequencies};
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
        );
    });

    suite.printSummary();
    return suite;
}

// -----------------------------------------------------------------------------
// BitWriter tests
// -----------------------------------------------------------------------------

TestSuite runBitWriterTests() {
    TestSuite suite{"BitWriterNaive"};

    std::cout << "\n=== BitWriterNaive ===\n";

    suite.run("flushing without bits creates an empty file", [] {
        const auto bytes{
            writeBits("bitwriter_empty", {})
        };

        expect(
            bytes.empty(),
            "Expected an empty output file"
        );
    });

    suite.run("eight zero bits produce 0x00", [] {
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
        );
    });

    suite.run("eight one bits produce 0xFF", [] {
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
        );
    });

    suite.run("11110000 produces 0xF0", [] {
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
        );
    });

    suite.run("multiple complete bytes are written correctly", [] {
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
        );
    });

    suite.run("partial byte is padded on the right", [] {
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
        );
    });

    suite.run("bits crossing a byte boundary are preserved", [] {
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
        );
    });

    suite.printSummary();
    return suite;
}

} // namespace

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main() {
    try {
        fs::create_directories(TEST_OUTPUT_DIRECTORY);
    } catch (const fs::filesystem_error& exception) {
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
        runFrequencyTests()
    };

    const TestSuite huffmanSuite{
        runHuffmanEncodingTests()
    };

    const TestSuite bitWriterSuite{
        runBitWriterTests()
    };

    const std::size_t totalPassed{
        frequencySuite.passed()
        + huffmanSuite.passed()
        + bitWriterSuite.passed()
    };

    const std::size_t totalTests{
        frequencySuite.total()
        + huffmanSuite.total()
        + bitWriterSuite.total()
    };

    std::cout
        << "\n============================================\n"
        << " Final Result: "
        << totalPassed << '/' << totalTests
        << " test cases passed\n"
        << "============================================\n";

    if (totalPassed == totalTests) {
        std::cout << "ALL TESTS PASSED\n";
        return 0;
    }

    std::cout << "SOME TESTS FAILED\n";
    return 1;
}