#include "BackupTask.hpp"
#include "../../utils/FileSystem.hpp"
#include "../../utils/FilePackager.hpp"
#include <filesystem>

BackupTask::BackupTask(const std::string& source, const std::string& backup, ILogger* log, 
                      const std::vector<std::shared_ptr<Filter>>& filterList, bool compress, bool package, const std::string& pkgFileName) 
    : sourcePath(source), backupPath(backup), status(TaskStatus::PENDING), logger(log), filters(filterList), 
      compressEnabled(compress), packageEnabled(package), packageFileName(pkgFileName) {}

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
    
    // 用于存储所有备份文件路径，以便后续拼接
    std::vector<std::string> backedUpFiles;
    
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
        std::string finalBackupFile;
        if (compressEnabled) {
            // 压缩并复制文件，添加.huff扩展名
            finalBackupFile = backupFile + ".huff";
            success = FileSystem::copyAndCompressFile(file.getFilePath().string(), finalBackupFile);
        } else {
            // 普通复制文件
            finalBackupFile = backupFile;
            success = FileSystem::copyFile(file.getFilePath().string(), backupFile);
        }
        
        if (!success) {
            logger->error("Copy failed: " + file.getFilePath().string() + " -> " + finalBackupFile);
            status = TaskStatus::FAILED;
            return false;
        }
        
        // 将备份后的文件路径添加到列表中
        backedUpFiles.push_back(finalBackupFile);
        
        successCount++;
        totalSize += file.getFileSize();
    }
    
    // 如果启用了文件拼接功能，将所有备份文件拼接成一个包文件
    if (packageEnabled) {
        logger->info("Packaging backup files into a single file...");
        
        std::string packagePath = (std::filesystem::path(backupPath) / packageFileName).string();
        
        // 创建FilePackager实例并执行拼接
        FilePackager packager;
        if (!packager.packageFiles(backedUpFiles, packagePath)) {
            logger->error("Failed to package backup files");
            status = TaskStatus::FAILED;
            return false;
        }
        
        logger->info("Backup files packaged successfully: " + packagePath);
        
        // 删除原始备份文件，只保留打包文件
        for (const auto& file : backedUpFiles) {
            if (!FileSystem::removeFile(file)) {
                logger->warn("Failed to remove temporary backup file: " + file);
            }
        }
        
        // 删除空目录
        removeEmptyDirectories(backupPath);
        
        logger->info("Removed temporary backup files, only packaged file remains");
    }
    
    logger->info("Backup completed!");
    status = TaskStatus::COMPLETED;
    return true;
}

TaskStatus BackupTask::getStatus() const {
    return status;
}

    // 删除空目录的辅助方法
    void BackupTask::removeEmptyDirectories(const std::string& path) {
        for (auto it = std::filesystem::directory_iterator(path); it != std::filesystem::directory_iterator();) {
            if (std::filesystem::is_directory(*it) && !std::filesystem::is_symlink(*it)) {
                std::string subdirPath = it->path().string();
                ++it;
                removeEmptyDirectories(subdirPath);
                if (std::filesystem::is_empty(subdirPath)) {
                    std::filesystem::remove(subdirPath);
                }
            } else {
                ++it;
            }
        }
    }