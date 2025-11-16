// core/BackupEngine.cpp
#include "BackupEngine.hpp"
#include "tasks/BackupTask.hpp"
#include "tasks/RestoreTask.hpp"

bool BackupEngine::backup(const std::string& sourceDir,
                          const std::string& backupDir,
                          ILogger* logger) {
    BackupTask task(sourceDir, backupDir, logger);
    return task.execute();
}

bool BackupEngine::restore(const std::string& backupDir,
                            const std::string& restoreDir,
                            ILogger* logger) {
    RestoreTask task(backupDir, restoreDir, logger);
    return task.execute();
}