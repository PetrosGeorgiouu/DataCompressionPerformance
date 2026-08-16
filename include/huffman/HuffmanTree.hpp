#pragma once

#include <iostream>
#include <queue>
#include <array>
#include <vector>
#include <unordered_map>
#include "huffman/BitWriter.hpp"
#include "huffman/FrequencyTable.hpp"

using namespace std;

class HuffmanTree
{
public:
  explicit HuffmanTree(array <uint64_t, 256> &freqs);

  ~HuffmanTree();

  HuffmanTree(const HuffmanTree &) = delete;
  HuffmanTree &operator=(const HuffmanTree &) = delete;

  unordered_map<char, string> getEncodings();

  void serialize(BitWriter &bitWriter);

private:
  struct Node;

  Node *root = nullptr;

  void getEncodingsHelper(
      const Node *node,
      string code,
      unordered_map<char, string> &encodings);

  void deleteHelper(Node *node);

  struct compare;

  void serializeHelper(BitWriter &bitWriter, Node *current);
};