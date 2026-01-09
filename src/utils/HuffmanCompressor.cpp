#include "HuffmanCompressor.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <queue>
#include <unordered_map>
#include <string>
#include <vector>
#include <filesystem>
namespace fs = std::filesystem;

HuffmanCompressor::HuffmanCompressor() : root(nullptr) {}

HuffmanCompressor::~HuffmanCompressor() {
    delete root;
}

std::unordered_map<char, unsigned int> HuffmanCompressor::calculateFrequency(const std::string& data) {
    std::unordered_map<char, unsigned int> freqMap;
    for (char ch : data) {
        freqMap[ch]++;
    }
    return freqMap;
}

// 构建Huffman树 - 简化版本，只处理unsigned char
HuffmanNode* HuffmanCompressor::buildHuffmanTree(const std::unordered_map<unsigned char, unsigned int>& freqMap) {
    // 创建优先队列（最小堆）
    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, Compare> minHeap;

    // 将所有字符及其频率加入优先队列
    for (const auto& pair : freqMap) {
        minHeap.push(new HuffmanNode(pair.first, pair.second));
    }

    // 处理空频率表的情况
    if (minHeap.empty()) {
        return nullptr;
    }

    // 构建Huffman树
    while (minHeap.size() > 1) {
        // 取出频率最小的两个节点
        HuffmanNode* left = minHeap.top();
        minHeap.pop();

        HuffmanNode* right = minHeap.top();
        minHeap.pop();

        // 创建一个内部节点，使用专门的内部节点构造函数
        HuffmanNode* newNode = new HuffmanNode(left->freq + right->freq, left, right);

        // 将新节点加入优先队列
        minHeap.push(newNode);
    }

    // 剩下的节点就是根节点
    return minHeap.top();
}

// 生成Huffman编码
void HuffmanCompressor::generateCodes(HuffmanNode* root, const std::string& code, std::unordered_map<unsigned char, std::string>& huffmanCodes) {
    if (root == nullptr) {
        return;
    }

    // 如果是叶子节点，保存其编码
    if (root->isLeaf) {
        huffmanCodes[root->data] = code;
        return;
    }

    // 递归遍历左右子树
    generateCodes(root->left, code + "0", huffmanCodes);
    generateCodes(root->right, code + "1", huffmanCodes);
}

// 将位串转换为字节序列
std::vector<unsigned char> HuffmanCompressor::bitStringToBytes(const std::string& bitString) {
    std::vector<unsigned char> bytes;
    unsigned char currentByte = 0;
    int bitCount = 0;

    for (char bit : bitString) {
        // 从高位到低位构建字节
        currentByte = (currentByte << 1) | (bit - '0');
        bitCount++;

        if (bitCount == 8) {
            bytes.push_back(currentByte);
            currentByte = 0;
            bitCount = 0;
        }
    }

    // 处理剩余的位
    if (bitCount > 0) {
        currentByte <<= (8 - bitCount);
        bytes.push_back(currentByte);
    }

    return bytes;
}

// 将字节序列转换为位串
std::string HuffmanCompressor::bytesToBitString(const std::vector<unsigned char>& bytes, int padding) {
    std::string bitString;
    
    for (unsigned char byte : bytes) {
        // 从高位到低位提取位
        for (int j = 7; j >= 0; j--) {
            bitString += ((byte >> j) & 1) ? '1' : '0';
        }
    }
    
    // 移除填充位
    if (padding > 0 && bitString.size() >= padding) {
        bitString = bitString.substr(0, bitString.size() - padding);
    }
    
    return bitString;
}

