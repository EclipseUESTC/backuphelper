#include "BackupTask.hpp"
#include "../../utils/FileSystem.hpp"
#include "../../utils/FilePackager.hpp"
#include "../../utils/Encryption.hpp"
#include <filesystem>

BackupTask::BackupTask(const std::string& source, const std::string& backup, ILogger* log, 
                      const std::vector<std::shared_ptr<Filter>>& filterList, bool compress, bool package, const std::string& pkgFileName, const std::string& pass) 
    : sourcePath(source), backupPath(backup), status(TaskStatus::PENDING), logger(log), filters(filterList), 
      compressEnabled(compress), packageEnabled(package), packageFileName(pkgFileName), password(pass) {}

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
    int regularCount = 0, symlinkCount = 0, dirCount = 0, otherCount = 0;
    namespace fs = std::filesystem;
    for (const auto& file : files) {
        fs::path p = file.getFilePath();
        std::error_code ec;
        auto status = fs::status(p, ec);
        if (fs::is_regular_file(status)) regularCount++;
        else if (fs::is_symlink(status)) symlinkCount++;
        else if (fs::is_directory(status)) dirCount++;
        else otherCount++;
    }
    
    logger->info("file types stats:");
    logger->info("  Normal files: " + std::to_string(regularCount));
    logger->info("  Symbolic link: " + std::to_string(symlinkCount));
    logger->info("  Directories: " + std::to_string(dirCount));
    logger->info("  Others: " + std::to_string(otherCount));
    int successCount = 0;
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

        // 根据压缩开关和文件类型选择复制方式
        bool success;
        std::string finalBackupFile;
        if (compressEnabled && file.isRegularFile()) {
            // 仅对普通文件进行压缩，添加.huff扩展名
            finalBackupFile = backupFile + ".huff";
            success = FileSystem::copyAndCompressFile(file.getFilePath().string(), finalBackupFile);
        } else {
            // 对非普通文件（目录、符号链接、设备文件等）直接复制，不压缩
            finalBackupFile = backupFile;
            success = FileSystem::copyFile(file.getFilePath().string(), backupFile);
        }
        
        if (!success) {
            logger->error("Copy failed: " + file.getFilePath().string() + " -> " + finalBackupFile);
            status = TaskStatus::FAILED;
            return false;
        }
        
        // 将备份后的文件路径添加到列表中（暂不加密，等打包后一起加密）
        backedUpFiles.push_back(finalBackupFile);
        
        successCount++;
        totalSize += file.getFileSize();
    }
    
    // 如果启用了文件拼接功能，将所有备份文件拼接成一个包文件
    std::string finalPackagePath;
    if (packageEnabled) {
        logger->info("Packaging backup files into a single file...");
        
        finalPackagePath = (std::filesystem::path(backupPath) / packageFileName).string();
        
        // 创建FilePackager实例并执行拼接
        FilePackager packager;
        if (!packager.packageFiles(backedUpFiles, finalPackagePath)) {
            logger->error("Failed to package backup files");
            status = TaskStatus::FAILED;
            return false;
        }
        
        logger->info("Backup files packaged successfully: " + finalPackagePath);
        
        // 删除原始备份文件，只保留打包文件
        for (const auto& file : backedUpFiles) {
            if (!FileSystem::removeFile(file)) {
                logger->warn("Failed to remove temporary backup file: " + file);
            }
        }
        
        // 删除空目录
        removeEmptyDirectories(backupPath);
        
        logger->info("Removed temporary backup files, only packaged file remains");
        
        // 如果提供了密码，则加密打包文件
        if (!password.empty()) {
            std::string encryptedFile = finalPackagePath + ".enc";
            if (!Encryption::encryptFile(finalPackagePath, encryptedFile, password)) {
                logger->error("Encryption failed: " + finalPackagePath);
                status = TaskStatus::FAILED;
                return false;
            }
            
            // 删除未加密的打包文件
            if (!FileSystem::removeFile(finalPackagePath)) {
                logger->warn("Failed to remove unencrypted package file: " + finalPackagePath);
            }
            
            finalPackagePath = encryptedFile;
        }
    } else {
        // 如果不打包，则对每个文件进行加密
        if (!password.empty()) {
            for (auto& backupFile : backedUpFiles) {
                std::string encryptedFile = backupFile + ".enc";
                if (!Encryption::encryptFile(backupFile, encryptedFile, password)) {
                    logger->error("Encryption failed: " + backupFile);
                    status = TaskStatus::FAILED;
                    return false;
                }
                
                // 删除未加密的备份文件
                if (!FileSystem::removeFile(backupFile)) {
                    logger->warn("Failed to remove unencrypted backup file: " + backupFile);
                }
                
                backupFile = encryptedFile;
            }
        }
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