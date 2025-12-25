#include "Encryption.hpp"
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <fstream>
#include <iostream>
#include <stdexcept>

// 生成随机盐值
std::vector<uint8_t> Encryption::generateSalt() {
    std::vector<uint8_t> salt(16);
    if (RAND_bytes(salt.data(), salt.size()) != 1) {
        throw std::runtime_error("Failed to generate salt");
    }
    return salt;
}

// 用于AES加密的密钥派生函数
std::vector<uint8_t> Encryption::deriveKey(const std::string& password, const std::vector<uint8_t>& salt) {
    std::vector<uint8_t> key(32); // AES-256密钥长度
    if (PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                          salt.data(), salt.size(),
                          10000, // 迭代次数
                          EVP_sha256(),
                          key.size(), key.data()) != 1) {
        throw std::runtime_error("Failed to derive key");
    }
    return key;
}

// AES加密函数
bool Encryption::encryptAES(const std::vector<uint8_t>& plaintext, const std::vector<uint8_t>& key, 
                          std::vector<uint8_t>& ciphertext, std::vector<uint8_t>& iv) {
    // 生成随机IV
    iv.resize(16);
    if (RAND_bytes(iv.data(), iv.size()) != 1) {
        return false;
    }
    
    // 创建加密上下文
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return false;
    }
    
    // 初始化加密操作
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    
    // 分配足够的空间用于存储密文
    ciphertext.resize(plaintext.size() + AES_BLOCK_SIZE);
    int len;
    int ciphertext_len;
    
    // 执行加密
    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), plaintext.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    ciphertext_len = len;
    
    // 完成加密
    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    ciphertext_len += len;
    
    // 调整密文大小
    ciphertext.resize(ciphertext_len);
    
    // 清理
    EVP_CIPHER_CTX_free(ctx);
    return true;
}

// AES解密函数
bool Encryption::decryptAES(const std::vector<uint8_t>& ciphertext, const std::vector<uint8_t>& key, 
                          const std::vector<uint8_t>& iv, std::vector<uint8_t>& plaintext) {
    // 创建解密上下文
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return false;
    }
    
    // 初始化解密操作
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    
    // 分配足够的空间用于存储明文
    plaintext.resize(ciphertext.size());
    int len;
    int plaintext_len;
    
    // 执行解密
    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), ciphertext.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    plaintext_len = len;
    
    // 完成解密
    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    plaintext_len += len;
    
    // 调整明文大小
    plaintext.resize(plaintext_len);
    
    // 清理
    EVP_CIPHER_CTX_free(ctx);
    return true;
}

// 加密文件
bool Encryption::encryptFile(const std::string& inputFile, const std::string& outputFile, const std::string& password) {
    try {
        // 读取输入文件
        std::ifstream inFile(inputFile, std::ios::binary);
        if (!inFile) {
            std::cerr << "Failed to open input file: " << inputFile << std::endl;
            return false;
        }
        
        std::vector<uint8_t> plaintext(
            (std::istreambuf_iterator<char>(inFile)),
            (std::istreambuf_iterator<char>())
        );
        inFile.close();
        
        // 生成盐值
        std::vector<uint8_t> salt = generateSalt();
        
        // 派生密钥
        std::vector<uint8_t> key = deriveKey(password, salt);
        
        // 加密数据
        std::vector<uint8_t> ciphertext;
        std::vector<uint8_t> iv;
        if (!encryptAES(plaintext, key, ciphertext, iv)) {
            std::cerr << "Failed to encrypt data" << std::endl;
            return false;
        }
        
        // 写入输出文件
        std::ofstream outFile(outputFile, std::ios::binary);
        if (!outFile) {
            std::cerr << "Failed to open output file: " << outputFile << std::endl;
            return false;
        }
        
        // 写入盐值 (16字节)
        outFile.write(reinterpret_cast<const char*>(salt.data()), salt.size());
        
        // 写入IV (16字节)
        outFile.write(reinterpret_cast<const char*>(iv.data()), iv.size());
        
        // 写入密文
        outFile.write(reinterpret_cast<const char*>(ciphertext.data()), ciphertext.size());
        
        outFile.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Encryption error: " << e.what() << std::endl;
        return false;
    }
}

// 解密文件
bool Encryption::decryptFile(const std::string& inputFile, const std::string& outputFile, const std::string& password) {
    try {
        // 读取输入文件
        std::ifstream inFile(inputFile, std::ios::binary);
        if (!inFile) {
            std::cerr << "Failed to open input file: " << inputFile << std::endl;
            return false;
        }
        
        // 读取盐值 (16字节)
        std::vector<uint8_t> salt(16);
        inFile.read(reinterpret_cast<char*>(salt.data()), salt.size());
        
        // 读取IV (16字节)
        std::vector<uint8_t> iv(16);
        inFile.read(reinterpret_cast<char*>(iv.data()), iv.size());
        
        // 读取密文
        std::vector<uint8_t> ciphertext(
            (std::istreambuf_iterator<char>(inFile)),
            (std::istreambuf_iterator<char>())
        );
        inFile.close();
        
        // 派生密钥
        std::vector<uint8_t> key = deriveKey(password, salt);
        
        // 解密数据
        std::vector<uint8_t> plaintext;
        if (!decryptAES(ciphertext, key, iv, plaintext)) {
            std::cerr << "Failed to decrypt data" << std::endl;
            return false;
        }
        
        // 写入输出文件
        std::ofstream outFile(outputFile, std::ios::binary);
        if (!outFile) {
            std::cerr << "Failed to open output file: " << outputFile << std::endl;
            return false;
        }
        
        outFile.write(reinterpret_cast<const char*>(plaintext.data()), plaintext.size());
        outFile.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Decryption error: " << e.what() << std::endl;
        return false;
    }
}
