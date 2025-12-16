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