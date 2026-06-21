#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct HuffmanNode {
    int val, freq;
    HuffmanNode* l;
    HuffmanNode* r;

    HuffmanNode(int v, int f)
        : val(v), freq(f), l(nullptr), r(nullptr) {}
};

void freeHuffmanTree(HuffmanNode* root);

std::vector<uint8_t> huffmanEncode(const std::vector<int>& data,
                                   HuffmanNode*& root,
                                   std::map<int, std::string>& code);
