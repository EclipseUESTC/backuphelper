#include "FileSystem.hpp"
#include "../core/models/File.hpp"
#include "HuffmanCompressor.hpp"
#include <iostream>  // 仅用于调试（可选），正式版可移除
#include <stdexcept>
#include <string.h>
#include <functional>
#include <algorithm>
#include <iomanip>
#include <sstream>

// 跨平台头文件包含
#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <errno.h>
    // 定义Windows常量以便跨平台使用
    #define ERROR_ALREADY_EXISTS 183
#endif

namespace fs = std::filesystem;

bool FileSystem::exists(const std::string& path) {
    std::error_code ec;
    // 使用symlink_status检查文件是否存在，不解析符号链接
    fs::file_status status = fs::symlink_status(path, ec);
    bool result = status.type() != fs::file_type::not_found && !ec;
    // 不抛出异常，静默处理错误（如权限不足）
    return result;
}

bool FileSystem::createDirectories(const std::string& path) {
    // 如果目录已存在，直接返回成功
    std::error_code ec;
    
    // 使用symlink_status检查文件状态，不解析符号链接
    fs::file_status status = fs::symlink_status(path, ec);
    if (!ec && (status.type() == fs::file_type::directory || status.type() == fs::file_type::symlink)) {
        return true;
    }
    
    // 否则创建目录
    bool result = fs::create_directories(path, ec);
    // 只在创建失败时返回错误，不输出信息
    return result && !ec;
}

bool FileSystem::copyFile(const std::string& source, const std::string& destination) {
    
    // 确保目标目录存在
    fs::path destPath(destination);
    if (!destPath.parent_path().empty()) {
        std::error_code ec;
        fs::create_directories(destPath.parent_path(), ec);
        if (ec) {
            return false;
        }
    }

    fs::path sourcePath(source);
    std::error_code ec;
    
    // 检查源文件和目标文件是否相同，避免循环复制
    // 对于符号链接，我们不使用canonical，因为它会解析链接
    // 直接比较路径字符串，避免符号链接解析问题
    if (sourcePath.string() == destPath.string()) {
        return true;
    }
    
    // 关键：先检查是否是符号链接
    if (fs::is_symlink(sourcePath)) {
        // 读取符号链接目标
        fs::path symlinkTarget = fs::read_symlink(sourcePath, ec);
        if (ec) {
            return false;
        }
        
        // 无论目标文件是否存在，都尝试创建符号链接
        // 但首先检查目标文件是否是符号链接，如果是则替换
        if (fs::exists(destPath, ec)) {
            if (fs::is_symlink(destPath, ec)) {
                fs::remove(destPath, ec);
                if (ec) {
                    return false;
                }
            } else {
                // 目标文件已存在且不是符号链接，返回失败
                return false;
            }
        }
        
        // 创建符号链接，保持原始目标路径
        // 对于相对路径，直接使用原始相对路径
        // 对于绝对路径，保持绝对路径
        fs::create_symlink(symlinkTarget, destPath, ec);
        if (ec) {
            return false;
        }
        
        // 复制符号链接的元数据（权限、时间戳等）
        fs::permissions(destPath, fs::symlink_status(sourcePath).permissions(), ec);
        
        // 复制符号链接的时间戳
        std::error_code timeEc;
        fs::last_write_time(destPath, fs::last_write_time(sourcePath, timeEc), timeEc);
        
        return true;
    }
    
    // 如果不是符号链接，使用 symlink_status 检查类型
    auto link_status = fs::symlink_status(sourcePath, ec);
    if (ec) {
        return false;
    }
    
    if (fs::is_regular_file(link_status)) {
        
        // 普通文件
        // 检查目标文件是否是符号链接，如果是则跳过复制，保留符号链接
        if (fs::is_symlink(destPath, ec)) {
            // 目标已存在且是符号链接，保留符号链接，跳过复制
            return true;
        }
        
        // 检查目标文件是否存在
        if (fs::exists(destPath, ec) && !ec) {
            // 目标文件存在，比较文件内容是否相同
            std::string sourceHash = calculateFileHash(source);
            std::string destHash = calculateFileHash(destination);
            
            if (!sourceHash.empty() && !destHash.empty() && sourceHash == destHash) {
                // 文件内容相同，不需要复制
                return true;
            }
        }
        
        // 目标文件不存在或内容不同，执行复制
        if (!fs::copy_file(source, destination, fs::copy_options::overwrite_existing | fs::copy_options::copy_symlinks, ec)) {
            return false;
        }
        
        // 复制文件元数据（权限、时间戳等）
        fs::permissions(destination, fs::status(source).permissions(), ec);
        
        // 复制时间戳
        std::error_code timeEc;
        fs::last_write_time(destination, fs::last_write_time(source, timeEc), timeEc);
        
    } else if (fs::is_directory(link_status)) {
        
        // 目录 - 只需要创建，内容会在遍历时处理
        // 检查目标文件是否是符号链接，如果是则跳过，保留符号链接
        if (fs::is_symlink(destPath, ec)) {
            // 目标已存在且是符号链接，保留符号链接，跳过目录创建
            return true;
        }
        
        if (!fs::exists(destPath, ec)) {
            fs::create_directories(destPath, ec);
            if (ec) {
                return false;
            }
        }
        
        // 复制目录的元数据（权限、时间戳等）
        fs::permissions(destPath, fs::status(sourcePath).permissions(), ec);
        
        // 复制目录的时间戳
        std::error_code timeEc;
        fs::last_write_time(destPath, fs::last_write_time(sourcePath, timeEc), timeEc);
        
    } else if (fs::is_fifo(link_status)) {
        
        // 命名管道
        if (fs::exists(destPath, ec)) {
            fs::remove(destPath, ec);
        }
        
        #ifdef _WIN32
            // Windows不支持直接创建类似Unix的命名管道文件
            return false;
        #else
            if (mkfifo(destination.c_str(), 0666) != 0) {
                return false;
            }
        #endif
        
    } else {
        // 不支持的文件类型
        return false;
    }
    
    return true;
}

