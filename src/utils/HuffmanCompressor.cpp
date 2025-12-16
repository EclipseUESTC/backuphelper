#include "HuffmanCompressor.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <queue>
#include <unordered_map>
#include <string>
#include <vector>

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

HuffmanNode* HuffmanCompressor::buildHuffmanTree(const std::unordered_map<char, unsigned int>& freqMap) {
    // 创建优先队列（最小堆）
    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, Compare> minHeap;

    // 将所有字符及其频率加入优先队列
    for (const auto& pair : freqMap) {
        minHeap.push(new HuffmanNode(pair.first, pair.second));
    }

    // 构建Huffman树
    while (minHeap.size() > 1) {
        // 取出频率最小的两个节点
        HuffmanNode* left = minHeap.top();
        minHeap.pop();

        HuffmanNode* right = minHeap.top();
        minHeap.pop();

        // 创建一个新节点，频率为两个节点的频率之和
        // 使用特殊字符'$'作为内部节点的标记
        HuffmanNode* newNode = new HuffmanNode('$', left->freq + right->freq);
        newNode->left = left;
        newNode->right = right;

        // 将新节点加入优先队列
        minHeap.push(newNode);
    }

    // 剩下的节点就是根节点
    return minHeap.top();
}

void HuffmanCompressor::generateCodes(HuffmanNode* root, const std::string& code, std::unordered_map<char, std::string>& huffmanCodes) {
    if (root == nullptr) {
        return;
    }

    // 如果是叶子节点，保存其编码
    if (root->left == nullptr && root->right == nullptr) {
        huffmanCodes[root->data] = code;
        return;
    }

    // 递归遍历左右子树
    generateCodes(root->left, code + "0", huffmanCodes);
    generateCodes(root->right, code + "1", huffmanCodes);
}

std::string HuffmanCompressor::decodeText(HuffmanNode* root, const std::string& encodedData) {
    std::string decodedText;
    HuffmanNode* current = root;

    for (char bit : encodedData) {
        if (bit == '0') {
            current = current->left;
        } else {
            current = current->right;
        }

        // 如果到达叶子节点
        if (current->left == nullptr && current->right == nullptr) {
            decodedText += current->data;
            current = root; // 回到根节点继续解码
        }
    }

    return decodedText;
}

std::vector<unsigned char> HuffmanCompressor::bitStringToBytes(const std::string& bitString) {
    std::vector<unsigned char> bytes;
    
    // 将位串按8位分组转换为字节
    for (size_t i = 0; i < bitString.length(); i += 8) {
        std::string byteString = bitString.substr(i, 8);
        // 处理不足8位的情况（最后一个字节）
        while (byteString.length() < 8) {
            byteString += "0";
        }
        
        unsigned char byte = 0;
        for (char bit : byteString) {
            byte = (byte << 1) | (bit - '0');
        }
        bytes.push_back(byte);
    }
    
    return bytes;
}

std::string HuffmanCompressor::bytesToBitString(const std::vector<unsigned char>& bytes, int padding) {
    std::string bitString;
    
    for (size_t i = 0; i < bytes.size(); i++) {
        unsigned char byte = bytes[i];
        std::string byteStr;
        
        // 将字节转换为8位的位串
        for (int j = 7; j >= 0; j--) {
            byteStr += ((byte >> j) & 1) ? '1' : '0';
        }
        
        // 处理最后一个字节的填充位
        if (i == bytes.size() - 1 && padding > 0) {
            byteStr = byteStr.substr(0, 8 - padding);
        }
        
        bitString += byteStr;
    }
    
    return bitString;
}

void HuffmanCompressor::writeTreeToFile(HuffmanNode* root, std::ofstream& outFile) {
    if (root == nullptr) {
        return;
    }
    
    // 如果是叶子节点，写入'1'和字符
    if (root->left == nullptr && root->right == nullptr) {
        outFile.put('1');
        outFile.put(root->data);
    } else {
        // 如果是内部节点，写入'0'并递归写入左右子树
        outFile.put('0');
        writeTreeToFile(root->left, outFile);
        writeTreeToFile(root->right, outFile);
    }
}

HuffmanNode* HuffmanCompressor::readTreeFromFile(std::ifstream& inFile) {
    char bit;
    inFile.get(bit);
    
    if (bit == '1') {
        // 叶子节点，读取字符
        char data;
        inFile.get(data);
        return new HuffmanNode(data, 0);
    } else {
        // 内部节点，递归读取左右子树
        HuffmanNode* left = readTreeFromFile(inFile);
        HuffmanNode* right = readTreeFromFile(inFile);
        HuffmanNode* node = new HuffmanNode('$', 0);
        node->left = left;
        node->right = right;
        return node;
    }
}

