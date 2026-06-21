#include "huffman.h"

#include <queue>
#include <string>

namespace {

struct NodeComparator {
    // Min-heap icin dusuk frekansli dugumlerin once cikmasini saglar.
    bool operator()(HuffmanNode* a, HuffmanNode* b) const
    {
        return a->freq > b->freq;
    }
};

// Huffman agacini gezerek her sembol icin ikili kod tablosu olusturur.
void buildCodes(HuffmanNode* root, const std::string& prefix, std::map<int, std::string>& code)
{
    if (!root) return;

    if (!root->l && !root->r) {
        code[root->val] = prefix.empty() ? "0" : prefix;
        return;
    }

    buildCodes(root->l, prefix + "0", code);
    buildCodes(root->r, prefix + "1", code);
}

}  // namespace

// Huffman agaci icin ayrilan bellegi serbest birakir.
void freeHuffmanTree(HuffmanNode* root)
{
    if (!root) return;
    freeHuffmanTree(root->l);
    freeHuffmanTree(root->r);
    delete root;
}

// Katsayilari frekans tabanli Huffman kodlariyla sikistirir ve byte dizisi dondurur.
//   p_i = f_i / N,  L_avg = sum_i p_i * l_i  (l_i: sembol i icin kod uzunlugu)
//   B_compressed = ceil(L_total / 8)
std::vector<uint8_t> huffmanEncode(const std::vector<int>& data,
                                   HuffmanNode*& root,
                                   std::map<int, std::string>& code)
{
    code.clear();
    root = nullptr;

    if (data.empty()) {
        return {};
    }

    std::map<int, int> freq;
    for (int v : data) {
        freq[v]++;
    }

    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, NodeComparator> pq;
    for (const auto& entry : freq) {
        pq.push(new HuffmanNode(entry.first, entry.second));
    }

    while (pq.size() > 1) {
        HuffmanNode* a = pq.top();
        pq.pop();
        HuffmanNode* b = pq.top();
        pq.pop();

        HuffmanNode* parent = new HuffmanNode(-1, a->freq + b->freq);
        parent->l = a;
        parent->r = b;
        pq.push(parent);
    }

    root = pq.top();
    buildCodes(root, "", code);

    std::string bits;
    bits.reserve(data.size() * 4);
    for (int v : data) {
        bits += code.at(v);
    }

    while (bits.size() % 8 != 0) {
        bits.push_back('0');
    }

    std::vector<uint8_t> out;
    out.reserve(bits.size() / 8);
    for (size_t i = 0; i < bits.size(); i += 8) {
        uint8_t b = 0;
        for (int j = 0; j < 8; j++) {
            if (bits[i + j] == '1') {
                b |= static_cast<uint8_t>(1 << (7 - j));
            }
        }
        out.push_back(b);
    }

    return out;
}
