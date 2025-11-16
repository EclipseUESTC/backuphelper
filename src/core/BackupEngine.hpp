#pragma once
#include <string>
#include "Types.hpp"

class ILogger;

class BackupEngine {
public:
    static bool backup(const std::string& sourcePath, const std::string& backupPath, ILogger* logger);
    static bool restore(const std::string& backupPath, const std::string& restorePath, ILogger* logger);
};