#include "File.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <sys/stat.h>
#include <sys/types.h>

// 仅在Windows平台包含windows.h头文件
#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
#endif

File::File(): 
    fileType(fs::file_type::none),
    fileSize(0),
    dataLoaded(false),
    ownerId(0),
    groupId(0),
    isHardLink(false),
    hardLinkCount(1) {}

File::File(const fs::path& path) {
    initialize(path);
}

void File::initialize(const fs::path& path) {
    this->filePath = path;
    this->fileName = path.filename().string();
    this->dataLoaded = false;
    
    try {
        // 使用symlink_status获取文件状态，不解析符号链接
        std::error_code ec;
        fs::file_status status = fs::symlink_status(path, ec);
        if (ec) {
            this->fileType = fs::file_type::none;
            // 确保元数据始终被初始化
            this->fileSize = 0;
            this->hardLinkCount = 1;
            this->permissions = 0644; // 默认权限
            this->ownerId = 0;
            this->groupId = 0;
            this->isHardLink = false;
            this->symlinkTarget.clear();
            // 使用当前时间作为默认时间戳
            auto now = std::chrono::system_clock::now();
            this->creationTime = now;
            this->lastModifiedTime = now;
            this->lastAccessTime = now;
            return;
        }
        this->fileType = status.type();
        
        // 获取文件大小
        if (fs::is_regular_file(status)) {
            this->fileSize = fs::file_size(path, ec);
            if (ec) {
                this->fileSize = 0;
            }
        } else {
            this->fileSize = 0;
        }
        
        // 获取硬链接数量和权限信息
        this->hardLinkCount = 1;
        this->permissions = 0644; // 默认权限
        this->ownerId = 0;
        this->groupId = 0;
        this->isHardLink = false;
        
        #ifdef _WIN32
            // Windows平台实现 - 使用后面定义的fileInfo变量
        #else
            struct stat st;
            if (stat(path.string().c_str(), &st) == 0) {
                this->hardLinkCount = st.st_nlink;
                this->isHardLink = (this->hardLinkCount > 1);
                this->permissions = st.st_mode & 07777; // 获取权限
                this->ownerId = st.st_uid;
                this->groupId = st.st_gid;
            }
        #endif
        
        // 获取符号链接目标
        this->symlinkTarget.clear();
        if (fs::is_symlink(status)) {
            this->symlinkTarget = fs::read_symlink(path, ec);
            if (ec) {
                this->symlinkTarget.clear();
            }
        }
        
        // 获取文件时间戳，确保始终被初始化
        auto now = std::chrono::system_clock::now();
        this->creationTime = now;
        this->lastModifiedTime = now;
        this->lastAccessTime = now;
        
        // 统一使用平台特定的API获取文件时间，确保符号链接的时间戳也能正确获取
        #ifdef _WIN32
            // Windows平台：使用Windows API获取文件时间
            WIN32_FILE_ATTRIBUTE_DATA fileInfo;
            if (GetFileAttributesExA(path.string().c_str(), GetFileExInfoStandard, &fileInfo)) {
                SYSTEMTIME st;
                tm tm_local;
                
                // 最后修改时间
                if (FileTimeToSystemTime(&fileInfo.ftLastWriteTime, &st)) {
                    tm_local.tm_year = st.wYear - 1900;
                    tm_local.tm_mon = st.wMonth - 1;
                    tm_local.tm_mday = st.wDay;
                    tm_local.tm_hour = st.wHour;
                    tm_local.tm_min = st.wMinute;
                    tm_local.tm_sec = st.wSecond;
                    tm_local.tm_isdst = 0;
                    
                    time_t modTime = mktime(&tm_local);
                    if (modTime != -1) {
                        this->lastModifiedTime = std::chrono::system_clock::from_time_t(modTime);
                    }
                }
                
                // 创建时间
                if (FileTimeToSystemTime(&fileInfo.ftCreationTime, &st)) {
                    struct tm tm;
                    tm.tm_year = st.wYear - 1900;
                    tm.tm_mon = st.wMonth - 1;
                    tm.tm_mday = st.wDay;
                    tm.tm_hour = st.wHour;
                    tm.tm_min = st.wMinute;
                    tm.tm_sec = st.wSecond;
                    tm.tm_isdst = 0;
                    time_t createTime = mktime(&tm);
                    if (createTime != -1) {
                        this->creationTime = std::chrono::system_clock::from_time_t(createTime);
                    }
                }
                
                // 访问时间
                if (FileTimeToSystemTime(&fileInfo.ftLastAccessTime, &st)) {
                    struct tm tm;
                    tm.tm_year = st.wYear - 1900;
                    tm.tm_mon = st.wMonth - 1;
                    tm.tm_mday = st.wDay;
                    tm.tm_hour = st.wHour;
                    tm.tm_min = st.wMinute;
                    tm.tm_sec = st.wSecond;
                    tm.tm_isdst = 0;
                    time_t accessTime = mktime(&tm);
                    if (accessTime != -1) {
                        this->lastAccessTime = std::chrono::system_clock::from_time_t(accessTime);
                    }
                }
            }
        #else
            // Linux/Unix平台：使用stat获取时间，对于符号链接使用lstat
            struct stat st;
            int statResult;
            if (fs::is_symlink(status)) {
                // 对于符号链接，使用lstat获取符号链接本身的元数据
                statResult = lstat(path.string().c_str(), &st);
            } else {
                // 对于普通文件和目录，使用stat获取元数据
                statResult = stat(path.string().c_str(), &st);
            }
            
            if (statResult == 0) {
                // 最后修改时间
                this->lastModifiedTime = std::chrono::system_clock::from_time_t(st.st_mtime);
                // 创建时间（Linux下ctime实际上是状态改变时间）
                this->creationTime = std::chrono::system_clock::from_time_t(st.st_ctime);
                // 最后访问时间
                this->lastAccessTime = std::chrono::system_clock::from_time_t(st.st_atime);
            }
        #endif
        
    } catch (const std::exception& e) {
        std::cerr << "Error initializing file: " << e.what() << std::endl; 
        // 确保元数据始终被初始化
        this->fileType = fs::file_type::none;
        this->fileSize = 0;
        this->hardLinkCount = 1;
        this->permissions = 0644; // 默认权限
        this->ownerId = 0;
        this->groupId = 0;
        this->isHardLink = false;
        this->symlinkTarget.clear();
        // 使用当前时间作为默认时间戳
        auto now = std::chrono::system_clock::now();
        this->creationTime = now;
        this->lastModifiedTime = now;
        this->lastAccessTime = now;
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

std::chrono::system_clock::time_point File::getCreationTime() const {
    return this->creationTime;
}

std::chrono::system_clock::time_point File::getLastModifiedTime() const {
    return this->lastModifiedTime;
}

std::chrono::system_clock::time_point File::getLastAccessTime() const {
    return this->lastAccessTime;
}

unsigned int File::getPermissions() const {
    return this->permissions;
}

unsigned int File::getOwnerId() const {
    return this->ownerId;
}

unsigned int File::getGroupId() const {
    return this->groupId;
}

const fs::path& File::getSymlinkTarget() const {
    return this->symlinkTarget;
}

bool File::getIsHardLink() const {
    return this->isHardLink;
}

unsigned int File::getHardLinkCount() const {
    return this->hardLinkCount;
}

const std::vector<char>& File::getFileData() const {
    return this->fileData;
}

void File::setFileData(const std::vector<char>& data) {
    this->fileData = data;
    this->dataLoaded = true;
    this->fileSize = data.size();
}

bool File::loadFileData() {
    if (this->isSymbolicLink() || this->isDirectory() || !this->isRegularFile()) {
        return false; // 只加载常规文件的数据
    }
    
    try {
        std::ifstream file(this->filePath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        // 获取文件大小
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        // 分配内存并读取数据
        this->fileData.resize(size);
        file.read(this->fileData.data(), size);
        
        if (!file) {
            return false;
        }
        
        this->dataLoaded = true;
        this->fileSize = size;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading file data: " << e.what() << std::endl;
        return false;
    }
}

bool File::saveFileData() {
    if (this->isSymbolicLink() || this->isDirectory() || !this->isRegularFile()) {
        return false; // 只保存常规文件的数据
    }
    
    try {
        std::ofstream file(this->filePath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        file.write(this->fileData.data(), this->fileData.size());
        if (!file) {
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving file data: " << e.what() << std::endl;
        return false;
    }
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