#pragma once
#include <string>
#include <vector>
#include <memory>
#include "../Types.hpp"
#include "../Filter.hpp"
#include "../../utils/ILogger.hpp"

class FileSystem; // 前向声明

class BackupTask {
private:
    std::string sourcePath;
    std::string backupPath;
    TaskStatus status;
    ILogger* logger;
    std::vector<std::shared_ptr<Filter>> filters;
    bool compressEnabled; // 压缩开关

public:
    BackupTask(const std::string& source, const std::string& backup, ILogger* log, 
              const std::vector<std::shared_ptr<Filter>>& filterList = {}, bool compress = true);
    bool execute();
    TaskStatus getStatus() const;

};