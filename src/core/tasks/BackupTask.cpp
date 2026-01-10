#include "BackupTask.hpp"
#include "../../utils/FileSystem.hpp"
#include "../../utils/FilePackager.hpp"
#include "../../utils/Encryption.hpp"
#include <filesystem>
#include <atomic>

BackupTask::BackupTask(const std::string& source, const std::string& backup, ILogger* log, 
                      const std::vector<std::shared_ptr<Filter>>& filterList, bool compress, bool package, const std::string& pkgFileName, const std::string& pass, 
                      std::atomic<bool>* interruptFlag) 
  : sourcePath(source), backupPath(backup), status(TaskStatus::PENDING), logger(log), filters(filterList), 
    compressEnabled(compress), packageEnabled(package), packageFileName(pkgFileName), password(pass), 
    interrupted(interruptFlag) {}

bool BackupTask::execute() {
    logger->info("Starting backup: " + sourcePath + " -> " + backupPath);
    status = TaskStatus::RUNNING;
    
    // 检查是否被中断
    if (isInterrupted()) {
        logger->info("Backup interrupted.");
        status = TaskStatus::CANCELLED;
        return false;
    }
    
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
    
    files = filteredFiles;
    
    if (files.empty()) {
        logger->warn("No files found to backup");
        status = TaskStatus::COMPLETED;
        return true;
    }
    int successCount = 0;
    uint64_t totalSize = 0;
    
    // 用于存储所有备份文件路径，以便后续拼接
    std::vector<std::string> backedUpFiles;
    
    for (const auto& file : files) {
        // 检查是否被中断
        if (isInterrupted()) {
            logger->info("Backup interrupted.");
            status = TaskStatus::CANCELLED;
            return false;
        }
        
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
            std::string compressedBackupFile = backupFile + ".huff";
            success = FileSystem::copyAndCompressFile(file.getFilePath().string(), compressedBackupFile);
            
            // 检查压缩是否真正创建了.huff文件
            if (success) {
                // 检查文件是否实际存在
                if (std::filesystem::exists(compressedBackupFile)) {
                    finalBackupFile = compressedBackupFile;
                } else {
                    // 压缩失败，文件已被替换为非压缩版本
                    finalBackupFile = backupFile;
                }
            } else {
                // 压缩失败，直接复制
                finalBackupFile = backupFile;
                success = FileSystem::copyFile(file.getFilePath().string(), backupFile);
            }
        } else if (file.isSymbolicLink()) {
            // 处理符号链接文件
            // 符号链接需要被打包，所以直接复制到备份目录
            // 但不会添加到打包文件列表中，因为符号链接的处理逻辑在FilePackager中
            finalBackupFile = backupFile;
            success = FileSystem::copyFile(file.getFilePath().string(), backupFile);
        } else {
            // 对非普通文件和非符号链接文件（目录、设备文件等）直接复制，不压缩
            finalBackupFile = backupFile;
            success = FileSystem::copyFile(file.getFilePath().string(), backupFile);
        }
        
        if (!success) {
            logger->error("Copy failed: " + file.getFilePath().string() + " -> " + finalBackupFile);
            status = TaskStatus::FAILED;
            return false;
        }
        
        // 将备份后的实际文件路径添加到列表中，包括符号链接
        // 符号链接需要被打包，FilePackager会处理符号链接的特殊逻辑
        backedUpFiles.push_back(finalBackupFile);
        
        successCount++;
        totalSize += file.getFileSize();
    }
    
    // 如果启用了文件拼接功能，将所有备份文件拼接成一个包文件
    std::string finalPackagePath;
    if (packageEnabled) {
        // 检查是否被中断
        if (isInterrupted()) {
            logger->info("Backup interrupted.");
            status = TaskStatus::CANCELLED;
            return false;
        }
        
        logger->info("Packaging backup files into a single file...");
        
        finalPackagePath = (std::filesystem::path(backupPath) / packageFileName).string();
        
        // 在删除原始文件之前，将它们转换为File对象，以便正确读取元数据
        std::vector<File> backupFileObjects;
        for (const auto& filePath : backedUpFiles) {
            backupFileObjects.emplace_back(filePath);
        }
        
        // 创建FilePackager实例并执行拼接
        FilePackager packager;
        if (!packager.packageFiles(backupFileObjects, finalPackagePath)) {
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
            // 检查是否被中断
            if (isInterrupted()) {
                logger->info("Backup interrupted.");
                status = TaskStatus::CANCELLED;
                return false;
            }
            
            std::string encryptedFile = finalPackagePath + ".enc";
            if (!Encryption::encryptFile(finalPackagePath, encryptedFile, password)) {
                logger->error("Encryption failed: " + finalPackagePath);
                status = TaskStatus::FAILED;
                return false;
            }
            
            // 复制打包文件的元数据到加密后的文件
            std::error_code ec;
            auto fileTime = std::filesystem::last_write_time(finalPackagePath, ec);
            if (!ec) {
                std::filesystem::last_write_time(encryptedFile, fileTime, ec);
                if (ec) {
                    logger->warn("Failed to copy file time to encrypted package: " + encryptedFile);
                }
            }
            
            // 复制权限
            auto permissions = std::filesystem::status(finalPackagePath, ec).permissions();
            if (!ec) {
                std::filesystem::permissions(encryptedFile, permissions, ec);
                if (ec) {
                    logger->warn("Failed to copy permissions to encrypted package: " + encryptedFile);
                }
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
                // 检查是否被中断
                if (isInterrupted()) {
                    logger->info("Backup interrupted.");
                    status = TaskStatus::CANCELLED;
                    return false;
                }
                
                std::string encryptedFile = backupFile + ".enc";
                if (!Encryption::encryptFile(backupFile, encryptedFile, password)) {
                    logger->error("Encryption failed: " + backupFile);
                    status = TaskStatus::FAILED;
                    return false;
                }
                
                // 复制加密前文件的元数据到加密后的文件
                std::error_code ec;
                
                // 获取加密前文件的元数据
                auto fileStatus = std::filesystem::status(backupFile, ec);
                if (!ec) {
                    // 复制修改时间
                    auto fileTime = std::filesystem::last_write_time(backupFile, ec);
                    if (!ec) {
                        std::filesystem::last_write_time(encryptedFile, fileTime, ec);
                        if (ec) {
                            logger->warn("Failed to copy file time to encrypted file: " + encryptedFile);
                        }
                    }
                    
                    // 复制权限
                    std::filesystem::permissions(encryptedFile, fileStatus.permissions(), ec);
                    if (ec) {
                        logger->warn("Failed to copy permissions to encrypted file: " + encryptedFile);
                    }
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

bool BackupTask::isInterrupted() const {
    if (interrupted != nullptr) {
        return interrupted->load();
    }
    return false;
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