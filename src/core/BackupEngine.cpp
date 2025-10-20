// src/core/BackupEngine.cpp
#include "BackupEngine.hpp"
#include "../utils/FileSystem.hpp"
#include "../utils/Logger.hpp"
#include <filesystem>

namespace fs = std::filesystem;

BackupEngine::BackupEngine(const std::string& source, const std::string& backup) 
    : sourcePath(source), backupPath(backup) {}

std::string BackupEngine::getBackupFilePath(const std::string& sourceFile) {
    return FileSystem::getRelativePath(sourceFile, sourcePath);
}

bool BackupEngine::performBackup() {
    Logger::info("开始备份: " + sourcePath + " -> " + backupPath);
    
    if (!FileSystem::exists(sourcePath)) {
        Logger::error("源目录不存在: " + sourcePath);
        return false;
    }
    
    auto files = FileSystem::getAllFiles(sourcePath);
    Logger::info("找到 " + std::to_string(files.size()) + " 个文件需要备份");
    
    if (files.empty()) {
        Logger::info("没有找到需要备份的文件");
        return true;
    }
    
    int successCount = 0;
    int failCount = 0;
    uint64_t totalSize = 0;
    
    for (const auto& file : files) {
        std::string relativePath = getBackupFilePath(file);
        std::string backupFile = (fs::path(backupPath) / relativePath).string();
        
        if (FileSystem::copyFile(file, backupFile)) {
            successCount++;
            totalSize += FileSystem::getFileSize(file);
        } else {
            failCount++;
        }
    }
    
    Logger::info("备份完成: " + std::to_string(successCount) + " 成功, " + 
                std::to_string(failCount) + " 失败, 总大小: " + 
                std::to_string(totalSize) + " 字节");
    
    return failCount == 0;
}

bool BackupEngine::performRestore(const std::string& restorePath) {
    std::string actualRestorePath = restorePath.empty() ? sourcePath + "_restored" : restorePath;
    Logger::info("开始还原: " + backupPath + " -> " + actualRestorePath);
    
    if (!FileSystem::exists(backupPath)) {
        Logger::error("备份目录不存在: " + backupPath);
        return false;
    }
    
    auto files = FileSystem::getAllFiles(backupPath);
    Logger::info("找到 " + std::to_string(files.size()) + " 个文件需要还原");
    
    if (files.empty()) {
        Logger::info("没有找到需要还原的文件");
        return true;
    }
    
    int successCount = 0;
    int failCount = 0;
    
    for (const auto& backupFile : files) {
        std::string relativePath = FileSystem::getRelativePath(backupFile, backupPath);
        std::string restoreFile = (fs::path(actualRestorePath) / relativePath).string();
        
        if (FileSystem::copyFile(backupFile, restoreFile)) {
            successCount++;
        } else {
            failCount++;
        }
    }
    
    Logger::info("还原完成: " + std::to_string(successCount) + " 成功, " + 
                std::to_string(failCount) + " 失败");
    
    return failCount == 0;
}