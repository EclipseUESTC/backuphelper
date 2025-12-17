#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <system_error>

// 引入File类定义
#include "../core/models/File.hpp"

namespace fs = std::filesystem;

// 引入HuffmanCompressor类
class HuffmanCompressor;

class FileSystem {
public:
    // 检查文件或目录是否存在
    static bool exists(const std::string& path);

    // 创建目录（包括父目录）
    static bool createDirectories(const std::string& path);

    // 复制单个文件
    static bool copyFile(const std::string& source, const std::string& destination);

    // 压缩并复制文件
    static bool copyAndCompressFile(const std::string& source, const std::string& destination);

    // 解压并复制文件
    static bool decompressAndCopyFile(const std::string& source, const std::string& destination);

    // 获取目录中的所有文件（递归）
    static std::vector<File> getAllFiles(const std::string& directory);

    // 获取文件大小
    static uint64_t getFileSize(const std::string& filePath);

    // 获取相对路径
    static std::string getRelativePath(const std::string& path, const std::string& base);

    // 压缩单个文件
    static bool compressFile(const std::string& source, const std::string& destination);

    // 解压单个文件
    static bool decompressFile(const std::string& source, const std::string& destination);

    // 删除单个文件
    static bool removeFile(const std::string& path);
    
    // 清空目录内容
    static bool clearDirectory(const std::string& path);
    
    // 复制目录（递归）
    static bool copyDirectory(const std::string& sourceDir, const std::string& destDir);
};