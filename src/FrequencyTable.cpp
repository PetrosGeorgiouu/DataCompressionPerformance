#include <iostream>
#include <fstream>
#include <array>
#include "huffman/FrequencyTable.hpp"

using namespace std;

array <uint64_t, 256> findFrequencies(const string &txtPath)
{
  ifstream file(txtPath, ios::binary);
  array <uint64_t, 256> freqs{};
  char c;
  while (file.get(c))
  {
    ++freqs[c];
  }
  return freqs;
}