std::vector<File> FileSystem::getAllFiles(const std::string& directory) {
    std::vector<File> files;
    std::error_code ec;

    // 使用symlink_status检查目录是否存在和是否为目录，不解析符号链接
    fs::file_status status = fs::symlink_status(directory, ec);
    if (ec || status.type() != fs::file_type::directory) {
        return files; // 返回空列表
    }

    try {
        // 收集所有文件和目录
        for (const auto& entry : fs::recursive_directory_iterator(
                 directory, 
                 fs::directory_options::skip_permission_denied | fs::directory_options::follow_directory_symlink, 
                 ec)) {
            if (ec) break;
            // 收集所有类型的文件，包括符号链接
            files.emplace_back(entry.path());
        }
        
        // 收集符号链接指向的文件和目录
        // 创建一个临时集合来存储已处理的文件路径，避免重复
        std::set<fs::path> processedPaths;
        for (const auto& file : files) {
            processedPaths.insert(file.getFilePath());
        }
        
        // 遍历所有文件，检查符号链接指向的文件是否已被收集
        for (const auto& file : files) {
            if (file.isSymbolicLink()) {
                // 读取符号链接目标
                fs::path symlinkTarget = fs::read_symlink(file.getFilePath(), ec);
                if (!ec) {
                    // 计算符号链接目标的完整路径
                    fs::path fullTargetPath;
                    if (symlinkTarget.is_absolute()) {
                        fullTargetPath = symlinkTarget;
                    } else {
                        fullTargetPath = file.getFilePath().parent_path() / symlinkTarget;
                    }
                    
                    // 检查目标是否存在，并且是源目录中的文件
                    if (fs::exists(fullTargetPath) && 
                        fullTargetPath.string().find(directory) == 0 && 
                        processedPaths.find(fullTargetPath) == processedPaths.end()) {
                        // 将目标文件添加到结果中
                        files.emplace_back(fullTargetPath);
                        processedPaths.insert(fullTargetPath);
                        
                        // 如果目标是目录，递归收集其内容
                        if (fs::is_directory(fullTargetPath, ec) && !ec) {
                            for (const auto& entry : fs::recursive_directory_iterator(
                                     fullTargetPath, 
                                     fs::directory_options::skip_permission_denied | fs::directory_options::follow_directory_symlink, 
                                     ec)) {
                                if (ec) break;
                                if (processedPaths.find(entry.path()) == processedPaths.end()) {
                                    files.emplace_back(entry.path());
                                    processedPaths.insert(entry.path());
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // 收集空目录
        std::function<void(const fs::path&)> collectEmptyDirs = [&](const fs::path& dirPath) {
            for (const auto& entry : fs::directory_iterator(dirPath, ec)) {
                if (ec) break;
                if (fs::is_directory(entry.symlink_status())) {
                    bool exists = false;
                    for (const auto& file : files) {
                        if (file.getFilePath() == entry.path()) {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists) {
                        files.emplace_back(entry.path());
                        processedPaths.insert(entry.path());
                    }
                    collectEmptyDirs(entry.path());
                }
            }
        };
        collectEmptyDirs(directory);
        
        // 确保符号链接被正确处理，不被其他文件类型覆盖
        // 按文件类型排序：符号链接 -> 真实文件 -> 目录 -> 其他类型
        // 符号链接优先处理，避免真实文件覆盖符号链接
        std::sort(files.begin(), files.end(), [](const File& a, const File& b) {
            // 符号链接优先级最高
            if (a.isSymbolicLink() && !b.isSymbolicLink()) return true;
            if (!a.isSymbolicLink() && b.isSymbolicLink()) return false;
            
            // 然后是真实文件
            if (a.isRegularFile() && !b.isRegularFile()) return true;
            if (!a.isRegularFile() && b.isRegularFile()) return false;
            
            // 然后是目录
            if (a.isDirectory() && !b.isDirectory()) return true;
            if (!a.isDirectory() && b.isDirectory()) return false;
            
            // 最后按路径排序
            return a.getFilePath().string() < b.getFilePath().string();
        });
    } catch (const fs::filesystem_error&) {
        // 忽略异常，返回已收集的文件（或空）
    }
    return files;
}

uint64_t FileSystem::getFileSize(const std::string& filePath) {
    std::error_code ec;
    auto size = fs::file_size(filePath, ec);
    return ec ? 0 : size;
}

std::string FileSystem::getRelativePath(const std::string& path, const std::string& base) {
    try {
        fs::path p(path);
        fs::path b(base);
        // 直接使用base作为基准目录，不做任何修改
        fs::path relative = fs::relative(p, b);
        return relative.string();
    } catch (const fs::filesystem_error&) {
        // 若无法计算相对路径，返回原路径（或 basename）
        return fs::path(path).filename().string();
    }
}

bool FileSystem::compressFile(const std::string& source, const std::string& destination) {
    HuffmanCompressor compressor;
    
    // 获取原始文件大小
    uint64_t originalSize = getFileSize(source);
    
    // 先尝试压缩
    if (!compressor.compressFile(source, destination)) {
        return false;
    }
    
    // 检查压缩效率
    uint64_t compressedSize = getFileSize(destination);
    
    // 如果压缩后的文件没有变小，输出警告信息并删除压缩文件
    if (compressedSize >= originalSize) {
        std::cerr << "Warning: Compressed file is not smaller than original file. "
                  << "Original size: " << originalSize << " bytes, "
                  << "Compressed size: " << compressedSize << " bytes. "
                  << "File: " << source << std::endl;
        
        // 删除压缩文件
        std::error_code ec;
        fs::remove(destination, ec);
        
        return false;
    }
    
    // 复制原始文件的元数据到压缩文件
    std::error_code ec;
    
    // 复制权限
    fs::permissions(destination, fs::status(source).permissions(), ec);
    
    // 复制时间戳
    try {
        auto fileTime = fs::last_write_time(source, ec);
        if (!ec) {
            fs::last_write_time(destination, fileTime, ec);
        }
    } catch (const std::exception&) {
        // 静默处理元数据复制异常
    }
    
    // 压缩成功且文件变小，返回true
    return true;
}

bool FileSystem::decompressFile(const std::string& source, const std::string& destination) {
    HuffmanCompressor compressor;
    
    // 保存原始压缩文件的元数据
    std::error_code ec;
    auto originalFileTime = fs::last_write_time(source, ec);
    auto originalPermissions = fs::status(source, ec).permissions();
    
    // 先解压缩文件
    if (!compressor.decompressFile(source, destination)) {
        return false;
    }
    
    // 复制压缩文件的元数据到解压缩文件
    
    // 复制时间戳
    if (!ec) {
        // 确保时间戳有效，避免1901年问题
        auto currentTime = fs::last_write_time(source, ec);
        if (!ec && currentTime.time_since_epoch().count() > 0) {
            fs::last_write_time(destination, currentTime, ec);
        } else {
            // 如果时间戳无效，使用当前时间
            auto currentTimeNow = fs::last_write_time(source, ec);
            if (!ec) {
                fs::last_write_time(destination, currentTimeNow, ec);
            }
        }
    }
    
    // 复制权限
    if (!ec) {
        fs::permissions(destination, originalPermissions, ec);
    }
    
    return true;
}

bool FileSystem::copyAndCompressFile(const std::string& source, const std::string& destination) {
    // 先尝试压缩文件
    if (compressFile(source, destination)) {
        return true;
    }
    
    // 如果压缩失败，直接复制原始文件，不添加.huff扩展名
    // 但首先确保目标文件不存在，避免生成无效文件
    std::error_code ec;
    fs::remove(destination, ec);
    
    // 去掉.huff扩展名，获取原始文件名
    std::string originalDest = destination;
    if (originalDest.size() > 5 && originalDest.substr(originalDest.size() - 5) == ".huff") {
        originalDest = originalDest.substr(0, originalDest.size() - 5);
    }
    
    // 直接复制原始文件到原始文件名
    return copyFile(source, originalDest);
}

bool FileSystem::decompressAndCopyFile(const std::string& source, const std::string& destination) {
    // 先检查源文件是否是符号链接
    if (fs::is_symlink(source)) {
        // 符号链接直接复制，不尝试解压
        return copyFile(source, destination);
    }
    
    // 调用我们修改过的decompressFile函数，它会处理元数据
    if (decompressFile(source, destination)) {
        return true;
    }
    
    // 如果解压失败，可能是因为文件没有被压缩，直接复制
    return copyFile(source, destination);
}

bool FileSystem::removeFile(const std::string& path) {
    std::error_code ec;
    bool result = fs::remove(path, ec);
    return !ec && result;
}

// 清空目录内容
bool FileSystem::clearDirectory(const std::string& path) {
    std::error_code ec;
    
    // 检查目录是否存在
    if (!fs::exists(path, ec)) {
        return false;
    }
    if (!fs::is_directory(path, ec)) {
        return false;
    }
    
    // 遍历目录并删除所有内容
    for (const auto& entry : fs::directory_iterator(path, ec)) {
        if (ec) {
            return false;
        }
        
        const auto& entryPath = entry.path();
        if (entry.is_directory(ec)) {
            if (!fs::remove_all(entryPath, ec)) {
                // 检查是否真的有错误
                if (ec) {
                    return false;
                }
            }
        } else {
            if (!fs::remove(entryPath, ec)) {
                // 检查是否真的有错误
                if (ec) {
                    return false;
                }
            }
        }
    }
    
    return !ec;
}

// 复制目录（递归）
bool FileSystem::copyDirectory(const std::string& sourceDir, const std::string& destDir) {
    std::error_code ec;
    
    // 检查源目录是否存在
    if (!fs::exists(sourceDir, ec)) {
        return false;
    }
    if (!fs::is_directory(sourceDir, ec)) {
        return false;
    }
    
    // 创建目标目录
    if (!fs::create_directories(destDir, ec)) {
        // 检查是否因为目录已存在而失败（这是正常情况，不是错误）
        if (ec) {
            return false;
        }
    }
    
    // 遍历源目录并复制所有内容
    for (const auto& entry : fs::directory_iterator(sourceDir, ec)) {
        if (ec) {
            return false;
        }
        
        const auto& sourcePath = entry.path();
        const auto destPath = fs::path(destDir) / sourcePath.filename();
        
        // 关键：先检查是否是符号链接
        if (fs::is_symlink(sourcePath)) {
            // 使用copyFile函数处理符号链接
            if (!copyFile(sourcePath.string(), destPath.string())) {
                return false;
            }
        } else if (entry.is_directory(ec)) {
            if (!copyDirectory(sourcePath.string(), destPath.string())) {
                return false;
            }
        } else {
            if (!fs::copy_file(sourcePath, destPath, fs::copy_options::overwrite_existing, ec)) {
                return false;
            }
        }
    }
    
    return !ec;
}

// 计算文件的哈希值，用于检测文件内容是否变化
// 使用文件大小和修改时间的组合，简单高效
std::string FileSystem::calculateFileHash(const std::string& filePath) {
    std::error_code ec;
    
    // 检查文件是否存在
    if (!fs::exists(filePath, ec)) {
        return "";
    }
    
    // 获取文件大小
    auto fileSize = fs::file_size(filePath, ec);
    if (ec) {
        return "";
    }
    
    // 获取文件最后修改时间
    auto lastWriteTime = fs::last_write_time(filePath, ec);
    if (ec) {
        return "";
    }
    
    // 将文件大小和修改时间组合成一个字符串作为哈希值
    std::stringstream ss;
    ss << fileSize << ":" << lastWriteTime.time_since_epoch().count();
    
    return ss.str();
}

