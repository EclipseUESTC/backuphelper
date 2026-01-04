// core/BackupEngine.cpp
#include "BackupEngine.hpp"
#include "tasks/BackupTask.hpp"
#include "tasks/RestoreTask.hpp"

bool BackupEngine::backup(const std::string& sourceDir,
                          const std::string& backupPath, ILogger* logger,
                          const std::vector<std::shared_ptr<Filter>>& filters,
                          bool compressEnabled,
                          bool packageEnabled,
                          const std::string& packageFileName,
                          const std::string& password,
                          std::atomic<bool>* interrupted) {
    BackupTask task(sourceDir, backupPath, logger, filters, compressEnabled, packageEnabled, packageFileName, password, interrupted);
    return task.execute();
}

bool BackupEngine::restore(const std::string& backupPath,
                            const std::string& restoreDir, ILogger* logger,
                            const std::vector<std::shared_ptr<Filter>>& filters,
                            bool compressEnabled,
                            bool packageEnabled,
                            const std::string& packageFileName,
                            const std::string& password,
                            std::atomic<bool>* interrupted) {
    RestoreTask task(backupPath, restoreDir, logger, filters, compressEnabled, packageEnabled, packageFileName, password, interrupted);
    return task.execute();
}