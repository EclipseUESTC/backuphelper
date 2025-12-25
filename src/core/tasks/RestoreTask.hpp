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
    bool packageEnabled;  // 拼接开关
    std::string packageFileName; // 拼接后的文件名
    std::string password; // 解密密码

public:
    RestoreTask(const std::string& backup, const std::string& restore, ILogger* log, 
               const std::vector<std::shared_ptr<Filter>>& filterList = {}, bool compress = true, 
               bool package = false, const std::string& pkgFileName = "backup.pkg",
               const std::string& pass = "");
    bool execute();
    TaskStatus getStatus() const;
    
};