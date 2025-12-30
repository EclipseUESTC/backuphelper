#pragma once
#include <string>
#include <filesystem>
#include <chrono>
#include <vector>

namespace fs = std::filesystem;

class File {
private:
    // 1. 文件类型
    fs::file_type fileType;
    
    // 2. 文件路径
    fs::path filePath;
    std::string fileName;
    
    // 3. 文件数据
    std::vector<char> fileData; // 存储文件内容，仅在需要时加载
    bool dataLoaded; // 标记文件数据是否已加载
    
    // 4. 文件元数据（属主/权限/时间）
    uint64_t fileSize;
    std::chrono::system_clock::time_point creationTime;
    std::chrono::system_clock::time_point lastModifiedTime;
    std::chrono::system_clock::time_point lastAccessTime;
    
    // 文件权限（跨平台）
    unsigned int permissions; // 存储文件权限，使用mode_t的跨平台表示
    
    // 文件属主信息
    unsigned int ownerId; // UID
    unsigned int groupId; // GID
    
    // 5. 文件链接方式（软/硬）
    fs::path symlinkTarget; // 符号链接目标路径
    bool isHardLink; // 是否是硬链接
    unsigned int hardLinkCount; // 硬链接数量

public:
    File();
    explicit File(const fs::path& path);
    void initialize(const fs::path& path);
    
    // 基本属性访问
    const fs::path& getFilePath() const;
    const std::string& getFileName() const;
    uint64_t getFileSize() const;
    fs::file_type getFileType() const;
    
    // 时间戳访问
    std::chrono::system_clock::time_point getCreationTime() const;
    std::chrono::system_clock::time_point getLastModifiedTime() const;
    std::chrono::system_clock::time_point getLastAccessTime() const;
    
    // 权限和属主访问
    unsigned int getPermissions() const;
    unsigned int getOwnerId() const;
    unsigned int getGroupId() const;
    
    // 链接信息访问
    const fs::path& getSymlinkTarget() const;
    bool getIsHardLink() const;
    unsigned int getHardLinkCount() const;
    
    // 文件数据操作
    const std::vector<char>& getFileData() const;
    void setFileData(const std::vector<char>& data);
    bool loadFileData(); // 从文件加载数据
    bool saveFileData(); // 将数据保存到文件
    
    // 文件类型判断
    bool exists() const;
    bool isDirectory() const;
    bool isRegularFile() const;
    bool isSymbolicLink() const;
    bool isFIFO() const;
    bool isCharacterDevice() const;
    bool isBlockDevice() const;
    bool isSocket() const;

    // 路径操作
    fs::path getRelativePath(const fs::path& base) const;

    // 更新操作
    void updateTimeStamp();
    
    // 字符串表示
    std::string toString() const;

    // 比较操作
    bool operator==(const File& other) const;
    bool operator!=(const File& other) const;
};