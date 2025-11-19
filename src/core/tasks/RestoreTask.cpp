#include "RestoreTask.hpp"
#include "../../utils/FileSystem.hpp"
#include <filesystem>

RestoreTask::RestoreTask(const std::string& backup, const std::string& restore, ILogger* log)
    : backupPath(backup), restorePath(restore), logger(log), status(TaskStatus::PENDING) {}

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
    
    if (files.empty()) {
        logger->info("No files found to restore");
        status = TaskStatus::COMPLETED;
        return true;
    }
    
    int successCount = 0;
    int failCount = 0;
    
    for (const auto& backupFile : files) {
        std::string relativePath = FileSystem::getRelativePath(backupFile, backupPath);
        std::string restoreFile = (std::filesystem::path(restorePath) / relativePath).string();
        
        // Ensure target parent directory exists
        if (!FileSystem::createDirectories(
                std::filesystem::path(restoreFile).parent_path().string())) {
            logger->error("Failed to create restore directory: " + restoreFile);
            status = TaskStatus::FAILED;
            return false;
        }

        // Copy file: from backup location → restore location
        if (!FileSystem::copyFile(backupFile, restoreFile)) {
            logger->error("Restore failed: " + backupFile + " -> " + restoreFile);
            status = TaskStatus::FAILED;
            return false;
        }
    }
    
    logger->info("Restore completed!"); 
    status = TaskStatus::COMPLETED;
    return true;
}

TaskStatus RestoreTask::getStatus() const {
    return status;
}