#include "RestoreTask.hpp"
#include "../../utils/FileSystem.hpp"
#include "../../utils/FilePackager.hpp"
#include "../../utils/Encryption.hpp"
#include <filesystem>

RestoreTask::RestoreTask(const std::string& backup, const std::string& restore, ILogger* log, 
                       const std::vector<std::shared_ptr<Filter>>& filterList, bool compress, bool package, const std::string& pkgFileName, const std::string& pass) 
    : backupPath(backup), restorePath(restore), status(TaskStatus::PENDING), logger(log), filters(filterList), 
      compressEnabled(compress), packageEnabled(package), packageFileName(pkgFileName), password(pass) {}

bool RestoreTask::execute() {
    logger->info("Starting restore: " + backupPath + " -> " + restorePath);
    status = TaskStatus::RUNNING;
    
    // 检查还原目录是否存在，如果不存在则尝试创建
    if (!FileSystem::exists(restorePath)) {
        logger->info("Restore directory doesn't exist, trying to create it: " + restorePath);
        if (!FileSystem::createDirectories(restorePath)) {
            logger->error("Failed to create restore directory: " + restorePath);
            logger->error("Please check if you have permission to create directories at this location.");
            status = TaskStatus::FAILED;
            return false;
        }
    }
    
    // 获取备份文件列表
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
            // 过滤掉非当前备份生成的.enc文件
            std::string filePath = file.getFilePath().string();
            std::filesystem::path backupFile(filePath);
            std::string fileName = backupFile.filename().string();
            
            // 如果启用了打包功能，只保留打包文件或打包文件的加密版本
            if (packageEnabled) {
                if (fileName == packageFileName || fileName == (packageFileName + ".enc")) {
                    filteredFiles.push_back(file);
                }
            } else {
                // 如果未启用打包功能，保留所有文件
                filteredFiles.push_back(file);
            }
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
    
    for (const auto& backupFile : files) {
        std::string backupFilePath = backupFile.getFilePath().string();
        std::string relativePath = backupFile.getRelativePath(std::filesystem::path(backupPath)).string();
        std::string restoreFile = (std::filesystem::path(restorePath) / relativePath).string();
        
        // 确保目标目录存在
        if (!FileSystem::createDirectories(
                std::filesystem::path(restoreFile).parent_path().string())) {
            logger->error("Failed to create restore directory: " + restoreFile);
            status = TaskStatus::FAILED;
            return false;
        }
        
        // 1. 检查是否需要解密
        std::string currentSource = backupFilePath;
        std::string currentDest = restoreFile;
        bool needCleanup = false;
        std::string tempFile;
        bool isEncrypted = false;
        bool isCompressed = false;
        bool isPackaged = false;
        
        // 先检查文件是否加密
        if (backupFilePath.size() > 4 && backupFilePath.substr(backupFilePath.size() - 4) == ".enc") {
            isEncrypted = true;
            
            // 检查是否是打包文件的加密版本
            std::filesystem::path backupFile(backupFilePath);
            std::string fileName = backupFile.filename().string();
            
            // 如果是打包文件的加密版本，或者是解包后的加密文件，才需要密码
            bool isPackagedEncrypted = (packageEnabled && (fileName == (packageFileName + ".enc")));
            bool isRegularEncrypted = (!packageEnabled && backupFilePath.size() > 4 && backupFilePath.substr(backupFilePath.size() - 4) == ".enc");
            
            if (isPackagedEncrypted || isRegularEncrypted) {
                // 如果是加密文件但没有提供密码，返回错误
                if (password.empty()) {
                    logger->error("File is encrypted but no password provided: " + backupFilePath);
                    status = TaskStatus::FAILED;
                    return false;
                }
                
                // 尝试解密
                logger->info("Decrypting file: " + backupFilePath);
                
                // 保存原始加密文件的元数据
                std::error_code ec;
                auto originalFileTime = std::filesystem::last_write_time(backupFilePath, ec);
                auto originalPermissions = std::filesystem::status(backupFilePath, ec).permissions();
                
                // 创建临时解密文件
                tempFile = backupFilePath + ".tmp";
                needCleanup = true;
                
                if (!Encryption::decryptFile(backupFilePath, tempFile, password)) {
                    logger->error("Decryption failed: " + backupFilePath + " (wrong password?)");
                    status = TaskStatus::FAILED;
                    // 清理临时文件
                    if (needCleanup) {
                        std::filesystem::remove(tempFile);
                    }
                    return false;
                }
                
                // 将原始加密文件的元数据复制到临时解密文件
                if (!ec) {
                    std::filesystem::last_write_time(tempFile, originalFileTime, ec);
                    if (ec) {
                        logger->warn("Failed to copy file time to decrypted temp file: " + tempFile);
                    }
                    
                    std::filesystem::permissions(tempFile, originalPermissions, ec);
                    if (ec) {
                        logger->warn("Failed to copy permissions to decrypted temp file: " + tempFile);
                    }
                }
                
                currentSource = tempFile;
                // 去掉目标文件的.enc扩展名
                if (currentDest.size() > 4) {
                    currentDest = currentDest.substr(0, currentDest.size() - 4);
                }
            } else {
                // 不是加密的打包文件，不需要解密
                isEncrypted = false;
            }
        }
        
        // 2. 检查是否需要解包
        std::string tempUnpackDir;
        bool unpacked = false;
        
        if (packageEnabled) {
            // 检查当前源文件是否是打包文件
            // 处理以下情况：
            // 1. 原始打包文件（未加密）：package.pkg
            // 2. 加密打包文件：package.pkg.enc
            // 3. 解密后的临时打包文件：package.pkg.enc.tmp
            std::filesystem::path sourcePath(currentSource);
            std::string fileName = sourcePath.filename().string();
            
            if (fileName == packageFileName || 
                fileName == (packageFileName + ".enc") ||
                (isEncrypted && (fileName == (packageFileName + ".enc.tmp")))) {
                logger->info("Unpacking file: " + currentSource);
                
                // 创建临时解包目录
                tempUnpackDir = backupPath + "/temp_unpack";
                
                // 创建FilePackager实例并执行解包
                FilePackager packager;
                if (!packager.unpackFiles(currentSource, tempUnpackDir)) {
                    logger->error("Failed to unpack backup files");
                    status = TaskStatus::FAILED;
                    // 清理临时文件
                    if (needCleanup) {
                        std::filesystem::remove(tempFile);
                    }
                    return false;
                }
                
                unpacked = true;
                
                // 获取解包后的所有文件并逐个处理
                auto unpackedFiles = FileSystem::getAllFiles(tempUnpackDir);
                
                for (const auto& unpackedFile : unpackedFiles) {
                    std::string unpackedFilePath = unpackedFile.getFilePath().string();
                    std::string unpackedRelativePath = unpackedFile.getRelativePath(std::filesystem::path(tempUnpackDir)).string();
                    std::string unpackedRestoreFile = (std::filesystem::path(restorePath) / unpackedRelativePath).string();
                    
                    // 确保目标目录存在
                    if (!FileSystem::createDirectories(
                            std::filesystem::path(unpackedRestoreFile).parent_path().string())) {
                        logger->error("Failed to create restore directory: " + unpackedRestoreFile);
                        status = TaskStatus::FAILED;
                        // 清理临时文件和目录
                        std::filesystem::remove_all(tempUnpackDir);
                        if (needCleanup) {
                            std::filesystem::remove(tempFile);
                        }
                        return false;
                    }
                    
                    bool unpackedSuccess = false;
                    
                    // 检查是否是符号链接
                    bool isSymlink = std::filesystem::is_symlink(unpackedFilePath);
                    
                    if (isSymlink) {
                        // 处理符号链接文件
                        // 首先读取符号链接的原始目标
                        std::error_code ec;
                        std::filesystem::path originalSymlinkTarget = std::filesystem::read_symlink(unpackedFilePath, ec);
                        if (ec) {
                            logger->error("Failed to read symlink target: " + unpackedFilePath);
                            status = TaskStatus::FAILED;
                            // 清理临时文件和目录
                            std::filesystem::remove_all(tempUnpackDir);
                            if (needCleanup) {
                                std::filesystem::remove(tempFile);
                            }
                            return false;
                        }
                        
                        // 处理符号链接目标，去掉.enc和.huff扩展名
                        std::string symlinkTargetStr = originalSymlinkTarget.string();
                        std::string finalSymlinkTarget = symlinkTargetStr;
                        
                        // 如果符号链接目标带有.enc扩展名，去掉它
                        if (finalSymlinkTarget.size() > 4 && finalSymlinkTarget.substr(finalSymlinkTarget.size() - 4) == ".enc") {
                            finalSymlinkTarget = finalSymlinkTarget.substr(0, finalSymlinkTarget.size() - 4);
                        }
                        
                        // 如果符号链接目标带有.huff扩展名，去掉它
                        if (finalSymlinkTarget.size() > 5 && finalSymlinkTarget.substr(finalSymlinkTarget.size() - 5) == ".huff") {
                            finalSymlinkTarget = finalSymlinkTarget.substr(0, finalSymlinkTarget.size() - 5);
                        }
                        
                        // 现在创建符号链接，使用修改后的目标路径
                        std::filesystem::path symlinkDestPath(unpackedRestoreFile);
                        // 先删除可能存在的目标文件
                        if (std::filesystem::exists(symlinkDestPath, ec)) {
                            std::filesystem::remove(symlinkDestPath, ec);
                        }
                        // 创建符号链接
                        std::filesystem::create_symlink(finalSymlinkTarget, symlinkDestPath, ec);
                        if (ec) {
                            logger->error("Failed to create symlink: " + unpackedRestoreFile);
                            status = TaskStatus::FAILED;
                            // 清理临时文件和目录
                            std::filesystem::remove_all(tempUnpackDir);
                            if (needCleanup) {
                                std::filesystem::remove(tempFile);
                            }
                            return false;
                        }
                        
                        // 复制符号链接的元数据
                        std::filesystem::permissions(symlinkDestPath, std::filesystem::status(unpackedFilePath).permissions(), ec);
                        std::filesystem::last_write_time(symlinkDestPath, std::filesystem::last_write_time(unpackedFilePath, ec), ec);
                        
                        unpackedSuccess = true;
                    } else if (compressEnabled) {
                        // 检查是否需要解压
                        if ((unpackedFilePath.size() > 5 && unpackedFilePath.substr(unpackedFilePath.size() - 5) == ".huff")) {
                            logger->info("Decompressing file: " + unpackedFilePath);
                            
                            // 去掉目标文件的.huff扩展名
                            std::string finalDest = unpackedRestoreFile;
                            if (finalDest.size() > 5 && finalDest.substr(finalDest.size() - 5) == ".huff") {
                                finalDest = finalDest.substr(0, finalDest.size() - 5);
                            }
                            
                            // 保存原始文件的元数据
                            std::error_code ec;
                            auto originalFileTime = std::filesystem::last_write_time(unpackedFilePath, ec);
                            auto originalPermissions = std::filesystem::status(unpackedFilePath, ec).permissions();
                            
                            unpackedSuccess = FileSystem::decompressAndCopyFile(unpackedFilePath, finalDest);
                            
                            // 确保解压后的文件具有正确的元数据
                            if (unpackedSuccess && !ec) {
                                std::filesystem::last_write_time(finalDest, originalFileTime, ec);
                                if (ec) {
                                    logger->warn("Failed to copy file time to decompressed file: " + finalDest);
                                }
                                
                                std::filesystem::permissions(finalDest, originalPermissions, ec);
                                if (ec) {
                                    logger->warn("Failed to copy permissions to decompressed file: " + finalDest);
                                }
                            }
                        } else {
                            // 普通复制
                            unpackedSuccess = FileSystem::copyFile(unpackedFilePath, unpackedRestoreFile);
                        }
                    } else {
                        // 不压缩，直接复制
                        unpackedSuccess = FileSystem::copyFile(unpackedFilePath, unpackedRestoreFile);
                    }
                    
                     if (unpackedSuccess) {
                        logger->info("Restored: " + 
                                    ((compressEnabled && (unpackedFilePath.size() > 5 && unpackedFilePath.substr(unpackedFilePath.size() - 5) == ".huff")) ? 
                                    (unpackedRestoreFile.size() > 5 ? unpackedRestoreFile.substr(0, unpackedRestoreFile.size() - 5) : unpackedRestoreFile) : 
                                    unpackedRestoreFile));
                        successCount++;
                    } else {
                        logger->error("Failed to restore unpacked file: " + unpackedFilePath);
                        status = TaskStatus::FAILED;
                        // 清理临时文件和目录
                        std::filesystem::remove_all(tempUnpackDir);
                        if (needCleanup) {
                            std::filesystem::remove(tempFile);
                        }
                        return false;
                    }
                }
                
                // 清理临时解包目录
                std::filesystem::remove_all(tempUnpackDir);
                
                // 跳过当前文件的后续处理，因为已经完成解包和还原
                // 清理临时文件
                if (needCleanup) {
                    std::filesystem::remove(tempFile);
                }
                continue;
            }
        }
        
        // 3. 检查是否需要解压
        if (compressEnabled) {
            // 检查文件是否压缩
            // 处理以下情况：
            // 1. 原始文件是.huff格式（未加密）
            // 2. 原始文件是.huff.enc格式（加密），解密后变成临时文件
            // 3. 符号链接文件，需要特殊处理
            bool shouldDecompress = false;
            std::filesystem::path sourcePath(currentSource);
            std::filesystem::path originalPath(backupFilePath);
            
            // 检查是否是符号链接
            bool isSymlink = std::filesystem::is_symlink(sourcePath);
            
            if (isSymlink) {
                // 处理符号链接文件
                // 首先读取符号链接的原始目标
                std::error_code ec;
                std::filesystem::path originalSymlinkTarget = std::filesystem::read_symlink(sourcePath, ec);
                if (ec) {
                    logger->error("Failed to read symlink target: " + currentSource);
                    status = TaskStatus::FAILED;
                    // 清理临时文件
                    if (needCleanup) {
                        std::filesystem::remove(tempFile);
                    }
                    return false;
                }
                
                // 处理符号链接目标，去掉.enc和.huff扩展名
                std::string symlinkTargetStr = originalSymlinkTarget.string();
                std::string finalSymlinkTarget = symlinkTargetStr;
                
                // 如果符号链接目标带有.enc扩展名，去掉它
                if (finalSymlinkTarget.size() > 4 && finalSymlinkTarget.substr(finalSymlinkTarget.size() - 4) == ".enc") {
                    finalSymlinkTarget = finalSymlinkTarget.substr(0, finalSymlinkTarget.size() - 4);
                }
                
                // 如果符号链接目标带有.huff扩展名，去掉它
                if (finalSymlinkTarget.size() > 5 && finalSymlinkTarget.substr(finalSymlinkTarget.size() - 5) == ".huff") {
                    finalSymlinkTarget = finalSymlinkTarget.substr(0, finalSymlinkTarget.size() - 5);
                }
                
                // 现在创建符号链接，使用修改后的目标路径
                std::filesystem::path symlinkDestPath(currentDest);
                // 先删除可能存在的目标文件
                if (std::filesystem::exists(symlinkDestPath, ec)) {
                    std::filesystem::remove(symlinkDestPath, ec);
                }
                // 创建符号链接
                std::filesystem::create_symlink(finalSymlinkTarget, symlinkDestPath, ec);
                if (ec) {
                    logger->error("Failed to create symlink: " + currentDest);
                    status = TaskStatus::FAILED;
                    // 清理临时文件
                    if (needCleanup) {
                        std::filesystem::remove(tempFile);
                    }
                    return false;
                }
                
                // 复制符号链接的元数据
                std::filesystem::permissions(symlinkDestPath, std::filesystem::status(sourcePath).permissions(), ec);
                std::filesystem::last_write_time(symlinkDestPath, std::filesystem::last_write_time(sourcePath, ec), ec);
                
                logger->info("Restored: " + currentDest);
                successCount++;
            } else {
                // 非符号链接文件，检查是否需要解压
                // 如果当前文件是.huff格式，直接解压
                if (sourcePath.extension().string() == ".huff") {
                    shouldDecompress = true;
                }
                // 如果原始文件是.huff.enc格式（加密压缩），解密后也需要解压
                else if (originalPath.extension().string() == ".enc" && 
                         originalPath.stem().extension().string() == ".huff") {
                    shouldDecompress = true;
                }
                
                if (shouldDecompress) {
                    logger->info("Decompressing file: " + currentSource);
                    
                    // 去掉目标文件的.huff扩展名
                    std::string finalDest = currentDest;
                    if (finalDest.size() > 5 && finalDest.substr(finalDest.size() - 5) == ".huff") {
                        finalDest = finalDest.substr(0, finalDest.size() - 5);
                    }
                    
                    if (FileSystem::decompressAndCopyFile(currentSource, finalDest)) {
                        logger->info("Restored: " + finalDest);
                        successCount++;
                    } else {
                        logger->error("Decompression failed: " + currentSource);
                        status = TaskStatus::FAILED;
                        // 清理临时文件
                        if (needCleanup) {
                            std::filesystem::remove(tempFile);
                        }
                        return false;
                    }
                } else {
                    // 普通复制
                    if (FileSystem::copyFile(currentSource, currentDest)) {
                        logger->info("Restored: " + currentDest);
                        successCount++;
                    } else {
                        logger->error("Copy failed: " + currentSource + " -> " + currentDest);
                        status = TaskStatus::FAILED;
                        // 清理临时文件
                        if (needCleanup) {
                            std::filesystem::remove(tempFile);
                        }
                        return false;
                    }
                }
            }
        } else {
            // 不压缩，直接复制
            if (FileSystem::copyFile(currentSource, currentDest)) {
                logger->info("Restored: " + currentDest);
                successCount++;
            } else {
                logger->error("Copy failed: " + currentSource + " -> " + currentDest);
                status = TaskStatus::FAILED;
                // 清理临时文件
                if (needCleanup) {
                    std::filesystem::remove(tempFile);
                }
                return false;
            }
        }
        
        // 清理临时文件
        if (needCleanup) {
            std::filesystem::remove(tempFile);
        }
    }
    
    logger->info("Restore completed successfully. Restored " + std::to_string(successCount) + " files out of " + std::to_string(files.size()));
    status = TaskStatus::COMPLETED;
    return true;
}

TaskStatus RestoreTask::getStatus() const {
    return status;
}