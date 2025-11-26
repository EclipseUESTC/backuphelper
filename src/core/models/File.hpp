#pragma once
#include <string>
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;

class File {
private:
    fs::path filePath;
    std::string fileName;
    uint64_t fileSize;
    fs::file_type fileType;
    std::chrono::system_clock::time_point lastModifiedTime;

public:
    File();
    explicit File(const fs::path& path);
    void initialize(const fs::path& path);
    const fs::path& getFilePath() const;
    const std::string& getFileName() const;
    uint64_t getFileSize() const;
    fs::file_type getFileType() const;
    std::chrono::system_clock::time_point getLastModifiedTime() const;

    bool exists() const;
    bool isDirectory() const;
    bool isRegularFile() const;
    bool isSymbolicLink() const;
    bool isFIFO() const;
    bool isCharacterDevice() const;
    bool isBlockDevice() const;
    bool isSocket() const;

    fs::path getRelativePath(const fs::path& base) const;

    void updateTimeStamp();
    std::string toString() const;

    bool operator==(const File& other) const;
    bool operator!=(const File& other) const;
};