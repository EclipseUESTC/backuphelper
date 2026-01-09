#include "src/utils/HuffmanCompressor.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
    // 创建HuffmanCompressor实例
    HuffmanCompressor compressor;
    
    // 创建测试文件
    std::string testContent = "This is a test file for Huffman compression.";
    std::string inputFile = "test_input.txt";
    std::string compressedFile = "test_compressed.huff";
    std::string decompressedFile = "test_decompressed.txt";
    
    // 写入测试文件
    std::ofstream outFile(inputFile);
    outFile << testContent;
    outFile.close();
    
    std::cout << "Original content: " << testContent << std::endl;
    
    // 压缩文件
    if (compressor.compressFile(inputFile, compressedFile)) {
        std::cout << "Compression successful!" << std::endl;
        
        // 解压缩文件
        if (compressor.decompressFile(compressedFile, decompressedFile)) {
            std::cout << "Decompression successful!" << std::endl;
            
            // 读取解压缩后的内容
            std::ifstream inFile(decompressedFile);
            std::string decompressedContent((std::istreambuf_iterator<char>(inFile)), 
                                          std::istreambuf_iterator<char>());
            inFile.close();
            
            std::cout << "Decompressed content: " << decompressedContent << std::endl;
            
            // 比较内容
            if (testContent == decompressedContent) {
                std::cout << "SUCCESS: Original and decompressed content match!" << std::endl;
            } else {
                std::cout << "FAILURE: Original and decompressed content don't match!" << std::endl;
            }
        } else {
            std::cout << "Decompression failed!" << std::endl;
        }
    } else {
        std::cout << "Compression failed!" << std::endl;
    }
    
    // 清理临时文件
    fs::remove(inputFile);
    fs::remove(compressedFile);
    fs::remove(decompressedFile);
    
    return 0;
}