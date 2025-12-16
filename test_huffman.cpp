#include <iostream>
#include <string>
#include <filesystem>
#include "src/utils/HuffmanCompressor.hpp"
#include "src/utils/FileSystem.hpp"

int main() {
    std::cout << "=== Huffman Compression Test ===\n\n";
    
    // 创建测试文件
    std::string testFile = "test_huffman.txt";
    std::string compressedFile = "test_huffman.compressed";
    std::string decompressedFile = "test_huffman.decompressed";
    
    // 写入测试内容
    std::ofstream outFile(testFile);
    if (!outFile) {
        std::cerr << "Error creating test file!\n";
        return 1;
    }
    
    // 写入一些重复内容，这样压缩效果会更明显
    for (int i = 0; i < 1000; ++i) {
        outFile << "Huffman coding is a lossless data compression algorithm. It uses variable-length code words.\n";
    }
    outFile.close();
    
    std::cout << "Created test file: " << testFile << std::endl;
    std::cout << "Original size: " << std::filesystem::file_size(testFile) << " bytes\n";
    
    // 压缩文件
    std::cout << "\nCompressing file...\n";
    HuffmanCompressor compressor;
    if (!compressor.compressFile(testFile, compressedFile)) {
        std::cerr << "Error compressing file!\n";
        std::filesystem::remove(testFile);
        return 1;
    }
    
    std::cout << "Compressed file: " << compressedFile << std::endl;
    std::cout << "Compressed size: " << std::filesystem::file_size(compressedFile) << " bytes\n";
    
    // 计算压缩率
    double originalSize = std::filesystem::file_size(testFile);
    double compressedSize = std::filesystem::file_size(compressedFile);
    double compressionRatio = (1 - compressedSize / originalSize) * 100;
    std::cout << "Compression ratio: " << compressionRatio << "%\n";
    
    // 解压文件
    std::cout << "\nDecompressing file...\n";
    if (!compressor.decompressFile(compressedFile, decompressedFile)) {
        std::cerr << "Error decompressing file!\n";
        std::filesystem::remove(testFile);
        std::filesystem::remove(compressedFile);
        return 1;
    }
    
    std::cout << "Decompressed file: " << decompressedFile << std::endl;
    std::cout << "Decompressed size: " << std::filesystem::file_size(decompressedFile) << " bytes\n";
    
    // 验证文件内容是否一致
    std::cout << "\nVerifying file integrity...\n";
    
    std::ifstream original(testFile);
    std::ifstream decompressed(decompressedFile);
    
    std::string originalContent((std::istreambuf_iterator<char>(original)), std::istreambuf_iterator<char>());
    std::string decompressedContent((std::istreambuf_iterator<char>(decompressed)), std::istreambuf_iterator<char>());
    
    if (originalContent == decompressedContent) {
        std::cout << "✓ Success: Decompressed file matches original!\n";
    } else {
        std::cout << "✗ Error: Decompressed file does not match original!\n";
    }
    
    original.close();
    decompressed.close();
    
    // 清理测试文件
    std::filesystem::remove(testFile);
    std::filesystem::remove(compressedFile);
    std::filesystem::remove(decompressedFile);
    
    std::cout << "\n=== Test completed! ===\n";
    
    return 0;
}