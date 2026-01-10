#include "RealTimeBackupManager.hpp"
#include "BackupEngine.hpp"
#include "../utils/FileSystemMonitor.hpp"
#include "../utils/ILogger.hpp"
#include "../utils/FileSystem.hpp"
#include <chrono>
#include <iostream>

RealTimeBackupManager::RealTimeBackupManager(ILogger* log)
    : logger(log), running(false), backupInProgress(false), lastBackupTime(0), filesChanged(false) {
    monitor = createFileSystemMonitor();
    
    // 设置事件回调
    monitor->setEventCallback([this](const FileChangeEvent& event) {
        // 将事件添加到队列
        {   std::lock_guard<std::mutex> lock(queueMutex);
            eventQueue.push(event);
        }
        queueCV.notify_one();
    });
}

RealTimeBackupManager::~RealTimeBackupManager() {
    stop();
}

bool RealTimeBackupManager::start(const RealTimeBackupConfig& config) {
    if (running) {
        return true;
    }
    
    this->config = config;
    
    // 初始化文件哈希缓存
    initializeFileHashCache();
    
    // 添加监控目录
    if (!monitor->addWatchDirectory(config.sourceDir)) {
        logger->error("Failed to add watch directory: " + config.sourceDir);
        return false;
    }
    
    // 启动监控器
    if (!monitor->start()) {
        logger->error("Failed to start file system monitor");
        return false;
    }
    
    // 启动工作线程
    running = true;
    workerThread = std::thread(&RealTimeBackupManager::workerThreadFunc, this);
    
    // 立即执行一次备份操作，确保源目录当前状态被备份
    executeBackup();
    
    logger->info("Real-time backup started for directory: " + config.sourceDir);
    return true;
}

void RealTimeBackupManager::stop() {
    if (!running) {
        return;
    }
    
    running = false;
    
    // 停止监控器
    monitor->stop();
    
    // 唤醒工作线程
    queueCV.notify_one();
    
    // 等待工作线程结束
    if (workerThread.joinable()) {
        workerThread.join();
    }
    
    logger->info("Real-time backup stopped");
}

bool RealTimeBackupManager::isRunning() const {
    return running;
}

bool RealTimeBackupManager::isBackupInProgress() const {
    return backupInProgress;
}

void RealTimeBackupManager::workerThreadFunc() {
    while (running) {
        std::unique_lock<std::mutex> lock(queueMutex);
        
        // 等待事件或超时
        auto now = std::chrono::steady_clock::now();
        auto waitResult = queueCV.wait_until(lock, now + std::chrono::seconds(1), [this]() {
            return !eventQueue.empty() || !running;
        });
        
        if (!running) {
            break;
        }
        
        if (waitResult && !eventQueue.empty()) {
            // 处理所有事件
            while (!eventQueue.empty()) {
                FileChangeEvent event = eventQueue.front();
                eventQueue.pop();
                lock.unlock();
                
                processFileChange(event);
                
                lock.lock();
            }
        }
        
        // 检查是否需要执行备份（防抖机制）
        auto currentTime = std::chrono::steady_clock::now().time_since_epoch().count() / 1000000; // 毫秒
        auto elapsedSinceLastBackup = currentTime - lastBackupTime;
        
        if (!backupInProgress && elapsedSinceLastBackup >= config.debounceTimeMs && filesChanged) {
            // 只有当文件真正变化时，才执行备份
            executeBackup();
            filesChanged = false;
        }
    }
}

void RealTimeBackupManager::processFileChange(const FileChangeEvent& event) {
    logger->debug("File change detected: " + event.filePath + ", Type: " + std::to_string(static_cast<int>(event.changeType)));
    
    // 对于创建和删除事件，直接标记为需要备份
    if (event.changeType == FileChangeType::Created || event.changeType == FileChangeType::Deleted) {
        logger->debug("File created/deleted, marking for backup: " + event.filePath);
        filesChanged = true;
        return;
    }
    
    // 对于重命名事件，直接标记为需要备份
    if (event.changeType == FileChangeType::Renamed) {
        logger->debug("File renamed, marking for backup: " + event.filePath);
        filesChanged = true;
        return;
    }
    
    // 对于修改事件，检查文件内容是否真正变化
    if (event.changeType == FileChangeType::Modified) {
        // 优化：直接标记为需要备份，不进行文件内容比较
        // 因为文件系统已经通知我们文件被修改了
        logger->debug("File modified, marking for backup: " + event.filePath);
        filesChanged = true;
        return;
        
        /*
        // 原代码：检查文件内容是否真正变化
        bool fileChanged = checkIfFileChanged(event.filePath);
        
        // 只有当文件真正变化时，才视为需要备份的信号
        if (fileChanged) {
            logger->debug("File content changed, marking for backup: " + event.filePath);
            filesChanged = true;
        } else {
            logger->debug("File content unchanged, skipping backup for: " + event.filePath);
        }
        */
    }
}

bool RealTimeBackupManager::executeBackup() {
    std::lock_guard<std::mutex> lock(backupMutex);
    
    if (backupInProgress) {
        return false;
    }
    
    backupInProgress = true;
    
    try {
        logger->info("Starting real-time backup...");
        
        // 执行备份
        bool success = BackupEngine::backup(
            config.sourceDir,
            config.backupDir,
            logger,
            config.filters,
            config.compressEnabled,
            config.packageEnabled,
            config.packageFileName,
            config.password
        );
        
        if (success) {
            logger->info("Real-time backup completed successfully");
            lastBackupTime = std::chrono::steady_clock::now().time_since_epoch().count() / 1000000; // 毫秒
            
            // 备份完成后，更新文件哈希缓存
            initializeFileHashCache();
        } else {
            logger->error("Real-time backup failed");
        }
        
        backupInProgress = false;
        return success;
    } catch (const std::exception& e) {
        logger->error("Exception during real-time backup: " + std::string(e.what()));
        backupInProgress = false;
        return false;
    }
}

void RealTimeBackupManager::initializeFileHashCache() {
    std::lock_guard<std::mutex> lock(fileHashCacheMutex);
    
    // 清空缓存
    fileHashCache.clear();
    
    // 获取所有文件
    auto files = FileSystem::getAllFiles(config.sourceDir);
    
    // 计算每个文件的哈希值并存储到缓存
    for (const auto& file : files) {
        if (file.isRegularFile()) {
            std::string filePath = file.getFilePath().string();
            std::string fileHash = FileSystem::calculateFileHash(filePath);
            if (!fileHash.empty()) {
                fileHashCache[filePath] = fileHash;
            }
        }
    }
    
    logger->debug("File hash cache initialized with " + std::to_string(fileHashCache.size()) + " files");
}

bool RealTimeBackupManager::checkIfFileChanged(const std::string& filePath) {
    // 计算当前文件的哈希值
    std::string currentHash = FileSystem::calculateFileHash(filePath);
    if (currentHash.empty()) {
        // 如果无法计算哈希值，视为文件已变化
        return true;
    }
    
    std::lock_guard<std::mutex> lock(fileHashCacheMutex);
    
    // 检查哈希值是否在缓存中
    auto it = fileHashCache.find(filePath);
    if (it == fileHashCache.end()) {
        // 新文件，视为已变化
        fileHashCache[filePath] = currentHash;
        return true;
    }
    
    // 比较当前哈希值与缓存中的值
    if (currentHash != it->second) {
        // 文件已变化，更新缓存
        fileHashCache[filePath] = currentHash;
        return true;
    }
    
    // 文件未变化
    return false;
}
