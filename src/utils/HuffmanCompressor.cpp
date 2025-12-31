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

HuffmanNode* HuffmanCompressor::buildHuffmanTree(const std::unordered_map<unsigned char, unsigned int>& freqMap) {
    // 创建优先队列（最小堆）
    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, Compare> minHeap;

    // 将所有字符及其频率加入优先队列
    for (const auto& pair : freqMap) {
        minHeap.push(new HuffmanNode(static_cast<char>(pair.first), pair.second));
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

// generateCodes的char版本实现
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

// generateCodes的unsigned char版本实现
void HuffmanCompressor::generateCodes(HuffmanNode* root, const std::string& code, std::unordered_map<unsigned char, std::string>& huffmanCodes) {
    if (root == nullptr) {
        return;
    }

    // 如果是叶子节点，保存其编码
        if (root->left == nullptr && root->right == nullptr) {
            huffmanCodes[static_cast<unsigned char>(root->data)] = code;
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
        
        // 获取文件大小
        inFile.seekg(0, std::ios::end);
        std::streampos fileSize = inFile.tellg();
        inFile.seekg(0, std::ios::beg);
        
        // 对于小文件，直接复制，不进行压缩（减少元数据开销）
        // 调整阈值，对于非常小的文件，压缩后的元数据可能比原文件更大
        if (fileSize < 512) {
            inFile.close();
            std::ifstream src(inputFilePath, std::ios::binary);
            std::ofstream dst(outputFilePath, std::ios::binary);
            if (!src || !dst) {
                std::cerr << "Error: Could not copy small file" << std::endl;
                return false;
            }
            dst << src.rdbuf();
            return true;
        }
        
        std::vector<unsigned char> inputData(fileSize);
        inFile.read(reinterpret_cast<char*>(inputData.data()), fileSize);
        inFile.close();
        
        if (inputData.empty()) {
            std::cerr << "Error: Input file is empty." << std::endl;
            return false;
        }
        
        // 2. 统计字符频率
        std::unordered_map<unsigned char, unsigned int> freqMap;
        for (unsigned char ch : inputData) {
            freqMap[ch]++;
        }
        
        // 如果字符种类过多，Huffman压缩效果会很差
        // 调整阈值，对于字符种类超过128的文件，直接复制
        if (freqMap.size() > 128) {
            // 直接复制文件，不进行压缩
            std::ofstream outFile(outputFilePath, std::ios::binary);
            if (!outFile) {
                std::cerr << "Error: Could not open output file for direct copy." << std::endl;
                return false;
            }
            outFile.write(reinterpret_cast<const char*>(inputData.data()), inputData.size());
            outFile.close();
            return true;
        }
        
        // 3. 构建Huffman树
        root = buildHuffmanTree(freqMap);
        
        // 4. 生成Huffman编码
        std::unordered_map<unsigned char, std::string> huffmanCodes;
        generateCodes(root, "", huffmanCodes);
        
        // 5. 直接构建压缩后的字节数据，避免使用大型位串作为中间表示
        std::vector<unsigned char> bytes;
        unsigned char currentByte = 0;
        int bitCount = 0;
        
        // 遍历输入数据，直接生成压缩字节
        for (unsigned char ch : inputData) {
            const std::string& code = huffmanCodes[ch];
            for (char bit : code) {
                // 将位写入当前字节
                currentByte = (currentByte << 1) | (bit - '0');
                bitCount++;
                
                // 当当前字节已满时，添加到结果中
                if (bitCount == 8) {
                    bytes.push_back(currentByte);
                    currentByte = 0;
                    bitCount = 0;
                }
            }
        }
        
        // 处理剩余的位
        if (bitCount > 0) {
            // 填充剩余位为0
            currentByte <<= (8 - bitCount);
            bytes.push_back(currentByte);
        }
        
        // 计算压缩后总大小（包括元数据）
        // 实际元数据大小：填充位(1字节) + 字符种类数(1字节) + 频率表(每个字符5字节) + 原始大小(4字节)
        size_t metadataSize = 1 + 1 + (freqMap.size() * 5) + 4;
        size_t totalCompressedSize = metadataSize + bytes.size();
        
        // 如果压缩后的文件没有变小，直接复制原始文件
        if (totalCompressedSize >= inputData.size()) {
            std::ofstream outFile(outputFilePath, std::ios::binary);
            if (!outFile) {
                std::cerr << "Error: Could not open output file for direct copy." << std::endl;
                delete root;
                root = nullptr;
                return false;
            }
            outFile.write(reinterpret_cast<const char*>(inputData.data()), inputData.size());
            outFile.close();
            delete root;
            root = nullptr;
            return true;
        }
        
        // 7. 写入压缩文件
        std::ofstream outFile(outputFilePath, std::ios::binary);
        if (!outFile.is_open()) {
            std::cerr << "Error: Could not open output file: " << outputFilePath << std::endl;
            delete root;
            root = nullptr;
            return false;
        }
        
        // 写入填充位数
        // 计算填充的位数，即最后一个字节中未使用的位的数量
        int padding = (bitCount > 0) ? (8 - bitCount) % 8 : 0;
        outFile.put(static_cast<char>(padding));
        
        // 优化：使用更紧凑的方式存储频率表，而不是完整的Huffman树
        // 写入字符种类数
        unsigned char charCount = static_cast<unsigned char>(freqMap.size());
        outFile.put(charCount);
        
        // 写入每个字符及其频率
        for (const auto& pair : freqMap) {
            outFile.put(pair.first);
            // 使用32位无符号整数存储频率
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
        
        // 3. 读取字符种类数
        char charCountChar;
        inFile.get(charCountChar);
        unsigned char charCount = static_cast<unsigned char>(charCountChar);
        
        // 4. 读取频率表
        std::unordered_map<unsigned char, unsigned int> freqMap;
        for (unsigned char i = 0; i < charCount; i++) {
            char ch;
            inFile.get(ch);
            unsigned int freq;
            inFile.read(reinterpret_cast<char*>(&freq), sizeof(freq));
            freqMap[static_cast<unsigned char>(ch)] = freq;
        }
        
        // 5. 重建Huffman树
        root = buildHuffmanTree(freqMap);
        
        // 6. 读取原始数据大小
        unsigned int originalSize = readIntFromFile(inFile);
        
        // 7. 读取压缩后的数据
        std::vector<unsigned char> compressedData(std::istreambuf_iterator<char>(inFile), {});
        inFile.close();
        
        // 8. 将压缩数据转换为位串
        std::string encodedData = bytesToBitString(compressedData, padding);
        
        // 9. 解码数据
        std::vector<unsigned char> decodedData;
        decodedData.reserve(originalSize);
        
        HuffmanNode* current = root;
        for (char bit : encodedData) {
            if (bit == '0') {
                current = current->left;
            } else {
                current = current->right;
            }
            
            // 如果到达叶子节点
            if (current->left == nullptr && current->right == nullptr) {
                decodedData.push_back(static_cast<unsigned char>(current->data));
                current = root; // 回到根节点继续解码
                
                // 提前结束解码，避免处理多余的填充位
                if (decodedData.size() == originalSize) {
                    break;
                }
            }
        }
        
        // 验证解码结果大小
        if (decodedData.size() != originalSize) {
            std::cerr << "Error: Decoded data size mismatch!" << std::endl;
            delete root;
            root = nullptr;
            return false;
        }
        
        // 10. 写入解压文件
        std::ofstream outFile(outputFilePath, std::ios::binary);
        if (!outFile.is_open()) {
            std::cerr << "Error: Could not open output file: " << outputFilePath << std::endl;
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
        std::cerr << "Error during decompression: " << e.what() << std::endl;
        delete root;
        root = nullptr;
        return false;
    }
}
