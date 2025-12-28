#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>

// 文件元数据结构
struct FileMetadata {
    std::string filename;      // 文件名
    uint64_t fileSize;         // 文件大小
    uint64_t offset;           // 在打包文件中的偏移量
    bool isCompressed;         // 是否压缩

    FileMetadata():
        filename(""), fileSize(0), offset(0), isCompressed(false) {}
};

class FilePackager {
public:
    FilePackager();
    ~FilePackager();

    // 打包文件集合到单个文件
    bool packageFiles(const std::vector<std::string>& inputFiles, const std::string& outputFile);

    // 解包单个文件到目录
    bool unpackFiles(const std::string& inputFile, const std::string& outputDir);

private:
    // 写入元数据到文件
    bool writeMetadata(const std::vector<FileMetadata>& metadata, std::ofstream& outFile);

    // 从文件读取元数据
    bool readMetadata(std::ifstream& inFile, std::vector<FileMetadata>& metadata);
};