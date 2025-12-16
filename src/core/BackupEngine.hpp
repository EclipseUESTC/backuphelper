#pragma once
#include <string>
#include <vector>
#include "Types.hpp"
#include "Filter.hpp"

class ILogger;

class BackupEngine {
public:
    static bool backup(const std::string& sourcePath, const std::string& backupPath, ILogger* logger, 
                      const std::vector<std::shared_ptr<Filter>>& filters = {}, bool compressEnabled = true);
    static bool restore(const std::string& backupPath, const std::string& restorePath, ILogger* logger, 
                       const std::vector<std::shared_ptr<Filter>>& filters = {}, bool compressEnabled = true);
};