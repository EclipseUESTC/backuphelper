#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <system_error>

namespace fs = std::filesystem;

class FileSystem {
public:
    // 检查文件或目录是否存在
    static bool exists(const std::string& path);

    // 创建目录（包括父目录）
    static bool createDirectories(const std::string& path);

    // 复制单个文件
    static bool copyFile(const std::string& source, const std::string& destination);

    // 获取目录中的所有文件（递归）
    static std::vector<std::string> getAllFiles(const std::string& directory);

    // 获取文件大小
    static uint64_t getFileSize(const std::string& filePath);

    // 获取相对路径
    static std::string getRelativePath(const std::string& path, const std::string& base);
};