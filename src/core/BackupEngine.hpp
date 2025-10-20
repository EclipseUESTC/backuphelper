#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "../utils/FileSystem.hpp"
#include "../utils/Logger.hpp"

class BackupEngine {
public:
    BackupEngine(const std::string& source, const std::string& backup);
    ~BackupEngine() = default;

    bool performBackup();
    bool performRestore(const std::string& restorePath = "");

    // 禁止拷贝
    BackupEngine(const BackupEngine&) = delete;
    BackupEngine& operator=(const BackupEngine&) = delete;

private:
    std::string sourcePath;
    std::string backupPath;

    std::string getBackupFilePath(const std::string& sourceFile);
};