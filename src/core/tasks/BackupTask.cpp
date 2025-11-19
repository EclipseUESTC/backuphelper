#include "BackUpTask.hpp"
#include "../../utils/FileSystem.hpp"
#include <filesystem>

BackupTask::BackupTask(const std::string& source, const std::string& backup, ILogger* log)
    : sourcePath(source), backupPath(backup), logger(log), status(TaskStatus::PENDING) {}

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
    
    if (files.empty()) {
        logger->warn("No files found to backup");
        status = TaskStatus::COMPLETED;
        return true;
    }
    
    int successCount = 0;
    int failCount = 0;
    uint64_t totalSize = 0;
    
    for (const auto& file : files) {
        std::string relativePath = FileSystem::getRelativePath(file, sourcePath);
        std::string backupFile = (std::filesystem::path(backupPath) / relativePath).string();
        
        // 获取父目录路径
        std::string parentDir = std::filesystem::path(backupFile).parent_path().string();
        
        if (!parentDir.empty() && !FileSystem::createDirectories(parentDir)) {
            logger->error("Failed to create target directory: " + parentDir);
            status = TaskStatus::FAILED;
            return false;
        }

        if (!FileSystem::copyFile(file, backupFile)) {
            logger->error("Copy failed: " + file + " -> " + backupFile);
            status = TaskStatus::FAILED;
            return false;
        }
    }
    
    logger->info("Backup completed!");
    status = TaskStatus::COMPLETED;
    return true;
}

TaskStatus BackupTask::getStatus() const {
    return status;
}