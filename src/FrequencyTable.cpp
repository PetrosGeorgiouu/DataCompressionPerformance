#include <iostream>
#include <fstream>
#include <unordered_map>
#include "huffman/FrequencyTable.hpp"

using namespace std;

unordered_map<char, uint64_t> findFrequencies(const string &txtPath)
{
  ifstream file(txtPath, ios::binary);
  unordered_map<char, uint64_t> freqs;
  char c;
  while (file.get(c))
  {
    ++freqs[c];
  }
  return freqs;
}