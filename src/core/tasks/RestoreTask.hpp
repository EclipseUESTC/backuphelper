#pragma once
#include <string>
#include <Types.hpp>
#include "../utils/ILogger.hpp"

class FileSystem; // 前向声明

class RestoreTask {
private:
    std::string backupPath;
    std::string restorePath;
    TaskStatus status;
    ILogger* logger;

public:
    RestoreTask(const std::string& backup, const std::string& restore, ILogger* log);
    bool execute();
    TaskStatus getStatus() const;
    
};