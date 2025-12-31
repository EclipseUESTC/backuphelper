#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>

// 引入File类
#include "../core/models/File.hpp"

// 文件元数据结构
struct FileMetadata {
    std::string filename;      // 文件名
    uint64_t fileSize;         // 文件大小
    uint64_t offset;           // 在打包文件中的偏移量
    bool isCompressed;         // 是否压缩
    uint32_t permissions;      // 文件权限
    uint64_t creationTime;     // 创建时间（时间戳）
    uint64_t lastModifiedTime; // 最后修改时间（时间戳）
    uint64_t lastAccessTime;   // 最后访问时间（时间戳）
    uint16_t fileType;         // 文件类型（0: 普通文件, 1: 目录, 2: 符号链接, 3: FIFO, 4: 字符设备, 5: 块设备, 6: 套接字）
    std::string symlinkTarget; // 符号链接目标

    FileMetadata():
        filename(""), fileSize(0), offset(0), isCompressed(false),
        permissions(0), creationTime(0), lastModifiedTime(0), lastAccessTime(0),
        fileType(0), symlinkTarget("") {}
    
    // 从File对象创建FileMetadata
    FileMetadata(const File& file, const std::filesystem::path& basePath);
};

class FilePackager {
public:
    FilePackager();
    ~FilePackager();

    // 打包文件集合到单个文件
    bool packageFiles(const std::vector<File>& inputFiles, const std::string& outputFile, const std::string& basePath = "");
    
    // 兼容旧接口，内部转换为File对象
    bool packageFiles(const std::vector<std::string>& inputFiles, const std::string& outputFile);

    // 解包单个文件到目录
    bool unpackFiles(const std::string& inputFile, const std::string& outputDir);
    
    // 解包单个文件并返回File对象列表
    std::vector<File> unpackFilesToFiles(const std::string& inputFile, const std::string& outputDir);

private:
    // 写入元数据到文件
    bool writeMetadata(const std::vector<FileMetadata>& metadata, std::ofstream& outFile);

    // 从文件读取元数据
    bool readMetadata(std::ifstream& inFile, std::vector<FileMetadata>& metadata);
    
    // 将FileMetadata转换为File对象
    File metadataToFile(const FileMetadata& metadata, const std::string& outputDir) const;
};