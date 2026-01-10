#include "FilePackager.hpp"
#include "HuffmanCompressor.hpp"
#include <filesystem>
#include <iostream>
#include <sys/types.h>  // 用于 mkfifo
#include <sys/stat.h>   // 用于 mkfifo
#include <cerrno>       // 用于 errno
#include <cstring>      // 用于 strerror

// Windows平台特定头文件
#ifdef _WIN32
    #define NOMINMAX  // 避免与C++标准库中的max宏冲突
    #include <windows.h>
    #include <time.h>  // 用于 localtime_s
#else
    #include <fcntl.h>  // 用于 utimensat
    #include <time.h>   // 用于 struct timespec
#endif

namespace fs = std::filesystem;

// 实现FileMetadata从File对象的构造函数
FileMetadata::FileMetadata(const File& file, const std::filesystem::path& basePath) {
    // Calculate relative filename
    this->filename = file.getRelativePath(basePath).string();
    
    // Basic file info
    this->fileSize = file.getFileSize();
    this->isCompressed = (this->filename.size() > 5 && this->filename.substr(this->filename.size() - 5) == ".huff");
    
    // Permissions and ownership
    this->permissions = file.getPermissions();
    
    // Time stamps
    // 将system_clock::time_point直接转换为Unix时间戳（秒）
    // Unix时间戳是从1970-01-01 00:00:00 UTC开始的秒数
    auto toUnixTime = [](const std::chrono::system_clock::time_point& tp) {
        // 使用duration_cast将time_point转换为秒
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
            tp.time_since_epoch()
        ).count();
        // 确保时间戳为正数，避免uint64_t溢出导致1901年问题
        return seconds > 0 ? static_cast<uint64_t>(seconds) : 0;
    };
    
    // 分别获取创建时间、最后修改时间和最后访问时间
    this->creationTime = toUnixTime(file.getCreationTime());
    this->lastModifiedTime = toUnixTime(file.getLastModifiedTime());
    this->lastAccessTime = toUnixTime(file.getLastAccessTime());
    
    // File type
    if (file.isRegularFile()) {
        this->fileType = 0;
    } else if (file.isDirectory()) {
        this->fileType = 1;
    } else if (file.isSymbolicLink()) {
        this->fileType = 2;
        this->symlinkTarget = file.getSymlinkTarget().string();
    } else if (file.isFIFO()) {
        this->fileType = 3;
    } else if (file.isCharacterDevice()) {
        this->fileType = 4;
    } else if (file.isBlockDevice()) {
        this->fileType = 5;
    } else if (file.isSocket()) {
        this->fileType = 6;
    }
}

FilePackager::FilePackager() {
}

FilePackager::~FilePackager() {
}

// 实现新的packageFiles方法，处理File对象
bool FilePackager::packageFiles(const std::vector<File>& inputFiles, const std::string& outputFile, const std::string& basePath) {
    try {
        std::ofstream outFile(outputFile, std::ios::binary);
        if (!outFile) {
            std::cerr << "Error: Cannot create output file: " << outputFile << std::endl;
            return false;
        }

        // 保留元数据的位置
        uint64_t metadataOffset = 0;
        outFile.write(reinterpret_cast<const char*>(&metadataOffset), sizeof(metadataOffset));

        // 收集文件元数据
        std::vector<FileMetadata> metadata;
        uint64_t currentOffset = sizeof(metadataOffset);

        // 使用提供的basePath，默认为输出文件的父目录
        fs::path actualBasePath = basePath.empty() ? fs::path(outputFile).parent_path() : fs::path(basePath);

        for (const auto& file : inputFiles) {
            // 创建文件元数据
            FileMetadata fileMeta(file, actualBasePath);
            fileMeta.offset = currentOffset;
            
            // 只有普通文件需要写入内容
            if (file.isRegularFile()) {
                // 确保文件数据已加载
                File mutableFile = file;
                if (!mutableFile.loadFileData()) {
                    std::cerr << "Error: Cannot load file data for " << file.getFilePath() << std::endl;
                    outFile.close();
                    fs::remove(outputFile);
                    return false;
                }
                
                const auto& fileData = mutableFile.getFileData();
                fileMeta.fileSize = fileData.size();
                
                // 写入文件内容
                outFile.write(fileData.data(), fileData.size());
                
                // 更新偏移量
                currentOffset += fileData.size();
            }
            
            metadata.push_back(fileMeta);
        }

        // 写入元数据
        uint64_t actualMetadataOffset = outFile.tellp();
        if (!writeMetadata(metadata, outFile)) {
            outFile.close();
            fs::remove(outputFile);
            return false;
        }

        // 更新元数据偏移量
        outFile.seekp(0, std::ios::beg);
        outFile.write(reinterpret_cast<const char*>(&actualMetadataOffset), sizeof(actualMetadataOffset));

        outFile.close();
        std::cout << "Packaging completed successfully!" << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error during packaging: " << e.what() << std::endl;
        return false;
    }
}

