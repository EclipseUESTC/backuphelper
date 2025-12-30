#include "FileSystem.hpp"
#include "../core/models/File.hpp"
#include "HuffmanCompressor.hpp"
#include <iostream>  // 仅用于调试（可选），正式版可移除
#include <stdexcept>
#include <string.h>
#include <functional>
#include <algorithm>
#include <iomanip>

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
    if (ec) {
        std::cerr << "Error: Failed to create directories for " << path << " (" << ec.message() << ")" << std::endl;
    }
    return result && !ec;
}

bool FileSystem::copyFile(const std::string& source, const std::string& destination) {
    
    // 确保目标目录存在
    fs::path destPath(destination);
    if (!destPath.parent_path().empty()) {
        std::error_code ec;
        fs::create_directories(destPath.parent_path(), ec);
        if (ec) {
            std::cerr << "Error: Failed to create directories for " << destPath.parent_path() << std::endl;
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
            std::cerr << "Error: Failed to read symlink " << source << " (" << ec.message() << ")" << std::endl;
            return false;
        }
        
        // 无论目标文件是否存在，都尝试创建符号链接
        // 但首先检查目标文件是否是符号链接，如果是则替换
        if (fs::exists(destPath, ec) && fs::is_symlink(destPath, ec)) {
            fs::remove(destPath, ec);
            if (ec) {
                std::cerr << "Error: Failed to remove existing symlink " << destination << std::endl;
                return false;
            }
        }
        
        // 创建符号链接，正确计算相对路径
        fs::path relativeTarget;
        if (symlinkTarget.is_absolute()) {
            relativeTarget = symlinkTarget;
        } else {
            // 对于相对路径，直接使用原始相对路径，不尝试解析
            // 这样可以避免循环链接问题，同时保持符号链接的相对特性
            relativeTarget = symlinkTarget;
        }
        
        // 尝试创建符号链接
        fs::create_symlink(relativeTarget, destPath, ec);
        if (ec) {
            // 如果创建失败，检查是否是因为目标文件已经存在
            if (ec.value() == static_cast<int>(std::errc::file_exists)) {
                // 目标文件已经存在，并且不是符号链接
                // 打印信息，说明跳过了符号链接创建
                std::cout << "Info: Skipping symlink creation for " << source << " -> " << destination << std::endl;
                std::cout << "  Reason: Target file already exists as a regular file or directory" << std::endl;
                // 继续执行，不中断备份操作
                return true;
            } else {
                // 其他错误，打印详细信息并返回失败
                std::cerr << "Error: Failed to create symlink " << destination << " -> " << relativeTarget 
                         << " (" << ec.message() << ")" << std::endl;
                return false;
            }
        }
        
        return true;
    }
    
    // 如果不是符号链接，使用 symlink_status 检查类型
    auto link_status = fs::symlink_status(sourcePath, ec);
    if (ec) {
        std::cerr << "Error: Failed to get file status for " << source << " (" << ec.message() << ")" << std::endl;
        return false;
    }
    
    if (fs::is_regular_file(link_status)) {
        
        // 普通文件
        // 检查目标文件是否是符号链接，如果是则先删除
        if (fs::is_symlink(destPath, ec)) {
            fs::remove(destPath, ec);
            if (ec) {
                std::cerr << "Error: Failed to remove existing symlink " << destination << std::endl;
                return false;
            }
        }
        
        if (!fs::copy_file(source, destination, fs::copy_options::overwrite_existing, ec)) {
            std::cerr << "Error: Failed to copy regular file from " << source << " to " << destination 
                     << " (" << ec.message() << ")" << std::endl;
            return false;
        }
        
    } else if (fs::is_directory(link_status)) {
        
        // 目录 - 只需要创建，内容会在遍历时处理
        // 检查目标文件是否是符号链接，如果是则先删除
        if (fs::is_symlink(destPath, ec)) {
            fs::remove(destPath, ec);
            if (ec) {
                std::cerr << "Error: Failed to remove existing symlink " << destination << std::endl;
                return false;
            }
        }
        
        if (!fs::exists(destPath, ec)) {
            fs::create_directories(destPath, ec);
            if (ec) {
                std::cerr << "Error: Failed to create directory " << destination << " (" << ec.message() << ")" << std::endl;
                return false;
            }
        }
        
    } else if (fs::is_fifo(link_status)) {
        
        // 命名管道
        if (fs::exists(destPath, ec)) {
            fs::remove(destPath, ec);
        }
        
        #ifdef _WIN32
            // Windows不支持直接创建类似Unix的命名管道文件
            std::cerr << "Error: FIFO files are not supported on Windows" << std::endl;
            return false;
        #else
            if (mkfifo(destination.c_str(), 0666) != 0) {
                std::cerr << "Error: Failed to create FIFO " << destination << " (" << strerror(errno) << ")" << std::endl;
                return false;
            }
        #endif
        
    } else {
        std::cerr << "Error: Unsupported file type for " << source << std::endl;
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
                 fs::directory_options::skip_permission_denied, 
                 ec)) {
            if (ec) break;
            // 收集所有类型的文件，包括符号链接
            files.emplace_back(entry.path());
        }
        
        // 收集空目录
        std::function<void(const fs::path&)> collectEmptyDirs = [&](const fs::path& dirPath) {
            for (const auto& entry : fs::directory_iterator(dirPath, ec)) {
                if (ec) break;
                if (fs::is_directory(entry.status())) {
                    bool exists = false;
                    for (const auto& file : files) {
                        if (file.getFilePath() == entry.path()) {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists) {
                        files.emplace_back(entry.path());
                    }
                    collectEmptyDirs(entry.path());
                }
            }
        };
        collectEmptyDirs(directory);
        
        // 确保符号链接被正确处理，不被其他文件类型覆盖
        // 按文件类型排序：真实文件 -> 目录 -> 符号链接 -> 其他类型
        std::sort(files.begin(), files.end(), [](const File& a, const File& b) {
            // 真实文件优先级最高
            if (a.isRegularFile() && !b.isRegularFile()) return true;
            if (!a.isRegularFile() && b.isRegularFile()) return false;
            
            // 然后是目录
            if (a.isDirectory() && !b.isDirectory()) return true;
            if (!a.isDirectory() && b.isDirectory()) return false;
            
            // 然后是符号链接
            if (!a.isSymbolicLink() && b.isSymbolicLink()) return true;
            if (a.isSymbolicLink() && !b.isSymbolicLink()) return false;
            
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
        // 确保 base 是目录（末尾带 /）
        if (!b.has_filename()) {
            b = b.parent_path();
        }
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
    
    // 如果压缩后的文件没有变小，删除压缩文件并返回false
    // 让调用者决定如何处理，避免将原始文件复制到.huff文件中
    if (compressedSize >= originalSize) {
        std::filesystem::remove(destination);
        std::cout << "Info: File compression skipped for " << source << std::endl;
        std::cout << "  Original size: " << originalSize << " bytes" << std::endl;
        std::cout << "  Compressed size: " << compressedSize << " bytes" << std::endl;
        std::cout << "  Compression not performed (compressed file is larger than original)" << std::endl;
        return false;
    }
    
    // 压缩成功，打印压缩效率
    double compressionRatio = static_cast<double>(originalSize - compressedSize) / originalSize * 100;
    std::cout << "Info: File compressed successfully: " << source << std::endl;
    std::cout << "  Original size: " << originalSize << " bytes" << std::endl;
    std::cout << "  Compressed size: " << compressedSize << " bytes" << std::endl;
    std::cout << "  Compression ratio: " << std::fixed << std::setprecision(2) << compressionRatio << "%" << std::endl;
    
    return true;
}

bool FileSystem::decompressFile(const std::string& source, const std::string& destination) {
    HuffmanCompressor compressor;
    return compressor.decompressFile(source, destination);
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
        std::cerr << "Info: Source is a symlink, copying directly" << std::endl;
        return copyFile(source, destination);
    }
    
    HuffmanCompressor compressor;
    
    // 先尝试解压
    if (compressor.decompressFile(source, destination)) {
        return true;
    }
    
    // 如果解压失败，可能是因为文件没有被压缩，直接复制
    std::cerr << "Warning: Failed to decompress file " << source << ", copying as regular file" << std::endl;
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
        std::cerr << "clearDirectory: Directory does not exist: " << path << ", Error: " << ec.message() << std::endl;
        return false;
    }
    if (!fs::is_directory(path, ec)) {
        std::cerr << "clearDirectory: Path is not a directory: " << path << ", Error: " << ec.message() << std::endl;
        return false;
    }
    
    // 遍历目录并删除所有内容
    for (const auto& entry : fs::directory_iterator(path, ec)) {
        if (ec) {
            std::cerr << "clearDirectory: Failed to iterate directory: " << path << ", Error: " << ec.message() << std::endl;
            return false;
        }
        
        const auto& entryPath = entry.path();
        if (entry.is_directory(ec)) {
            if (!fs::remove_all(entryPath, ec)) {
                // 检查是否真的有错误
                if (ec) {
                    std::cerr << "clearDirectory: Failed to remove directory: " << entryPath.string() << ", Error: " << ec.message() << std::endl;
                    return false;
                }
                // 如果没有错误码，说明目录已经不存在，继续处理
            }
        } else {
            if (!fs::remove(entryPath, ec)) {
                // 检查是否真的有错误
                if (ec) {
                    std::cerr << "clearDirectory: Failed to remove file: " << entryPath.string() << ", Error: " << ec.message() << std::endl;
                    return false;
                }
                // 如果没有错误码，说明文件已经不存在，继续处理
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
        std::cerr << "copyDirectory: Source directory does not exist: " << sourceDir << ", Error: " << ec.message() << std::endl;
        return false;
    }
    if (!fs::is_directory(sourceDir, ec)) {
        std::cerr << "copyDirectory: Source path is not a directory: " << sourceDir << ", Error: " << ec.message() << std::endl;
        return false;
    }
    
    // 创建目标目录
    if (!fs::create_directories(destDir, ec)) {
        // 检查是否因为目录已存在而失败（这是正常情况，不是错误）
        if (ec) {
            std::cerr << "copyDirectory: Failed to create destination directory: " << destDir << ", Error: " << ec.message() << std::endl;
            return false;
        }
        // 目录已存在，这是正常情况，继续执行
        std::cout << "copyDirectory: Destination directory already exists: " << destDir << std::endl;
    }
    
    // 遍历源目录并复制所有内容
    for (const auto& entry : fs::directory_iterator(sourceDir, ec)) {
        if (ec) {
            std::cerr << "copyDirectory: Failed to iterate source directory: " << sourceDir << ", Error: " << ec.message() << std::endl;
            return false;
        }
        
        const auto& sourcePath = entry.path();
        const auto destPath = fs::path(destDir) / sourcePath.filename();
        
        // 关键：先检查是否是符号链接
        if (fs::is_symlink(sourcePath)) {
            // 使用copyFile函数处理符号链接
            if (!copyFile(sourcePath.string(), destPath.string())) {
                std::cerr << "copyDirectory: Failed to copy symlink: " << sourcePath.string() << " to " << destPath.string() << std::endl;
                return false;
            }
        } else if (entry.is_directory(ec)) {
            if (!copyDirectory(sourcePath.string(), destPath.string())) {
                std::cerr << "copyDirectory: Failed to copy subdirectory: " << sourcePath.string() << " to " << destPath.string() << std::endl;
                return false;
            }
        } else {
            if (!fs::copy_file(sourcePath, destPath, fs::copy_options::overwrite_existing, ec)) {
                std::cerr << "copyDirectory: Failed to copy file: " << sourcePath.string() << " to " << destPath.string() << ", Error: " << ec.message() << std::endl;
                return false;
            }
        }
    }
    
    return !ec;
}