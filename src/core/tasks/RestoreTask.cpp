#include "RestoreTask.hpp"
#include "../../utils/FileSystem.hpp"
#include "../../utils/FilePackager.hpp"
#include <filesystem>

RestoreTask::RestoreTask(const std::string& backup, const std::string& restore, ILogger* log, 
                       const std::vector<std::shared_ptr<Filter>>& filterList, bool compress, bool package, const std::string& pkgFileName) 
    : backupPath(backup), restorePath(restore), status(TaskStatus::PENDING), logger(log), filters(filterList), 
      compressEnabled(compress), packageEnabled(package), packageFileName(pkgFileName) {}

bool RestoreTask::execute() {
    logger->info("Starting restore: " + backupPath + " -> " + restorePath);
    status = TaskStatus::RUNNING;
    
    // 如果启用了文件拼接功能，先解包备份文件
    if (packageEnabled) {
        logger->info("Unpacking backup files...");
        
        std::string packagePath = (std::filesystem::path(backupPath) / packageFileName).string();
        
        // 创建临时目录用于存放解包后的文件
        std::string tempUnpackDir = backupPath + "/temp_unpack";
        if (!FileSystem::createDirectories(tempUnpackDir)) {
            logger->error("Failed to create temporary unpack directory");
            status = TaskStatus::FAILED;
            return false;
        }
        
        // 创建FilePackager实例并执行解包
        FilePackager packager;
        if (!packager.unpackFiles(packagePath, tempUnpackDir)) {
            logger->error("Failed to unpack backup files");
            status = TaskStatus::FAILED;
            // 清理临时目录
            std::filesystem::remove_all(tempUnpackDir);
            return false;
        }
        
        logger->info("Backup files unpacked successfully");
    }
    
    // 如果启用了拼接功能，从临时目录获取文件列表，否则从原始备份目录获取
    std::string actualBackupPath = packageEnabled ? (backupPath + "/temp_unpack") : backupPath;
    
    if (!FileSystem::exists(actualBackupPath)) {
        logger->error("Backup directory not found: " + actualBackupPath);
        status = TaskStatus::FAILED;
        // 如果使用了临时目录，清理它
        if (packageEnabled) {
            std::filesystem::remove_all(backupPath + "/temp_unpack");
        }
        return false;
    }
    
    auto files = FileSystem::getAllFiles(actualBackupPath);
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
        // 如果使用了临时目录，清理它
        if (packageEnabled) {
            std::filesystem::remove_all(backupPath + "/temp_unpack");
        }
        return true;
    }
    
    int successCount = 0;
    int failCount = 0;
    
    for (const auto& backupFile : files) {
        std::string relativePath = backupFile.getRelativePath(std::filesystem::path(actualBackupPath)).string();
        std::string restoreFile = (std::filesystem::path(restorePath) / relativePath).string();
        
        // Ensure target parent directory exists
        if (!FileSystem::createDirectories(
                std::filesystem::path(restoreFile).parent_path().string())) {
            logger->error("Failed to create restore directory: " + restoreFile);
            status = TaskStatus::FAILED;
            // 如果使用了临时目录，清理它
            if (packageEnabled) {
                std::filesystem::remove_all(backupPath + "/temp_unpack");
            }
            return false;
        }

        // 根据文件扩展名决定解压方式
        bool success;
        std::string backupFilePath = backupFile.getFilePath().string();
        // 检查文件扩展名是否为.huff，而不仅仅依赖compressEnabled标志
        if (backupFilePath.size() > 5 && backupFilePath.substr(backupFilePath.size() - 5) == ".huff") {
            // 解压并复制文件（去掉.huff扩展名）
            std::string destinationWithoutExt = restoreFile.substr(0, restoreFile.size() - 5);
            success = FileSystem::decompressAndCopyFile(backupFilePath, destinationWithoutExt);
        } else {
            // 普通复制文件
            success = FileSystem::copyFile(backupFilePath, restoreFile);
        }
        
        if (!success) {
            logger->error("Restore failed: " + backupFile.getFilePath().string() + " -> " + restoreFile);
            status = TaskStatus::FAILED;
            // 如果使用了临时目录，清理它
            if (packageEnabled) {
                std::filesystem::remove_all(backupPath + "/temp_unpack");
            }
            return false;
        }
        successCount++;
    }
    
    // 如果使用了临时目录，清理它
    if (packageEnabled) {
        std::filesystem::remove_all(backupPath + "/temp_unpack");
        logger->info("Temporary unpack directory cleaned up");
    }
    
    logger->info("Restore completed!"); 
    status = TaskStatus::COMPLETED;
    return true;
}

TaskStatus RestoreTask::getStatus() const {
    return status;
}