#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <iostream>
#include <fstream>

// Huffman节点结构体
struct HuffmanNode {
    char data;              // 存储的字符
    unsigned int freq;      // 字符出现频率
    HuffmanNode* left;      // 左子节点
    HuffmanNode* right;     // 右子节点

    HuffmanNode(char data, unsigned int freq) : data(data), freq(freq), left(nullptr), right(nullptr) {}
    ~HuffmanNode() {
        delete left;
        delete right;
    }
};

// 比较器，用于优先队列
struct Compare {
    bool operator()(HuffmanNode* l, HuffmanNode* r) {
        return l->freq > r->freq;
    }
};

class HuffmanCompressor {
public:
    HuffmanCompressor();
    ~HuffmanCompressor();

    // 压缩文件
    bool compressFile(const std::string& inputFilePath, const std::string& outputFilePath);

    // 解压文件
    bool decompressFile(const std::string& inputFilePath, const std::string& outputFilePath);

private:
    // 统计字符频率
    std::unordered_map<char, unsigned int> calculateFrequency(const std::string& data);

    // 构建Huffman树
    HuffmanNode* buildHuffmanTree(const std::unordered_map<char, unsigned int>& freqMap);

    // 生成Huffman编码
    void generateCodes(HuffmanNode* root, const std::string& code, std::unordered_map<char, std::string>& huffmanCodes);

    // 解码函数
    std::string decodeText(HuffmanNode* root, const std::string& encodedData);

    // 将位串转换为字节序列
    std::vector<unsigned char> bitStringToBytes(const std::string& bitString);

    // 将字节序列转换为位串
    std::string bytesToBitString(const std::vector<unsigned char>& bytes, int padding);

    // 写入Huffman树到文件
    void writeTreeToFile(HuffmanNode* root, std::ofstream& outFile);

    // 从文件中读取Huffman树
    HuffmanNode* readTreeFromFile(std::ifstream& inFile);

    // 辅助函数：将整数转换为字节数组
    void writeIntToFile(std::ofstream& outFile, unsigned int value);

    // 辅助函数：从文件中读取整数
    unsigned int readIntFromFile(std::ifstream& inFile);

    HuffmanNode* root;  // Huffman树的根节点
};