void HuffmanCompressor::writeIntToFile(std::ofstream& outFile, unsigned int value) {
    outFile.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

unsigned int HuffmanCompressor::readIntFromFile(std::ifstream& inFile) {
    unsigned int value;
    inFile.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

bool HuffmanCompressor::compressFile(const std::string& inputFilePath, const std::string& outputFilePath) {
    try {
        // 1. 读取输入文件内容
        std::ifstream inFile(inputFilePath, std::ios::binary);
        if (!inFile.is_open()) {
            std::cerr << "Error: Could not open input file: " << inputFilePath << std::endl;
            return false;
        }
        
        std::ostringstream ss;
        ss << inFile.rdbuf();
        std::string inputData = ss.str();
        inFile.close();
        
        if (inputData.empty()) {
            std::cerr << "Error: Input file is empty." << std::endl;
            return false;
        }
        
        // 2. 统计字符频率
        std::unordered_map<char, unsigned int> freqMap = calculateFrequency(inputData);
        
        // 3. 构建Huffman树
        root = buildHuffmanTree(freqMap);
        
        // 4. 生成Huffman编码
        std::unordered_map<char, std::string> huffmanCodes;
        generateCodes(root, "", huffmanCodes);
        
        // 5. 使用Huffman编码压缩数据
        std::string encodedData;
        for (char ch : inputData) {
            encodedData += huffmanCodes[ch];
        }
        
        // 6. 将编码后的数据转换为字节序列
        std::vector<unsigned char> bytes = bitStringToBytes(encodedData);
        
        // 7. 写入压缩文件
        std::ofstream outFile(outputFilePath, std::ios::binary);
        if (!outFile.is_open()) {
            std::cerr << "Error: Could not open output file: " << outputFilePath << std::endl;
            delete root;
            root = nullptr;
            return false;
        }
        
        // 写入填充位数
        int padding = (8 - (encodedData.size() % 8)) % 8;
        outFile.put(static_cast<char>(padding));
        
        // 写入Huffman树
        writeTreeToFile(root, outFile);
        
        // 写入原始数据大小（用于验证解压结果）
        writeIntToFile(outFile, static_cast<unsigned int>(inputData.size()));
        
        // 写入压缩后的数据
        outFile.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        
        outFile.close();
        
        // 清理资源
        delete root;
        root = nullptr;
        
        std::cout << "Compression successful!" << std::endl;
        std::cout << "Original size: " << inputData.size() << " bytes" << std::endl;
        std::cout << "Compressed size: " << bytes.size() + 1 + static_cast<unsigned int>(2 * freqMap.size() - 1) + sizeof(unsigned int) << " bytes" << std::endl;
        std::cout << "Compression ratio: " << (double)bytes.size() / inputData.size() * 100 << "%" << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error during compression: " << e.what() << std::endl;
        delete root;
        root = nullptr;
        return false;
    }
}

bool HuffmanCompressor::decompressFile(const std::string& inputFilePath, const std::string& outputFilePath) {
    try {
        // 1. 读取压缩文件
        std::ifstream inFile(inputFilePath, std::ios::binary);
        if (!inFile.is_open()) {
            std::cerr << "Error: Could not open input file: " << inputFilePath << std::endl;
            return false;
        }
        
        // 2. 读取填充位数
        char paddingChar;
        inFile.get(paddingChar);
        int padding = static_cast<unsigned char>(paddingChar);
        
        // 3. 读取Huffman树
        root = readTreeFromFile(inFile);
        
        // 4. 读取原始数据大小
        unsigned int originalSize = readIntFromFile(inFile);
        
        // 5. 读取压缩后的数据
        std::vector<unsigned char> compressedData(std::istreambuf_iterator<char>(inFile), {});
        inFile.close();
        
        // 6. 将压缩数据转换为位串
        std::string encodedData = bytesToBitString(compressedData, padding);
        
        // 7. 解码数据
        std::string decodedData = decodeText(root, encodedData);
        
        // 验证解码结果大小
        if (decodedData.size() != originalSize) {
            std::cerr << "Error: Decoded data size mismatch!" << std::endl;
            delete root;
            root = nullptr;
            return false;
        }
        
        // 8. 写入解压文件
        std::ofstream outFile(outputFilePath, std::ios::binary);
        if (!outFile.is_open()) {
            std::cerr << "Error: Could not open output file: " << outputFilePath << std::endl;
            delete root;
            root = nullptr;
            return false;
        }
        
        outFile.write(decodedData.c_str(), decodedData.size());
        outFile.close();
        
        // 清理资源
        delete root;
        root = nullptr;
        
        std::cout << "Decompression successful!" << std::endl;
        std::cout << "Decompressed size: " << decodedData.size() << " bytes" << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error during decompression: " << e.what() << std::endl;
        delete root;
        root = nullptr;
        return false;
    }
}
