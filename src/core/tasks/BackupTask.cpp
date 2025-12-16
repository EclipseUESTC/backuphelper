#include "BackupTask.hpp"
#include "../../utils/FileSystem.hpp"
#include <filesystem>

BackupTask::BackupTask(const std::string& source, const std::string& backup, ILogger* log, 
                      const std::vector<std::shared_ptr<Filter>>& filterList, bool compress) 
    : sourcePath(source), backupPath(backup), status(TaskStatus::PENDING), logger(log), filters(filterList), compressEnabled(compress) {}

bool BackupTask::execute() {
    logger->info("Starting backup: " + sourcePath + " -> " + backupPath);
    status = TaskStatus::RUNNING;
    
    if (!FileSystem::exists(sourcePath)) {
        logger->error("Source directory not found: " + sourcePath);
        status = TaskStatus::FAILED;
        return false;
    }
    
    // 先确保顶级备份目录存在
    if (!FileSystem::createDirectories(backupPath)) {
        logger->error("Failed to create base backup directory: " + backupPath);
        status = TaskStatus::FAILED;
        return false;
    }
    
    auto files = FileSystem::getAllFiles(sourcePath);
    logger->info("Found " + std::to_string(files.size()) + " files to backup");
    
    // 应用过滤器
    std::vector<File> filteredFiles;
    for (const auto& file : files) {
        bool passAllFilters = true;
        for (const auto& filter : filters) {
            if (!filter->match(file)) {
                passAllFilters = false;
                break;
            }
        }
        if (passAllFilters) {
            filteredFiles.push_back(file);
        }
    }
    
    logger->info("After filtering, " + std::to_string(filteredFiles.size()) + " files will be backed up");
    files = filteredFiles;
    
    if (files.empty()) {
        logger->warn("No files found to backup");
        status = TaskStatus::COMPLETED;
        return true;
    }
    
    int successCount = 0;
    int failCount = 0;
    uint64_t totalSize = 0;
    
    for (const auto& file : files) {
        std::string relativePath = file.getRelativePath(std::filesystem::path(sourcePath)).string();
        std::string backupFile = (std::filesystem::path(backupPath) / relativePath).string();
        
        // 获取父目录路径
        std::string parentDir = std::filesystem::path(backupFile).parent_path().string();
        
        if (!parentDir.empty() && !FileSystem::createDirectories(parentDir)) {
            logger->error("Failed to create target directory: " + parentDir);
            status = TaskStatus::FAILED;
            return false;
        }

        // 根据压缩开关选择复制方式
        bool success;
        if (compressEnabled) {
            // 压缩并复制文件，添加.huff扩展名
            success = FileSystem::copyAndCompressFile(file.getFilePath().string(), backupFile + ".huff");
        } else {
            // 普通复制文件
            success = FileSystem::copyFile(file.getFilePath().string(), backupFile);
        }
        
        if (!success) {
            logger->error("Copy failed: " + file.getFilePath().string() + " -> " + backupFile);
            status = TaskStatus::FAILED;
            return false;
        }
        successCount++;
        totalSize += file.getFileSize();
    }
    
    logger->info("Backup completed!");
    status = TaskStatus::COMPLETED;
    return true;
}

TaskStatus BackupTask::getStatus() const {
    return status;
}