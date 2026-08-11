#pragma once

#include <iostream>
#include <queue>
#include <unordered_map>
#include <vector>
#include "huffman/BitWriter.hpp"
#include "huffman/FrequencyTable.hpp"

using namespace std;

class HuffmanTree
{
public:
  explicit HuffmanTree(std::unordered_map<char, uint64_t> &freqs);

  ~HuffmanTree();

  HuffmanTree(const HuffmanTree &) = delete;
  HuffmanTree &operator=(const HuffmanTree &) = delete;

  std::unordered_map<char, std::string> getEncodings();

  void serialize(BitWriter &bitWriter);

private:
  struct Node;

  Node *root = nullptr;

  void getEncodingsHelper(
      const Node *node,
      std::string code,
      std::unordered_map<char, std::string> &encodings);

  void deleteHelper(Node *node);

  struct compare;

  void serializeHelper(BitWriter &bitWriter, Node *current);
};