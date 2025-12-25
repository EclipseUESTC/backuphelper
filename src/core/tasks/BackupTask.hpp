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
    
    // 删除空目录的辅助方法
    void removeEmptyDirectories(const std::string& path);
    
    // 当前任务状态
    TaskStatus status;
    // 日志记录器
    ILogger* logger;
    // 过滤器列表
    std::vector<std::shared_ptr<Filter>> filters;
    // 压缩开关
    bool compressEnabled;
    // 拼接开关
    bool packageEnabled;
    // 包文件名
    std::string packageFileName;
    // 加密密码
    std::string password;

public:
    BackupTask(const std::string& source, const std::string& backup, ILogger* log, 
              const std::vector<std::shared_ptr<Filter>>& filterList = {}, bool compress = true, 
              bool package = false, const std::string& pkgFileName = "backup.pkg",
              const std::string& pass = "");
    bool execute();
    TaskStatus getStatus() const;

};