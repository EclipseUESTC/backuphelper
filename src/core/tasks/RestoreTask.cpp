#include "RestoreTask.hpp"
#include "../../utils/FileSystem.hpp"
#include <filesystem>

RestoreTask::RestoreTask(const std::string& backup, const std::string& restore, ILogger* log, 
                       const std::vector<std::shared_ptr<Filter>>& filterList, bool compress) 
    : backupPath(backup), restorePath(restore), status(TaskStatus::PENDING), logger(log), filters(filterList), compressEnabled(compress) {}

bool RestoreTask::execute() {
    logger->info("Starting restore: " + backupPath + " -> " + restorePath);
    status = TaskStatus::RUNNING;
    
    if (!FileSystem::exists(backupPath)) {
        logger->error("Backup directory not found: " + backupPath);
        status = TaskStatus::FAILED;
        return false;
    }
    
    auto files = FileSystem::getAllFiles(backupPath);
    logger->info("Found " + std::to_string(files.size()) + " files to restore");
    
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
    
    logger->info("After filtering, " + std::to_string(filteredFiles.size()) + " files will be restored");
    files = filteredFiles;
    
    if (files.empty()) {
        logger->info("No files found to restore");
        status = TaskStatus::COMPLETED;
        return true;
    }
    
    int successCount = 0;
    int failCount = 0;
    
    for (const auto& backupFile : files) {
        std::string relativePath = backupFile.getRelativePath(std::filesystem::path(backupPath)).string();
        std::string restoreFile = (std::filesystem::path(restorePath) / relativePath).string();
        
        // Ensure target parent directory exists
        if (!FileSystem::createDirectories(
                std::filesystem::path(restoreFile).parent_path().string())) {
            logger->error("Failed to create restore directory: " + restoreFile);
            status = TaskStatus::FAILED;
            return false;
        }

        // 根据文件扩展名决定解压方式
        bool success;
        std::string backupFilePath = backupFile.getFilePath().string();
        if (compressEnabled && backupFilePath.size() > 5 && backupFilePath.substr(backupFilePath.size() - 5) == ".huff") {
            // 解压并复制文件（去掉.huff扩展名）
            success = FileSystem::decompressAndCopyFile(backupFilePath, restoreFile);
        } else {
            // 普通复制文件
            success = FileSystem::copyFile(backupFilePath, restoreFile);
        }
        
        if (!success) {
            logger->error("Restore failed: " + backupFile.getFilePath().string() + " -> " + restoreFile);
            status = TaskStatus::FAILED;
            return false;
        }
        successCount++;
    }
    
    logger->info("Restore completed!"); 
    status = TaskStatus::COMPLETED;
    return true;
}

TaskStatus RestoreTask::getStatus() const {
    return status;
}