void HuffmanCompressor::writeIntToFile(std::ofstream& outFile, unsigned int value) {
    outFile.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

unsigned int HuffmanCompressor::readIntFromFile(std::ifstream& inFile) {
    unsigned int value;
    inFile.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

// 旧的char版本方法，保留以兼容现有代码
HuffmanNode* HuffmanCompressor::buildHuffmanTreeChar(const std::unordered_map<char, unsigned int>& freqMap) {
    // 转换为unsigned char频率表
    std::unordered_map<unsigned char, unsigned int> freqMapUnsigned;
    for (const auto& pair : freqMap) {
        freqMapUnsigned[static_cast<unsigned char>(pair.first)] = pair.second;
    }
    return buildHuffmanTree(freqMapUnsigned);
}

// 旧的unsigned char版本方法，保留以兼容现有代码
HuffmanNode* HuffmanCompressor::buildHuffmanTreeUnsignedChar(const std::unordered_map<unsigned char, unsigned int>& freqMap) {
    return buildHuffmanTree(freqMap);
}

// 旧的char版本方法，保留以兼容现有代码
void HuffmanCompressor::generateCodesChar(HuffmanNode* root, const std::string& code, std::unordered_map<char, std::string>& huffmanCodes) {
    // 生成unsigned char编码表
    std::unordered_map<unsigned char, std::string> huffmanCodesUnsigned;
    generateCodes(root, code, huffmanCodesUnsigned);
    
    // 转换为char编码表
    for (const auto& pair : huffmanCodesUnsigned) {
        huffmanCodes[static_cast<char>(pair.first)] = pair.second;
    }
}

// 旧的unsigned char版本方法，保留以兼容现有代码
void HuffmanCompressor::generateCodesUnsignedChar(HuffmanNode* root, const std::string& code, std::unordered_map<unsigned char, std::string>& huffmanCodes) {
    generateCodes(root, code, huffmanCodes);
}

bool HuffmanCompressor::compressFile(const std::string& inputFilePath, const std::string& outputFilePath) {
    // 初始化root为nullptr
    root = nullptr;
    
    try {
        // 1. 读取输入文件内容
        std::ifstream inFile(inputFilePath, std::ios::binary);
        if (!inFile.is_open()) {
            return false;
        }
        
        // 获取文件大小
        inFile.seekg(0, std::ios::end);
        std::streampos fileSize = inFile.tellg();
        inFile.seekg(0, std::ios::beg);
        
        // 读取文件内容
        std::vector<unsigned char> inputData(fileSize);
        inFile.read(reinterpret_cast<char*>(inputData.data()), fileSize);
        inFile.close();
        
        // 2. 写入压缩文件
        std::ofstream outFile(outputFilePath, std::ios::binary);
        if (!outFile.is_open()) {
            return false;
        }
        
        // 处理空文件
        if (inputData.empty()) {
            // 空文件的情况：写入0作为填充位
            outFile.put(0);
            // 写入0作为字符种类数（使用unsigned int，与正常情况一致）
            unsigned int charCount = 0;
            outFile.write(reinterpret_cast<const char*>(&charCount), sizeof(charCount));
            // 写入原始大小0
            writeIntToFile(outFile, 0);
            // 没有压缩数据
            outFile.close();
            return true;
        }
        
        // 计算字符频率
        std::unordered_map<unsigned char, unsigned int> freqMap;
        for (unsigned char ch : inputData) {
            freqMap[ch]++;
        }
        
        // 3. 构建Huffman树
        root = buildHuffmanTree(freqMap);
        if (root == nullptr) {
            outFile.close();
            return false;
        }
        
        // 4. 生成Huffman编码
        std::unordered_map<unsigned char, std::string> huffmanCodes;
        generateCodes(root, "", huffmanCodes);
        
        // 5. 构建压缩后的位串
        std::string encodedData;
        for (unsigned char ch : inputData) {
            encodedData += huffmanCodes[ch];
        }
        
        // 6. 计算需要添加的填充位数
        int padding = (8 - (encodedData.size() % 8)) % 8;
        
        // 7. 将位串转换为字节序列
        std::vector<unsigned char> bytes = bitStringToBytes(encodedData);
        
        // 8. 写入压缩文件
        
        // 写入填充位数
        outFile.put(static_cast<char>(padding));
        
        // 写入字符种类数（使用unsigned int以支持更多种类的字符）
        unsigned int charCount = static_cast<unsigned int>(freqMap.size());
        outFile.write(reinterpret_cast<const char*>(&charCount), sizeof(charCount));
        
        // 写入每个字符及其频率
        for (const auto& pair : freqMap) {
            outFile.put(pair.first);
            unsigned int freq = pair.second;
            outFile.write(reinterpret_cast<const char*>(&freq), sizeof(freq));
        }
        
        // 写入原始数据大小
        writeIntToFile(outFile, static_cast<unsigned int>(inputData.size()));
        
        // 写入压缩后的数据
        outFile.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        
        outFile.close();
        
        // 清理资源
        delete root;
        root = nullptr;
        
        return true;
        
    } catch (const std::exception& e) {
        delete root;
        root = nullptr;
        return false;
    }
}

bool HuffmanCompressor::decompressFile(const std::string& inputFilePath, const std::string& outputFilePath) {
    // 初始化root为nullptr
    root = nullptr;
    
    try {
        // 1. 读取压缩文件
        std::ifstream inFile(inputFilePath, std::ios::binary);
        if (!inFile.is_open()) {
            return false;
        }
        
        // 2. 读取填充位数
        char paddingChar;
        inFile.get(paddingChar);
        int padding = static_cast<unsigned char>(paddingChar);
        
        // 3. 读取字符种类数
        unsigned int charCount;
        inFile.read(reinterpret_cast<char*>(&charCount), sizeof(charCount));
        
        // 4. 读取频率表
        std::unordered_map<unsigned char, unsigned int> freqMap;
        for (unsigned int i = 0; i < charCount; i++) {
            char ch;
            inFile.get(ch);
            unsigned int freq;
            inFile.read(reinterpret_cast<char*>(&freq), sizeof(freq));
            freqMap[static_cast<unsigned char>(ch)] = freq;
        }
        
        // 5. 读取原始数据大小
        unsigned int originalSize = readIntFromFile(inFile);
        
        // 处理空文件
        if (originalSize == 0) {
            // 空文件的情况：直接创建空文件
            std::ofstream outFile(outputFilePath, std::ios::binary);
            if (!outFile.is_open()) {
                inFile.close();
                return false;
            }
            outFile.close();
            inFile.close();
            return true;
        }
        
        // 6. 重建Huffman树
        root = buildHuffmanTree(freqMap);
        if (root == nullptr) {
            inFile.close();
            return false;
        }
        
        // 7. 读取压缩后的数据
        // 先保存当前文件指针位置
        std::streampos currentPos = inFile.tellg();
        
        // 将文件指针移动到文件末尾获取文件大小
        inFile.seekg(0, std::ios::end);
        std::streampos fileSize = inFile.tellg();
        
        // 计算剩余数据大小
        std::streampos remainingSize = fileSize - currentPos;
        
        // 将文件指针恢复到原来的位置
        inFile.seekg(currentPos, std::ios::beg);
        
        // 读取剩余数据
        std::vector<unsigned char> compressedData(remainingSize);
        inFile.read(reinterpret_cast<char*>(compressedData.data()), remainingSize);
        inFile.close();
        
        // 8. 将压缩数据转换为位串
        std::string encodedData = bytesToBitString(compressedData, padding);
        
        // 9. 解码数据
        std::vector<unsigned char> decodedData;
        decodedData.reserve(originalSize);
        
        // 处理特殊情况：只有一个字符的情况
        if (root->isLeaf) {
            // 只有一个字符，直接填充originalSize个该字符
            decodedData.insert(decodedData.end(), originalSize, root->data);
        } else {
            // 正常解码
            HuffmanNode* current = root;
            for (size_t i = 0; i < encodedData.size() && decodedData.size() < originalSize; ++i) {
                char bit = encodedData[i];
                if (bit == '0') {
                    current = current->left;
                } else {
                    current = current->right;
                }
                
                // 确保current不为nullptr
                if (current == nullptr) {
                    break; // 遇到无效的编码，提前结束
                }
                
                // 如果到达叶子节点
                if (current->isLeaf) {
                    decodedData.push_back(current->data);
                    current = root;
                }
            }
        }
        
        // 10. 写入解压文件
        std::ofstream outFile(outputFilePath, std::ios::binary);
        if (!outFile.is_open()) {
            delete root;
            root = nullptr;
            return false;
        }
        
        outFile.write(reinterpret_cast<const char*>(decodedData.data()), decodedData.size());
        outFile.close();
        
        // 清理资源
        delete root;
        root = nullptr;
        
        return true;
        
    } catch (const std::exception& e) {
        delete root;
        root = nullptr;
        return false;
    }
}