#pragma once
#include <string>
#include <vector>
#include <memory>
#include "../Types.hpp"
#include "../Filter.hpp"
#include "../../utils/ILogger.hpp"

class FileSystem; // 前向声明

class RestoreTask {
private:
    std::string backupPath;
    std::string restorePath;
    TaskStatus status;
    ILogger* logger;
    std::vector<std::shared_ptr<Filter>> filters;
    bool compressEnabled; // 压缩开关

public:
    RestoreTask(const std::string& backup, const std::string& restore, ILogger* log, 
               const std::vector<std::shared_ptr<Filter>>& filterList = {}, bool compress = true);
    bool execute();
    TaskStatus getStatus() const;
    
};