// 兼容旧接口，内部转换为File对象
bool FilePackager::packageFiles(const std::vector<std::string>& inputFiles, const std::string& outputFile) {
    // Convert string paths to File objects and check if they exist
    std::vector<File> files;
    for (const auto& path : inputFiles) {
        files.emplace_back(path);
        // 检查文件是否存在
        if (!files.back().exists()) {
            std::cerr << "Error: File not found: " << path << std::endl;
            return false;
        }
    }
    return packageFiles(files, outputFile);
}

// 兼容旧接口，内部转换为File对象，支持basePath
bool FilePackager::packageFiles(const std::vector<std::string>& inputFiles, const std::string& outputFile, const std::string& basePath) {
    // Convert string paths to File objects and check if they exist
    std::vector<File> files;
    for (const auto& path : inputFiles) {
        files.emplace_back(path);
        // 检查文件是否存在
        if (!files.back().exists()) {
            std::cerr << "Error: File not found: " << path << std::endl;
            return false;
        }
    }
    return packageFiles(files, outputFile, basePath);
}

// 将FileMetadata转换为File对象
File FilePackager::metadataToFile(const FileMetadata& metadata, const std::string& outputDir) const {
    // Create full path
    fs::path fullPath = fs::path(outputDir) / metadata.filename;
    
    // Create File object
    File file(fullPath);
    
    return file;
}

