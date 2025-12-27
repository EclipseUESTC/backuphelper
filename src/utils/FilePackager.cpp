#include "FilePackager.hpp"
#include "HuffmanCompressor.hpp"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

FilePackager::FilePackager() {
}

FilePackager::~FilePackager() {
}

bool FilePackager::packageFiles(const std::vector<std::string>& inputFiles, const std::string& outputFile) {
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
        std::vector<PackagerFileMetadata> metadata;
        uint64_t currentOffset = sizeof(metadataOffset);

        // 获取输出文件的父目录路径，用于计算相对路径
        fs::path outputDir = fs::path(outputFile).parent_path();

        for (const auto& inputFile : inputFiles) {
            std::ifstream inFile(inputFile, std::ios::binary);
            if (!inFile) {
                std::cerr << "Error: Cannot open input file: " << inputFile << std::endl;
                outFile.close();
                fs::remove(outputFile);
                return false;
            }

            // 获取文件大小
            inFile.seekg(0, std::ios::end);
            uint64_t fileSize = inFile.tellg();
            inFile.seekg(0, std::ios::beg);

            // 创建文件元数据 - 保存相对路径而仅仅是文件名
            PackagerFileMetadata fileMeta;
            // 计算相对于输出目录的路径
            fs::path filePath(inputFile);
            fileMeta.filename = fs::relative(filePath, outputDir).string();
            fileMeta.fileSize = fileSize;
            fileMeta.offset = currentOffset;
            fileMeta.isCompressed = (inputFile.size() > 5 && inputFile.substr(inputFile.size() - 5) == ".huff");

            // 写入文件内容
            char buffer[8192];
            while (inFile.read(buffer, sizeof(buffer)) || inFile.gcount() > 0) {
                outFile.write(buffer, inFile.gcount());
            }

            // 更新偏移量
            currentOffset += fileSize;
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
        std::vector<PackagerFileMetadata> metadata;
        if (!readMetadata(inFile, metadata)) {
            inFile.close();
            return false;
        }

        // 创建输出目录
        fs::create_directories(outputDir);

        // 解包每个文件
        for (const auto& fileMeta : metadata) {
            std::string outputPath = (fs::path(outputDir) / fileMeta.filename).string();
            
            // 创建文件的父目录
            fs::path parentDir = fs::path(outputPath).parent_path();
            if (!parentDir.empty()) {
                fs::create_directories(parentDir);
            }
            
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
                uint64_t bytesToRead = std::min<uint64_t>(sizeof(buffer), fileMeta.fileSize - bytesRead);
                inFile.read(buffer, bytesToRead);
                outFile.write(buffer, inFile.gcount());
                bytesRead += inFile.gcount();
            }

            outFile.close();
        }

        inFile.close();
        std::cout << "Unpacking completed successfully!" << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error during unpacking: " << e.what() << std::endl;
        return false;
    }
}

bool FilePackager::writeMetadata(const std::vector<PackagerFileMetadata>& metadata, std::ofstream& outFile) {
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
        }

        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error writing metadata: " << e.what() << std::endl;
        return false;
    }
}

bool FilePackager::readMetadata(std::ifstream& inFile, std::vector<PackagerFileMetadata>& metadata) {
    try {
        // 读取元数据数量
        uint32_t metadataCount = 0;
        inFile.read(reinterpret_cast<char*>(&metadataCount), sizeof(metadataCount));

        // 读取每个文件的元数据
        for (uint32_t i = 0; i < metadataCount; i++) {
            PackagerFileMetadata fileMeta;

            // 读取文件名
            uint32_t filenameLength = 0;
            inFile.read(reinterpret_cast<char*>(&filenameLength), sizeof(filenameLength));
            fileMeta.filename.resize(filenameLength);
            inFile.read(&fileMeta.filename[0], filenameLength);

            // 读取文件大小、偏移量和压缩标志
            uint64_t tempSize, tempOffset;
            inFile.read(reinterpret_cast<char*>(tempSize), sizeof(tempSize));
            inFile.read(reinterpret_cast<char*>(tempOffset), sizeof(tempOffset));
            fileMeta.fileSize = tempSize;
            fileMeta.offset = tempOffset;
            inFile.read(reinterpret_cast<char*>(&fileMeta.isCompressed), sizeof(fileMeta.isCompressed));

            metadata.push_back(fileMeta);
        }

        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error reading metadata: " << e.what() << std::endl;
        return false;
    }
}