// src/utils/FileSystem.cpp
#include "FileSystem.hpp"
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
    std::error_code ec;
    bool result = fs::create_directories(path, ec);
    return result && !ec;
}

bool FileSystem::copyFile(const std::string& source, const std::string& destination) {
    // 确保目标目录存在
    fs::path destPath(destination);
    if (!destPath.parent_path().empty()) {
        if (!createDirectories(destPath.parent_path().string())) {
            return false;
        }
    }

    std::error_code ec;
    fs::copy_file(source, destination, fs::copy_options::overwrite_existing, ec);
    return !ec;
}

std::vector<std::string> FileSystem::getAllFiles(const std::string& directory) {
    std::vector<std::string> files;
    std::error_code ec;

    if (!fs::exists(directory, ec) || !fs::is_directory(directory, ec)) {
        return files; // 返回空列表
    }

    try {
        for (const auto& entry : fs::recursive_directory_iterator(directory, ec)) {
            if (ec) break;
            if (entry.is_regular_file()) {
                files.push_back(entry.path().string());
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