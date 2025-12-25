#pragma once
#include <string>
#include <vector>

class Encryption {
private:
    // 用于AES加密的密钥派生函数
    static std::vector<uint8_t> deriveKey(const std::string& password, const std::vector<uint8_t>& salt);
    
    // 生成随机盐值
    static std::vector<uint8_t> generateSalt();
    
    // AES加密函数
    static bool encryptAES(const std::vector<uint8_t>& plaintext, const std::vector<uint8_t>& key, 
                          std::vector<uint8_t>& ciphertext, std::vector<uint8_t>& iv);
    
    // AES解密函数
    static bool decryptAES(const std::vector<uint8_t>& ciphertext, const std::vector<uint8_t>& key, 
                          const std::vector<uint8_t>& iv, std::vector<uint8_t>& plaintext);
    
public:
    // 加密文件
    static bool encryptFile(const std::string& inputFile, const std::string& outputFile, const std::string& password);
    
    // 解密文件
    static bool decryptFile(const std::string& inputFile, const std::string& outputFile, const std::string& password);
};
