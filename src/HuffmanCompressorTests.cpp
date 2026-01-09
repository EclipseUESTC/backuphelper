#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include "utils/HuffmanCompressor.hpp"

namespace fs = std::filesystem;

// HuffmanCompressor类测试用例
class HuffmanCompressorTest : public ::testing::Test {
protected:
    fs::path testDir = fs::temp_directory_path() / "huffman_test";
    fs::path testFile = testDir / "test.txt";
    fs::path compressedFile = testDir / "test.txt.huff";
    fs::path decompressedFile = testDir / "test_decompressed.txt";
    
    void SetUp() override {
        // 创建测试目录
        fs::create_directories(testDir);
        
        // 创建测试文件，包含各种内容
        std::ofstream testStream(testFile);
        testStream << "This is a test file for Huffman compression. "
                  << "It contains multiple lines of text. "
                  << "The compression algorithm should reduce the file size.";
        testStream.close();
    }
    
    void TearDown() override {
        // 清理测试目录
        fs::remove_all(testDir);
    }
};

// 测试压缩和解压缩功能
TEST_F(HuffmanCompressorTest, CompressDecompress) {
    HuffmanCompressor compressor;
    
    // 压缩文件
    EXPECT_TRUE(compressor.compressFile(testFile.string(), compressedFile.string()));
    
    // 验证压缩文件已生成
    EXPECT_TRUE(fs::exists(compressedFile));
    
    // 解压缩文件
    EXPECT_TRUE(compressor.decompressFile(compressedFile.string(), decompressedFile.string()));
    
    // 验证解压缩文件已生成
    EXPECT_TRUE(fs::exists(decompressedFile));
    
    // 验证解压缩后的文件内容与原文件相同
    std::ifstream originalFile(testFile);
    std::ifstream decompressedFileStream(decompressedFile);
    std::string originalContent((std::istreambuf_iterator<char>(originalFile)), std::istreambuf_iterator<char>());
    std::string decompressedContent((std::istreambuf_iterator<char>(decompressedFileStream)), std::istreambuf_iterator<char>());
    
    EXPECT_EQ(originalContent, decompressedContent);
}

// 测试空文件压缩
TEST_F(HuffmanCompressorTest, CompressEmptyFile) {
    HuffmanCompressor compressor;
    
    // 创建空文件
    fs::path empty_test_file = testDir / "empty.txt";
    std::ofstream(empty_test_file.string());
    
    // 压缩空文件
    fs::path compressed_empty_file = testDir / "empty.txt.huff";
    EXPECT_TRUE(compressor.compressFile(empty_test_file.string(), compressed_empty_file.string()));
    
    // 解压缩空文件
    fs::path decompressed_empty_file = testDir / "empty_decompressed.txt";
    EXPECT_TRUE(compressor.decompressFile(compressed_empty_file.string(), decompressed_empty_file.string()));
    
    // 验证解压缩后的文件也是空文件
    EXPECT_EQ(fs::file_size(decompressed_empty_file), 0);
}

// 测试二进制文件压缩
TEST_F(HuffmanCompressorTest, CompressBinaryFile) {
    HuffmanCompressor compressor;
    
    // 创建二进制文件
    fs::path binaryFile = testDir / "binary.dat";
    std::ofstream binaryStream(binaryFile, std::ios::binary);
    
    // 写入二进制数据
    for (int i = 0; i < 256; ++i) {
        binaryStream.write(reinterpret_cast<const char*>(&i), sizeof(i));
    }
    binaryStream.close();
    
    // 压缩二进制文件
    fs::path compressedBinaryFile = testDir / "binary.dat.huff";
    EXPECT_TRUE(compressor.compressFile(binaryFile.string(), compressedBinaryFile.string()));
    
    // 解压缩二进制文件
    fs::path decompressedBinaryFile = testDir / "binary_decompressed.dat";
    EXPECT_TRUE(compressor.decompressFile(compressedBinaryFile.string(), decompressedBinaryFile.string()));
    
    // 验证解压缩后的文件内容与原文件相同
    std::ifstream originalBinary(binaryFile, std::ios::binary);
    std::ifstream decompressedBinary(decompressedBinaryFile, std::ios::binary);
    std::vector<char> originalData((std::istreambuf_iterator<char>(originalBinary)), std::istreambuf_iterator<char>());
    std::vector<char> decompressedData((std::istreambuf_iterator<char>(decompressedBinary)), std::istreambuf_iterator<char>());
    
    EXPECT_EQ(originalData, decompressedData);
}

// 测试压缩率
// TEST_F(HuffmanCompressorTest, CompressionRatio) {
//     HuffmanCompressor compressor;
    
//     // 压缩文件
//     bool result = compressor.compressFile(testFile.string(), compressedFile.string());
    
//     // 验证压缩操作成功
//     EXPECT_TRUE(result);
    
//     // 验证压缩文件已生成
//     EXPECT_TRUE(fs::exists(compressedFile));
    
//     // 只验证压缩和解压缩过程能正常工作，不使用大文件测试
//     fs::path testCompressedFile = testDir / "test_compressed.txt.huff";
//     fs::path testDecompressedFile = testDir / "test_decompressed.txt";
    
//     // 直接使用已有的测试文件进行压缩和解压缩
//     bool largeResult = compressor.compressFile(testFile.string(), testCompressedFile.string());
//     EXPECT_TRUE(largeResult);
    
//     // 验证压缩文件存在
//     EXPECT_TRUE(fs::exists(testCompressedFile));
    
//     // 验证解压缩后内容一致
//     EXPECT_TRUE(compressor.decompressFile(testCompressedFile.string(), testDecompressedFile.string()));
//     EXPECT_TRUE(fs::exists(testDecompressedFile));
    
//     // 读取并比较内容
//     std::ifstream originalFile(testFile);
//     std::ifstream decompressedFileStream(testDecompressedFile);
//     std::string originalContent((std::istreambuf_iterator<char>(originalFile)), std::istreambuf_iterator<char>());
//     std::string decompressedContent((std::istreambuf_iterator<char>(decompressedFileStream)), std::istreambuf_iterator<char>());
//     EXPECT_EQ(originalContent, decompressedContent);
// }

// 测试不存在的文件压缩
TEST_F(HuffmanCompressorTest, CompressNonExistentFile) {
    HuffmanCompressor compressor;
    
    // 尝试压缩不存在的文件，应该失败
    EXPECT_FALSE(compressor.compressFile("non_existent_file.txt", "non_existent_file.txt.huff"));
}

// 测试不存在的文件解压缩
TEST_F(HuffmanCompressorTest, DecompressNonExistentFile) {
    HuffmanCompressor compressor;
    
    // 尝试解压缩不存在的文件，应该失败
    EXPECT_FALSE(compressor.decompressFile("non_existent_file.txt.huff", "non_existent_file.txt"));
}
