#include "RestoreTask.hpp"
#include "../utils/FileSystem.hpp"
#include <filesystem>

RestoreTask::RestoreTask(const std::string& backup, const std::string& restore, ILogger* log)
    : backupPath(backup), restorePath(restore), logger(log), status(TaskStatus::PENDING) {}

bool RestoreTask::execute() {
    logger->info("开始还原: " + backupPath + " -> " + restorePath);
    
    if (!FileSystem::exists(backupPath)) {
        logger->error("备份目录不存在: " + backupPath);
        return false;
    }
    
    auto files = FileSystem::getAllFiles(backupPath);
    logger->info("找到 " + std::to_string(files.size()) + " 个文件需要还原");
    
    if (files.empty()) {
        logger->info("没有找到需要还原的文件");
        return true;
    }
    
    int successCount = 0;
    int failCount = 0;
    
    for (const auto& backupFile : files) {
        std::string relativePath = FileSystem::getRelativePath(backupFile, backupPath);
        std::string restoreFile = (std::filesystem::path(restorePath) / relativePath).string();
        
        // 确保目标父目录存在
        if (!FileSystem::createDirectories(
                std::filesystem::path(restorePath).parent_path().string())) {
            logger->error("无法创建恢复目录: " + restorePath);
            status = TaskStatus::FAILED;
            return false;
        }

        // 复制文件：从备份位置 → 恢复位置
        if (!FileSystem::copyFile(backupFile, restoreFile)) {
            logger->error("恢复失败: " + backupFile + " -> " + restoreFile);
            status = TaskStatus::FAILED;
            return false;
        }
    }
    
    logger->info("还原完成！"); 
    status = TaskStatus::COMPLETED;
    return true;
}

TaskStatus RestoreTask::getStatus() const {
    return status;
}