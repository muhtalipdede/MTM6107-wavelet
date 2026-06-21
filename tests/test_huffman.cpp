#include "huffman.h"

#include <gtest/gtest.h>
#include <map>
#include <string>
#include <vector>

TEST(HuffmanEncodeTest, EmptyInputReturnsEmptyOutput)
{
    HuffmanNode* root = nullptr;
    std::map<int, std::string> code;

    std::vector<uint8_t> encoded = huffmanEncode({}, root, code);

    EXPECT_TRUE(encoded.empty());
    EXPECT_TRUE(code.empty());
    EXPECT_EQ(root, nullptr);
}

TEST(HuffmanEncodeTest, SingleSymbolUsesFallbackCode)
{
    std::vector<int> data(5, 42);

    HuffmanNode* root = nullptr;
    std::map<int, std::string> code;
    std::vector<uint8_t> encoded = huffmanEncode(data, root, code);

    ASSERT_EQ(code.size(), 1u);
    EXPECT_EQ(code[42], "0");
    EXPECT_FALSE(encoded.empty());
    freeHuffmanTree(root);
}

TEST(HuffmanEncodeTest, MultipleSymbolsProduceCodesForAllValues)
{
    std::vector<int> data = {1, 1, 2, 3, 3, 3};

    HuffmanNode* root = nullptr;
    std::map<int, std::string> code;
    std::vector<uint8_t> encoded = huffmanEncode(data, root, code);

    EXPECT_EQ(code.size(), 3u);
    EXPECT_FALSE(code[1].empty());
    EXPECT_FALSE(code[2].empty());
    EXPECT_FALSE(code[3].empty());
    EXPECT_FALSE(encoded.empty());
    freeHuffmanTree(root);
}

TEST(HuffmanEncodeTest, MoreDistinctSymbolsIncreaseCompressedSize)
{
    HuffmanNode* rootA = nullptr;
    HuffmanNode* rootB = nullptr;
    std::map<int, std::string> codeA;
    std::map<int, std::string> codeB;

    std::vector<int> lowEntropy(100, 7);
    std::vector<int> highEntropy;
    for (int i = 0; i < 100; i++) {
        highEntropy.push_back(i % 10);
    }

    std::vector<uint8_t> encodedLow = huffmanEncode(lowEntropy, rootA, codeA);
    std::vector<uint8_t> encodedHigh = huffmanEncode(highEntropy, rootB, codeB);

    EXPECT_LT(encodedLow.size(), encodedHigh.size());
    freeHuffmanTree(rootA);
    freeHuffmanTree(rootB);
}
