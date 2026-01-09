#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <iostream>
#include <fstream>

// Huffman节点结构体
struct HuffmanNode {
    unsigned char data;     // 存储的字符，使用unsigned char支持所有字节值
    unsigned int freq;      // 字符出现频率
    HuffmanNode* left;      // 左子节点
    HuffmanNode* right;     // 右子节点
    bool isLeaf;            // 标记是否为叶节点
    unsigned int id;        // 唯一ID，确保比较器的一致性
    static unsigned int nextId; // 静态计数器，用于生成唯一ID

    // 叶节点构造函数
    HuffmanNode(unsigned char data, unsigned int freq) 
        : data(data), freq(freq), left(nullptr), right(nullptr), isLeaf(true), id(nextId++) {}
    
    // 内部节点构造函数
    HuffmanNode(unsigned int freq, HuffmanNode* left, HuffmanNode* right) 
        : data(0), freq(freq), left(left), right(right), isLeaf(false), id(nextId++) {}
    
    ~HuffmanNode() {
        delete left;
        delete right;
    }
};

// 初始化静态计数器
unsigned int HuffmanNode::nextId = 0;

// 比较器，用于优先队列，确保Huffman树构建唯一
struct Compare {
    bool operator()(HuffmanNode* l, HuffmanNode* r) {
        // 当频率相同时，按字符值排序，确保生成的Huffman树结构唯一
        if (l->freq == r->freq) {
            // 叶节点优先于内部节点
            if (l->isLeaf && !r->isLeaf) {
                return false; // 叶节点l优先级高，排在前面
            }
            if (!l->isLeaf && r->isLeaf) {
                return true;  // 内部节点l优先级低，排在后面
            }
            // 对于叶节点，按字符值排序
            if (l->isLeaf && r->isLeaf) {
                return l->data > r->data;
            }
            // 对于内部节点，按唯一ID排序，确保Huffman树构建的一致性
            return l->id > r->id;
        }
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
    // 统计字符频率（char版本，兼容旧代码）
    std::unordered_map<char, unsigned int> calculateFrequency(const std::string& data);

    // 构建Huffman树
    HuffmanNode* buildHuffmanTree(const std::unordered_map<unsigned char, unsigned int>& freqMap);
    
    // 构建Huffman树（char版本，兼容旧代码）
    HuffmanNode* buildHuffmanTreeChar(const std::unordered_map<char, unsigned int>& freqMap);
    
    // 构建Huffman树（unsigned char版本，用于二进制文件处理）
    HuffmanNode* buildHuffmanTreeUnsignedChar(const std::unordered_map<unsigned char, unsigned int>& freqMap);

    // 生成Huffman编码
    void generateCodes(HuffmanNode* root, const std::string& code, std::unordered_map<unsigned char, std::string>& huffmanCodes);
    
    // 生成Huffman编码（char版本，兼容旧代码）
    void generateCodesChar(HuffmanNode* root, const std::string& code, std::unordered_map<char, std::string>& huffmanCodes);
    
    // 生成Huffman编码（unsigned char版本，用于二进制文件处理）
    void generateCodesUnsignedChar(HuffmanNode* root, const std::string& code, std::unordered_map<unsigned char, std::string>& huffmanCodes);

    // 将位串转换为字节序列
    std::vector<unsigned char> bitStringToBytes(const std::string& bitString);

    // 将字节序列转换为位串
    std::string bytesToBitString(const std::vector<unsigned char>& bytes, int padding);

    // 辅助函数：将整数转换为字节数组
    void writeIntToFile(std::ofstream& outFile, unsigned int value);

    // 辅助函数：从文件中读取整数
    unsigned int readIntFromFile(std::ifstream& inFile);

    HuffmanNode* root;  // Huffman树的根节点
};