#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include "utils/Encryption.hpp"

namespace fs = std::filesystem;

// Encryption类测试用例
class EncryptionTest : public ::testing::Test {
protected:
    fs::path testDir = fs::temp_directory_path() / "backup_encryption_test";
    fs::path plaintextFile = testDir / "plaintext.txt";
    fs::path encryptedFile = testDir / "encrypted.enc";
    fs::path decryptedFile = testDir / "decrypted.txt";
    std::string testPassword = "StrongPassword123!";
    std::string wrongPassword = "WrongPassword!";
    
    void SetUp() override {
        // 创建测试目录
        fs::create_directories(testDir);
        
        // 创建测试文件，包含一些文本内容
        std::ofstream(plaintextFile) << "This is a test file for encryption.\n" 
                                      << "It contains multiple lines of text.\n" 
                                      << "Line 3: Testing encryption and decryption.\n" 
                                      << "Line 4: Last line of the test file.";
    }
    
    void TearDown() override {
        // 清理测试目录
        fs::remove_all(testDir);
    }
    
    // 辅助函数：读取文件内容
    std::string readFileContent(const fs::path& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return "";
        }
        return std::string((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    }
    
    // 辅助函数：比较两个文件的内容
    bool compareFiles(const fs::path& file1, const fs::path& file2) {
        return readFileContent(file1) == readFileContent(file2);
    }
};

// 测试文件加密和解密功能
TEST_F(EncryptionTest, EncryptDecryptFile) {
    // 加密文件
    EXPECT_TRUE(Encryption::encryptFile(plaintextFile.string(), encryptedFile.string(), testPassword));
    
    // 验证加密文件已生成
    EXPECT_TRUE(fs::exists(encryptedFile));
    EXPECT_GT(fs::file_size(encryptedFile), 0);
    
    // 验证加密文件内容与原文件不同
    EXPECT_FALSE(compareFiles(plaintextFile, encryptedFile));
    
    // 解密文件
    EXPECT_TRUE(Encryption::decryptFile(encryptedFile.string(), decryptedFile.string(), testPassword));
    
    // 验证解密文件已生成
    EXPECT_TRUE(fs::exists(decryptedFile));
    EXPECT_GT(fs::file_size(decryptedFile), 0);
    
    // 验证解密后的文件内容与原文件相同
    EXPECT_TRUE(compareFiles(plaintextFile, decryptedFile));
}

// 测试错误密码解密
TEST_F(EncryptionTest, DecryptWithWrongPassword) {
    // 首先加密文件
    EXPECT_TRUE(Encryption::encryptFile(plaintextFile.string(), encryptedFile.string(), testPassword));
    
    // 使用错误密码尝试解密
    EXPECT_FALSE(Encryption::decryptFile(encryptedFile.string(), decryptedFile.string(), wrongPassword));
    
    // 验证解密失败时不会生成文件或生成的文件不完整
    if (fs::exists(decryptedFile)) {
        EXPECT_LT(fs::file_size(decryptedFile), fs::file_size(plaintextFile));
        EXPECT_FALSE(compareFiles(plaintextFile, decryptedFile));
    }
}

// 测试加密空文件
TEST_F(EncryptionTest, EncryptDecryptEmptyFile) {
    // 创建空文件
    fs::path empty_test_file = testDir / "empty.txt";
    std::ofstream(empty_test_file).close();
    
    // 加密空文件
    EXPECT_TRUE(Encryption::encryptFile(empty_test_file.string(), encryptedFile.string(), testPassword));
    
    // 验证加密文件已生成
    EXPECT_TRUE(fs::exists(encryptedFile));
    
    // 解密空文件
    EXPECT_TRUE(Encryption::decryptFile(encryptedFile.string(), decryptedFile.string(), testPassword));
    
    // 验证解密文件已生成且为空
    EXPECT_TRUE(fs::exists(decryptedFile));
    EXPECT_EQ(fs::file_size(decryptedFile), 0);
}

// 测试不存在的文件加密
TEST_F(EncryptionTest, EncryptNonExistentFile) {
    // 尝试加密不存在的文件
    fs::path nonExistentFile = testDir / "non_existent.txt";
    EXPECT_FALSE(Encryption::encryptFile(nonExistentFile.string(), encryptedFile.string(), testPassword));
    
    // 验证加密文件未生成
    EXPECT_FALSE(fs::exists(encryptedFile));
}

// 测试不存在的文件解密
TEST_F(EncryptionTest, DecryptNonExistentFile) {
    // 尝试解密不存在的文件
    fs::path nonExistentFile = testDir / "non_existent.enc";
    EXPECT_FALSE(Encryption::decryptFile(nonExistentFile.string(), decryptedFile.string(), testPassword));
    
    // 验证解密文件未生成
    EXPECT_FALSE(fs::exists(decryptedFile));
}

// 测试相同内容的文件加密结果是否不同（由于盐值随机）
TEST_F(EncryptionTest, SameContentDifferentEncryptionResults) {
    // 创建两个相同内容的文件
    fs::path plaintextFile2 = testDir / "plaintext2.txt";
    std::ofstream(plaintextFile2) << readFileContent(plaintextFile);
    
    // 加密第一个文件
    fs::path encryptedFile1 = testDir / "encrypted1.enc";
    EXPECT_TRUE(Encryption::encryptFile(plaintextFile.string(), encryptedFile1.string(), testPassword));
    
    // 加密第二个文件
    fs::path encryptedFile2 = testDir / "encrypted2.enc";
    EXPECT_TRUE(Encryption::encryptFile(plaintextFile2.string(), encryptedFile2.string(), testPassword));
    
    // 验证两个加密文件内容不同（由于盐值随机）
    EXPECT_FALSE(compareFiles(encryptedFile1, encryptedFile2));
    
    // 但解密后应该相同
    fs::path decryptedFile1 = testDir / "decrypted1.txt";
    fs::path decryptedFile2 = testDir / "decrypted2.txt";
    EXPECT_TRUE(Encryption::decryptFile(encryptedFile1.string(), decryptedFile1.string(), testPassword));
    EXPECT_TRUE(Encryption::decryptFile(encryptedFile2.string(), decryptedFile2.string(), testPassword));
    EXPECT_TRUE(compareFiles(decryptedFile1, decryptedFile2));
    EXPECT_TRUE(compareFiles(plaintextFile, decryptedFile1));
}

// 测试不同密码对相同文件的加密结果
TEST_F(EncryptionTest, DifferentPasswordsDifferentResults) {
    // 使用不同密码加密相同文件
    fs::path encryptedFile1 = testDir / "encrypted_pwd1.enc";
    fs::path encryptedFile2 = testDir / "encrypted_pwd2.enc";
    
    EXPECT_TRUE(Encryption::encryptFile(plaintextFile.string(), encryptedFile1.string(), testPassword));
    EXPECT_TRUE(Encryption::encryptFile(plaintextFile.string(), encryptedFile2.string(), wrongPassword));
    
    // 验证加密结果不同
    EXPECT_FALSE(compareFiles(encryptedFile1, encryptedFile2));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
