#pragma once
#include <string>
#include "../Types.hpp"
#include "../../utils/ILogger.hpp"

class FileSystem; // 前向声明

class BackupTask {
private:
    std::string sourcePath;
    std::string backupPath;
    TaskStatus status;
    ILogger* logger;


public:
    BackupTask(const std::string& source, const std::string& backup, ILogger* log);
    bool execute();
    TaskStatus getStatus() const;

};