#include "FileSystem.hpp"
#include "../core/models/File.hpp"
#include "HuffmanCompressor.hpp"
#include <iostream>  // 仅用于调试（可选），正式版可移除
#include <stdexcept>

namespace fs = std::filesystem;

bool FileSystem::exists(const std::string& path) {
    std::error_code ec;
    bool result = fs::exists(path, ec);
    // 不抛出异常，静默处理错误（如权限不足）
    return result && !ec;
}

bool FileSystem::createDirectories(const std::string& path) {
    // 如果目录已存在，直接返回成功
    std::error_code ec;
    if (fs::exists(path, ec) && fs::is_directory(path, ec)) {
        return true;
    }
    
    // 否则创建目录
    bool result = fs::create_directories(path, ec);
    if (ec) {
        std::cerr << "Error: Failed to create directories for " << path << " (" << ec.message() << ")" << std::endl;
    }
    return result && !ec;
}

bool FileSystem::copyFile(const std::string& source, const std::string& destination) {
    // 确保目标目录存在
    fs::path destPath(destination);
    if (!destPath.parent_path().empty()) {
        if (!createDirectories(destPath.parent_path().string())) {
            std::cerr << "Error: Failed to create directories for " << destPath.parent_path().string() << std::endl;
            return false;
        }
    }

    std::error_code ec;
    fs::copy_file(source, destination, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        std::cerr << "Error: Failed to copy file from " << source << " to " << destination << " (" << ec.message() << ")" << std::endl;
        return false;
    }
    return true;
}

std::vector<File> FileSystem::getAllFiles(const std::string& directory) {
    std::vector<File> files;
    std::error_code ec;

    if (!fs::exists(directory, ec) || !fs::is_directory(directory, ec)) {
        return files; // 返回空列表
    }

    try {
        for (const auto& entry : fs::recursive_directory_iterator(directory, ec)) {
            if (ec) break;
            if (entry.is_regular_file()) {
                files.emplace_back(entry.path());
            }
        }
    } catch (const fs::filesystem_error&) {
        // 忽略异常，返回已收集的文件（或空）
    }
    return files;
}

uint64_t FileSystem::getFileSize(const std::string& filePath) {
    std::error_code ec;
    auto size = fs::file_size(filePath, ec);
    return ec ? 0 : size;
}

std::string FileSystem::getRelativePath(const std::string& path, const std::string& base) {
    try {
        fs::path p(path);
        fs::path b(base);
        // 确保 base 是目录（末尾带 /）
        if (!b.has_filename()) {
            b = b.parent_path();
        }
        fs::path relative = fs::relative(p, b);
        return relative.string();
    } catch (const fs::filesystem_error&) {
        // 若无法计算相对路径，返回原路径（或 basename）
        return fs::path(path).filename().string();
    }
}

bool FileSystem::compressFile(const std::string& source, const std::string& destination) {
    HuffmanCompressor compressor;
    return compressor.compressFile(source, destination);
}

bool FileSystem::decompressFile(const std::string& source, const std::string& destination) {
    HuffmanCompressor compressor;
    return compressor.decompressFile(source, destination);
}

bool FileSystem::copyAndCompressFile(const std::string& source, const std::string& destination) {
    // 使用HuffmanCompressor压缩文件
    return compressFile(source, destination);
}

bool FileSystem::decompressAndCopyFile(const std::string& source, const std::string& destination) {
    // 使用HuffmanCompressor解压文件
    return decompressFile(source, destination);
}

bool FileSystem::removeFile(const std::string& path) {
    std::error_code ec;
    bool result = fs::remove(path, ec);
    return !ec && result;
}

// 清空目录内容
bool FileSystem::clearDirectory(const std::string& path) {
    std::error_code ec;
    
    // 检查目录是否存在
    if (!fs::exists(path, ec)) {
        std::cerr << "clearDirectory: Directory does not exist: " << path << ", Error: " << ec.message() << std::endl;
        return false;
    }
    if (!fs::is_directory(path, ec)) {
        std::cerr << "clearDirectory: Path is not a directory: " << path << ", Error: " << ec.message() << std::endl;
        return false;
    }
    
    // 遍历目录并删除所有内容
    for (const auto& entry : fs::directory_iterator(path, ec)) {
        if (ec) {
            std::cerr << "clearDirectory: Failed to iterate directory: " << path << ", Error: " << ec.message() << std::endl;
            return false;
        }
        
        const auto& entryPath = entry.path();
        if (entry.is_directory(ec)) {
            if (!fs::remove_all(entryPath, ec)) {
                // 检查是否真的有错误
                if (ec) {
                    std::cerr << "clearDirectory: Failed to remove directory: " << entryPath.string() << ", Error: " << ec.message() << std::endl;
                    return false;
                }
                // 如果没有错误码，说明目录已经不存在，继续处理
            }
        } else {
            if (!fs::remove(entryPath, ec)) {
                // 检查是否真的有错误
                if (ec) {
                    std::cerr << "clearDirectory: Failed to remove file: " << entryPath.string() << ", Error: " << ec.message() << std::endl;
                    return false;
                }
                // 如果没有错误码，说明文件已经不存在，继续处理
            }
        }
    }
    
    return !ec;
}

// 复制目录（递归）
bool FileSystem::copyDirectory(const std::string& sourceDir, const std::string& destDir) {
    std::error_code ec;
    
    // 检查源目录是否存在
    if (!fs::exists(sourceDir, ec)) {
        std::cerr << "copyDirectory: Source directory does not exist: " << sourceDir << ", Error: " << ec.message() << std::endl;
        return false;
    }
    if (!fs::is_directory(sourceDir, ec)) {
        std::cerr << "copyDirectory: Source path is not a directory: " << sourceDir << ", Error: " << ec.message() << std::endl;
        return false;
    }
    
    // 创建目标目录
    if (!fs::create_directories(destDir, ec)) {
        // 检查是否因为目录已存在而失败（这是正常情况，不是错误）
        if (ec) {
            std::cerr << "copyDirectory: Failed to create destination directory: " << destDir << ", Error: " << ec.message() << std::endl;
            return false;
        }
        // 目录已存在，这是正常情况，继续执行
        std::cout << "copyDirectory: Destination directory already exists: " << destDir << std::endl;
    }
    
    // 遍历源目录并复制所有内容
    for (const auto& entry : fs::directory_iterator(sourceDir, ec)) {
        if (ec) {
            std::cerr << "copyDirectory: Failed to iterate source directory: " << sourceDir << ", Error: " << ec.message() << std::endl;
            return false;
        }
        
        const auto& sourcePath = entry.path();
        const auto destPath = fs::path(destDir) / sourcePath.filename();
        
        if (entry.is_directory(ec)) {
            if (!copyDirectory(sourcePath.string(), destPath.string())) {
                std::cerr << "copyDirectory: Failed to copy subdirectory: " << sourcePath.string() << " to " << destPath.string() << std::endl;
                return false;
            }
        } else {
            if (!fs::copy_file(sourcePath, destPath, fs::copy_options::overwrite_existing, ec)) {
                std::cerr << "copyDirectory: Failed to copy file: " << sourcePath.string() << " to " << destPath.string() << ", Error: " << ec.message() << std::endl;
                return false;
            }
        }
    }
    
    return !ec;
}