// 解包单个文件并返回File对象列表
std::vector<File> FilePackager::unpackFilesToFiles(const std::string& inputFile, const std::string& outputDir) {
    std::vector<File> files;
    
    // First unpack the files to disk
    if (!unpackFiles(inputFile, outputDir)) {
        return files;
    }
    
    // Then read the unpacked files into File objects
    try {
        std::ifstream inFile(inputFile, std::ios::binary);
        if (!inFile) {
            std::cerr << "Error: Cannot open input file: " << inputFile << std::endl;
            return files;
        }
        
        // Read metadata offset
        uint64_t metadataOffset = 0;
        inFile.read(reinterpret_cast<char*>(&metadataOffset), sizeof(metadataOffset));
        
        // Seek to metadata
        inFile.seekg(metadataOffset, std::ios::beg);
        
        // Read metadata
        std::vector<FileMetadata> metadata;
        if (!readMetadata(inFile, metadata)) {
            inFile.close();
            return files;
        }
        
        inFile.close();
        
        // Create File objects from metadata
        for (const auto& fileMeta : metadata) {
            fs::path fullPath = fs::path(outputDir) / fileMeta.filename;
            files.emplace_back(fullPath);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error during unpacking to File objects: " << e.what() << std::endl;
    }
    
    return files;
}



bool FilePackager::unpackFiles(const std::string& inputFile, const std::string& outputDir) {
    try {
        std::ifstream inFile(inputFile, std::ios::binary);
        if (!inFile) {
            std::cerr << "Error: Cannot open input file: " << inputFile << std::endl;
            return false;
        }

        // 读取元数据偏移量
        uint64_t metadataOffset = 0;
        inFile.read(reinterpret_cast<char*>(&metadataOffset), sizeof(metadataOffset));

        // 跳转到元数据位置
        inFile.seekg(metadataOffset, std::ios::beg);

        // 读取元数据
        std::vector<FileMetadata> metadata;
        if (!readMetadata(inFile, metadata)) {
            inFile.close();
            return false;
        }

        // 创建输出目录
        fs::create_directories(outputDir);

        // 解包每个文件
        for (const auto& fileMeta : metadata) {
            std::string outputPath = (fs::path(outputDir) / fileMeta.filename).string();
            fs::path outputFsPath(outputPath);
            std::error_code ec;
            
            // 创建文件的父目录
            fs::path parentDir = outputFsPath.parent_path();
            if (!parentDir.empty()) {
                fs::create_directories(parentDir, ec);
                if (ec) {
                    std::cerr << "Error: Cannot create directory: " << parentDir 
                              << " (" << ec.message() << ")" << std::endl;
                    inFile.close();
                    return false;
                }
            }
            
            // 根据文件类型处理
            if (fileMeta.fileType == 0) {
                // 普通文件
                std::ofstream outFile(outputPath, std::ios::binary);
                if (!outFile) {
                    std::cerr << "Error: Cannot create output file: " << outputPath << std::endl;
                    inFile.close();
                    return false;
                }

                // 跳转到文件数据位置
                inFile.seekg(fileMeta.offset, std::ios::beg);

                // 读取并写入文件内容
                char buffer[8192];
                uint64_t bytesRead = 0;
                while (bytesRead < fileMeta.fileSize) {
                    uint64_t bytesToRead = std::min<uint64_t>(sizeof(buffer), 
                                                              fileMeta.fileSize - bytesRead);
                    inFile.read(buffer, bytesToRead);
                    outFile.write(buffer, inFile.gcount());
                    bytesRead += inFile.gcount();
                    
                    // 检查读取错误
                    if (!inFile) {
                        std::cerr << "Error: Failed to read file data for " << outputPath << std::endl;
                        outFile.close();
                        inFile.close();
                        return false;
                    }
                }

                outFile.close();
            } else if (fileMeta.fileType == 1) {
                // 目录
                fs::create_directories(outputFsPath, ec);
                if (ec) {
                    std::cerr << "Error: Cannot create directory: " << outputPath 
                              << " (" << ec.message() << ")" << std::endl;
                    inFile.close();
                    return false;
                }
            } else if (fileMeta.fileType == 2) {
                // 符号链接
                // 在创建符号链接前确保目标文件/目录不存在
                if (fs::exists(outputFsPath, ec)) {
                    fs::remove(outputFsPath, ec);
                }
                fs::create_symlink(fileMeta.symlinkTarget, outputFsPath, ec);
                if (ec) {
                    std::cerr << "Error: Cannot create symlink: " << outputPath 
                              << " -> " << fileMeta.symlinkTarget 
                              << " (" << ec.message() << ")" << std::endl;
                    inFile.close();
                    return false;
                }
            } else if (fileMeta.fileType == 3) {
                // FIFO文件（命名管道）
                #ifdef _WIN32
                    // Windows不支持直接创建类似Unix的命名管道文件
                    std::cerr << "Warning: FIFO files are not supported on Windows, skipping " 
                              << outputPath << std::endl;
                #else
                    // 删除已存在的文件
                    if (fs::exists(outputFsPath, ec)) {
                        fs::remove(outputFsPath, ec);
                    }
                    // 创建FIFO文件
                    if (mkfifo(outputPath.c_str(), 0666) != 0) {
                        std::cerr << "Error: Failed to create FIFO " << outputPath 
                                  << " (" << strerror(errno) << ")" << std::endl;
                        // 继续执行，不中断整个解包过程
                    }
                #endif
            } else {
                std::cerr << "Warning: Unknown file type " << fileMeta.fileType 
                          << " for file " << outputPath << ", skipping..." << std::endl;
                continue;
            }
            
            // 恢复文件权限（不适用于符号链接，因为符号链接没有独立的权限）
            if (fileMeta.fileType != 2) {  // 不是符号链接
                fs::permissions(outputFsPath, fs::perms(fileMeta.permissions), 
                                fs::perm_options::replace, ec);
                if (ec) {
                    std::cerr << "Warning: Cannot set permissions for " << outputPath 
                              << " (" << ec.message() << ")" << std::endl;
                }
            }
            
            // 恢复文件时间戳
            // 注意：使用平台特定的API来设置文件时间，避免时钟系统差异导致的问题
            // 普通文件、目录和符号链接都需要恢复时间戳
            if (fileMeta.fileType == 0 || fileMeta.fileType == 1 || fileMeta.fileType == 2) {  // 普通文件、目录和符号链接
                try {
                    // 确保时间戳有效（避免1901年问题）
                    // 转换Unix时间戳为time_t，Unix时间戳已经是UTC时间，不需要再转换
                    time_t creationTime = static_cast<time_t>(fileMeta.creationTime);
                    time_t lastAccessTime = static_cast<time_t>(fileMeta.lastAccessTime);
                    time_t lastModifiedTime = static_cast<time_t>(fileMeta.lastModifiedTime);
                    
                    // 如果时间戳为0（1970-01-01），使用当前时间作为默认值
                    if (fileMeta.creationTime == 0) creationTime = time(nullptr);
                    if (fileMeta.lastAccessTime == 0) lastAccessTime = time(nullptr);
                    if (fileMeta.lastModifiedTime == 0) lastModifiedTime = time(nullptr);
                    
                    #ifdef _WIN32
                        // Windows平台实现：使用Windows API直接设置文件时间
                        HANDLE hFile = CreateFileA(outputPath.c_str(), FILE_WRITE_ATTRIBUTES, 0, NULL, 
                                                 OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
                        if (hFile != INVALID_HANDLE_VALUE) {
                            FILETIME ftCreation, ftAccess, ftWrite;
                            SYSTEMTIME st;
                            
                            // 将时间戳直接转换为FILETIME，不经过localtime_s，避免时区问题
                            // Unix时间戳是从1970-01-01 00:00:00 UTC开始的秒数
                            // FILETIME是从1601-01-01 00:00:00 UTC开始的100纳秒间隔数
                            const uint64_t FILETIME_EPOCH_DIFF = 11644473600ULL; // 1970-01-01到1601-01-01的秒数
                            
                            // 转换创建时间
                            uint64_t creationFileTime = (static_cast<uint64_t>(creationTime) + FILETIME_EPOCH_DIFF) * 10000000ULL;
                            ftCreation.dwLowDateTime = static_cast<DWORD>(creationFileTime);
                            ftCreation.dwHighDateTime = static_cast<DWORD>(creationFileTime >> 32);
                            
                            // 转换访问时间
                            uint64_t accessFileTime = (static_cast<uint64_t>(lastAccessTime) + FILETIME_EPOCH_DIFF) * 10000000ULL;
                            ftAccess.dwLowDateTime = static_cast<DWORD>(accessFileTime);
                            ftAccess.dwHighDateTime = static_cast<DWORD>(accessFileTime >> 32);
                            
                            // 转换修改时间
                            uint64_t writeFileTime = (static_cast<uint64_t>(lastModifiedTime) + FILETIME_EPOCH_DIFF) * 10000000ULL;
                            ftWrite.dwLowDateTime = static_cast<DWORD>(writeFileTime);
                            ftWrite.dwHighDateTime = static_cast<DWORD>(writeFileTime >> 32);
                            
                            // 分别设置创建时间、访问时间和修改时间
                            SetFileTime(hFile, &ftCreation, &ftAccess, &ftWrite);
                            
                            CloseHandle(hFile);
                        }
                    #else
                        // Linux/Unix平台实现：使用utimensat函数直接设置文件时间
                        struct timespec times[2];
                        
                        // 设置访问时间和修改时间
                        times[0].tv_sec = lastAccessTime;  // 访问时间(atime)
                        times[0].tv_nsec = 0;
                        times[1].tv_sec = lastModifiedTime;  // 修改时间(mtime)
                        times[1].tv_nsec = 0;
                        
                        // 使用utimensat函数设置文件时间，0表示使用当前工作目录
                        // 对于符号链接，需要使用AT_SYMLINK_NOFOLLOW标志，不跟随符号链接
                        int flags = 0;
                        if (fileMeta.fileType == 2) {
                            flags = AT_SYMLINK_NOFOLLOW;
                        }
                        
                        if (utimensat(0, outputPath.c_str(), times, flags) != 0) {
                            std::cerr << "Warning: Cannot set file times for " << outputPath 
                                      << " (" << strerror(errno) << ")" << std::endl;
                        }
                        
                        // 注意：Linux不支持直接设置文件创建时间，需要使用其他方法
                        // 这里不处理创建时间，因为Linux上没有标准API支持
                    #endif
                    
                } catch (const std::exception& e) {
                    std::cerr << "Warning: Failed to set timestamp for " << outputPath 
                              << " (" << e.what() << ")" << std::endl;
                }
            }
        }

        inFile.close();
        std::cout << "Unpacking completed successfully!" << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error during unpacking: " << e.what() << std::endl;
        return false;
    }
}

bool FilePackager::writeMetadata(const std::vector<FileMetadata>& metadata, std::ofstream& outFile) {
    try {
        // 写入元数据数量
        uint32_t metadataCount = metadata.size();
        outFile.write(reinterpret_cast<const char*>(&metadataCount), sizeof(metadataCount));

        // 写入每个文件的元数据
        for (const auto& fileMeta : metadata) {
            // 写入文件名
            uint32_t filenameLength = fileMeta.filename.size();
            outFile.write(reinterpret_cast<const char*>(&filenameLength), sizeof(filenameLength));
            outFile.write(fileMeta.filename.c_str(), filenameLength);

            // 写入文件大小、偏移量和压缩标志
            uint64_t tempSize = fileMeta.fileSize, tempOffset = fileMeta.offset;
            outFile.write(reinterpret_cast<const char*>(&tempSize), sizeof(tempSize));
            outFile.write(reinterpret_cast<const char*>(&tempOffset), sizeof(tempOffset));
            outFile.write(reinterpret_cast<const char*>(&fileMeta.isCompressed), sizeof(fileMeta.isCompressed));
            
            // 写入权限和时间戳
            outFile.write(reinterpret_cast<const char*>(&fileMeta.permissions), sizeof(fileMeta.permissions));
            outFile.write(reinterpret_cast<const char*>(&fileMeta.creationTime), sizeof(fileMeta.creationTime));
            outFile.write(reinterpret_cast<const char*>(&fileMeta.lastModifiedTime), sizeof(fileMeta.lastModifiedTime));
            outFile.write(reinterpret_cast<const char*>(&fileMeta.lastAccessTime), sizeof(fileMeta.lastAccessTime));
            
            // 写入文件类型
            outFile.write(reinterpret_cast<const char*>(&fileMeta.fileType), sizeof(fileMeta.fileType));
            
            // 写入符号链接目标（如果是符号链接）
            uint32_t symlinkTargetLength = fileMeta.symlinkTarget.size();
            outFile.write(reinterpret_cast<const char*>(&symlinkTargetLength), sizeof(symlinkTargetLength));
            if (symlinkTargetLength > 0) {
                outFile.write(fileMeta.symlinkTarget.c_str(), symlinkTargetLength);
            }
        }

        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error writing metadata: " << e.what() << std::endl;
        return false;
    }
}

bool FilePackager::readMetadata(std::ifstream& inFile, std::vector<FileMetadata>& metadata) {
    try {
        // 读取元数据数量
        uint32_t metadataCount = 0;
        inFile.read(reinterpret_cast<char*>(&metadataCount), sizeof(metadataCount));

        // 读取每个文件的元数据
        for (uint32_t i = 0; i < metadataCount; i++) {
            FileMetadata fileMeta;

            // 读取文件名
            uint32_t filenameLength = 0;
            inFile.read(reinterpret_cast<char*>(&filenameLength), sizeof(filenameLength));
            fileMeta.filename.resize(filenameLength);
            inFile.read(&fileMeta.filename[0], filenameLength);

            // 读取文件大小、偏移量和压缩标志
            uint64_t tempSize, tempOffset;
            inFile.read(reinterpret_cast<char*>(&tempSize), sizeof(tempSize));
            inFile.read(reinterpret_cast<char*>(&tempOffset), sizeof(tempOffset));
            fileMeta.fileSize = tempSize;
            fileMeta.offset = tempOffset;
            inFile.read(reinterpret_cast<char*>(&fileMeta.isCompressed), sizeof(fileMeta.isCompressed));
            
            // 读取权限和时间戳
            inFile.read(reinterpret_cast<char*>(&fileMeta.permissions), sizeof(fileMeta.permissions));
            inFile.read(reinterpret_cast<char*>(&fileMeta.creationTime), sizeof(fileMeta.creationTime));
            inFile.read(reinterpret_cast<char*>(&fileMeta.lastModifiedTime), sizeof(fileMeta.lastModifiedTime));
            inFile.read(reinterpret_cast<char*>(&fileMeta.lastAccessTime), sizeof(fileMeta.lastAccessTime));
            
            // 读取文件类型
            inFile.read(reinterpret_cast<char*>(&fileMeta.fileType), sizeof(fileMeta.fileType));
            
            // 读取符号链接目标
            uint32_t symlinkTargetLength = 0;
            inFile.read(reinterpret_cast<char*>(&symlinkTargetLength), sizeof(symlinkTargetLength));
            if (symlinkTargetLength > 0) {
                fileMeta.symlinkTarget.resize(symlinkTargetLength);
                inFile.read(&fileMeta.symlinkTarget[0], symlinkTargetLength);
            }

            metadata.push_back(fileMeta);
        }

        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error reading metadata: " << e.what() << std::endl;
        return false;
    }
}