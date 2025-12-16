// core/BackupEngine.cpp
#include "BackupEngine.hpp"
#include "tasks/BackupTask.hpp"
#include "tasks/RestoreTask.hpp"

bool BackupEngine::backup(const std::string& sourceDir,
                          const std::string& backupDir,
                          ILogger* logger, 
                          const std::vector<std::shared_ptr<Filter>>& filters,
                          bool compressEnabled) {
    BackupTask task(sourceDir, backupDir, logger, filters, compressEnabled);
    return task.execute();
}

bool BackupEngine::restore(const std::string& backupDir,
                            const std::string& restoreDir,
                            ILogger* logger, 
                            const std::vector<std::shared_ptr<Filter>>& filters,
                            bool compressEnabled) {
    RestoreTask task(backupDir, restoreDir, logger, filters, compressEnabled);
    return task.execute();
}