// src/utils/FileSystem.cpp
#include "FileSystem.hpp"
#include "Logger.hpp"
#include <filesystem>
#include <fstream>
#include <system_error>

namespace fs = std::filesystem;

bool FileSystem::exists(const std::string& path) {
    std::error_code ec;
    return fs::exists(path, ec);
}

bool FileSystem::createDirectories(const std::string& path)  {
        std::error_code ec;
        bool result = fs::create_directories(path, ec);
        if (ec) {
            Logger::error("创建目录失败: " + path + " - " + ec.message());
        }
        return result;
}

bool FileSystem::copyFile(const std::string& source, const std::string& destination) {
    try {
        fs::path destPath(destination);
        createDirectories(destPath.parent_path().string());
        
        fs::copy_file(source, destination, fs::copy_options::overwrite_existing);
        Logger::debug("复制文件: " + source + " -> " + destination);
        return true;
    } catch (const fs::filesystem_error& e) {
        Logger::error("复制文件失败: " + std::string(e.what()));
        return false;
    }
}

std::vector<std::string> FileSystem::getAllFiles(const std::string& directory) {
    std::vector<std::string> files;
    
    try {
        for (const auto& entry : fs::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().string());
            }
        }
    } catch (const fs::filesystem_error& e) {
        Logger::error("遍历目录失败: " + std::string(e.what()));
    }
    
    return files;
}

uint64_t FileSystem::getFileSize(const std::string& filePath) {
    try {
        return fs::file_size(filePath);
    } catch (const fs::filesystem_error& e) {
        Logger::error("获取文件大小失败: " + filePath);
        return 0;
    }
}

std::string FileSystem::getRelativePath(const std::string& path, const std::string& base) {
    try {
        fs::path relative = fs::relative(path, base);
        return relative.string();
    } catch (const fs::filesystem_error& e) {
        Logger::error("计算相对路径失败: " + path + " -> " + base);
        return path;
    }
}