#include "BackUpTask.hpp"
#include "../utils/FileSystem.hpp"
#include <filesystem>

BackupTask::BackupTask(const std::string& source, const std::string& backup, ILogger* log)
    : sourcePath(source), backupPath(backup), logger(log), status(TaskStatus::PENDING) {}

bool BackupTask::execute() {
    logger->info("开始备份: " + sourcePath + " -> " + backupPath);
    status = TaskStatus::RUNNING;
    
    if (!FileSystem::exists(sourcePath)) {
        logger->error("源目录不存在: " + sourcePath);
        status = TaskStatus::FAILED;
        return false;
    }
    
    auto files = FileSystem::getAllFiles(sourcePath);
    logger->info("找到 " + std::to_string(files.size()) + " 个文件需要备份");
    
    if (files.empty()) {
        logger->warn("没有找到需要备份的文件");
        status = TaskStatus::COMPLETED;
        return true;
    }
    
    int successCount = 0;
    int failCount = 0;
    uint64_t totalSize = 0;
    
    for (const auto& file : files) {
        std::string relativePath = FileSystem::getRelativePath(file, sourcePath);
        std::string backupFile = (fs::path(backupPath) / relativePath).string();
        
        if (!FileSystem::createDirectories(
                std::filesystem::path(backupFile).parent_path().string())) {
            logger->error("无法创建目标目录: " + backupFile);
            status = TaskStatus::FAILED;
            return false;
        }

        if (!FileSystem::copyFile(file, backupFile)) {
            logger->error("复制失败: " + file + " -> " + backupFile);
            status = TaskStatus::FAILED;
            return false;
        }
    }
    
    logger->info("备份完成!");
    status = TaskStatus::COMPLETED;
    return true;
}

TaskStatus BackupTask::getStatus() const {
    return status;
}