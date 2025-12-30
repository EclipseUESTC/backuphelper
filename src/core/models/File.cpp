#include "File.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

// 仅在Windows平台包含windows.h头文件
#ifdef _WIN32
    #include <windows.h>
#endif

File::File(): fileSize(0), fileType(fs::file_type::none) {}

File::File(const fs::path& path) {
    initialize(path);
}

void File::initialize(const fs::path& path) {
    this->filePath = path;
    this->fileName = path.filename().string();
    try {
        // 使用symlink_status获取文件状态，不解析符号链接
        fs::file_status status = fs::symlink_status(path);
        this->fileType = status.type();
        
        if (fs::is_regular_file(status)) {
            // 获取文件大小，添加错误处理
            std::error_code ec;
            this->fileSize = fs::file_size(path, ec);
            if (ec) {
                this->fileSize = 0;
            }
            
            // 使用C++17标准的文件时间获取方式，添加错误处理
            std::error_code timeEc;
            auto fileTime = fs::last_write_time(path, timeEc);
            if (!timeEc) {
                // 将文件时间转换为系统时钟时间点
                // 使用正确的转换方法：将file_time_type转换为system_clock::time_point
                auto fileClockNow = fs::file_time_type::clock::now();
                auto sysClockNow = std::chrono::system_clock::now();
                auto tp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    fileTime - fileClockNow + sysClockNow);
                this->lastModifiedTime = tp;
            }
        } else if (fs::is_directory(status)) {
            this->fileSize = 0;
            
            // 对于目录，获取其修改时间，添加错误处理
            std::error_code timeEc;
            auto fileTime = fs::last_write_time(path, timeEc);
            if (!timeEc) {
                auto fileClockNow = fs::file_time_type::clock::now();
                auto sysClockNow = std::chrono::system_clock::now();
                auto tp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    fileTime - fileClockNow + sysClockNow);
                this->lastModifiedTime = tp;
            }
        } else if (fs::is_symlink(status)) {
            // 对于符号链接，获取其大小为0
            this->fileSize = 0;
            
            // 对于符号链接，只获取链接本身的属性，不解析链接
            // 注意：某些文件系统可能不支持获取符号链接的修改时间
            // 所以如果获取失败，我们忽略并使用默认值
            std::error_code timeEc;
            // 这里不使用fs::last_write_time，因为它会解析符号链接
            // 我们直接跳过获取符号链接的修改时间，避免循环链接问题
        }
    } catch (const std::exception& e) {
        std::cerr << "Error initializing file: " << e.what() << std::endl; 
    }
}

const fs::path& File::getFilePath() const {
    return this->filePath;
}

const std::string& File::getFileName() const {
    return this->fileName;
}

uint64_t File::getFileSize() const {
    return this->fileSize;
}

fs::file_type File::getFileType() const {
    return this->fileType;
}

std::chrono::system_clock::time_point File::getLastModifiedTime() const {
    return this->lastModifiedTime;
}


bool File::exists() const {
    return fs::exists(this->filePath);
}

bool File::isDirectory() const {
    return fileType == fs::file_type::directory;
}

bool File::isRegularFile() const {
    return fileType == fs::file_type::regular;
}

bool File::isSymbolicLink() const {
    return fileType == fs::file_type::symlink;
}

bool File::isFIFO() const {
    return fileType == fs::file_type::fifo;
}

bool File::isCharacterDevice() const {
    return fileType == fs::file_type::character;
}

bool File::isBlockDevice() const {
    return fileType == fs::file_type::block;
}

bool File::isSocket() const {
    return fileType == fs::file_type::socket;
}

fs::path File::getRelativePath(const fs::path& base) const {
    try {
        // 直接获取相对于base的路径，确保返回的是符号链接本身的路径
        // 使用make_relative辅助函数，避免解析符号链接
        fs::path baseAbs = fs::canonical(base);
        fs::path fileAbs = fs::canonical(this->filePath.parent_path());
        
        // 计算父目录的相对路径
        fs::path relativeDir = fileAbs.lexically_relative(baseAbs);
        
        // 添加文件名，确保符号链接的名字被保留
        return relativeDir / this->filePath.filename();
    } catch (const std::exception& ) {
        // 如果计算相对路径失败（如循环链接），则使用文件名作为相对路径
        return this->filePath.filename();
    }
}


void File::updateTimeStamp() {
    try {
        if (fs::exists(this->filePath)) {
            initialize(this->filePath);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error updating file timestamp: " << e.what() << std::endl;
    }
}

std::string File::toString() const {
    std::stringstream ss;
    ss << "File: " << this->filePath.string() << "\n"
       << "Name: " << this->fileName << "\n"
       << "Size: " << this->fileSize << " bytes\n";

    if (this->isDirectory()) {
        ss << "Type: Directory" << std::endl;
    } else if (this->isRegularFile()) {
        ss << "Type: Regular File" << std::endl;
    } else if (this->isBlockDevice()) {
        ss << "Type: Block Device" << std::endl;
    } else if (this->isCharacterDevice()) {
        ss << "Type: Character Device" << std::endl;
    } else if (this->isFIFO()) {
        ss << "Type: FIFO Device" << std::endl;
    } else if (this->isSocket()) {
        ss << "Type: Socket File" << std::endl;
    } else {
        ss << "Type: Symbolic Link" << std::endl;
    }

    auto modTime = std::chrono::system_clock::to_time_t(this->lastModifiedTime);
    ss << "Last Modified: " << std::ctime(&modTime);
    
    return ss.str();
}

bool File::operator==(const File& other) const {
    return this->filePath == other.filePath;
}

bool File::operator!=(const File& other) const {
    return !(*this == other);
}