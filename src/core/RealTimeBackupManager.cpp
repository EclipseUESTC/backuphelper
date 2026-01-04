#include "RealTimeBackupManager.hpp"
#include "BackupEngine.hpp"
#include "../utils/FileSystemMonitor.hpp"
#include "../utils/ILogger.hpp"
#include <chrono>
#include <iostream>

RealTimeBackupManager::RealTimeBackupManager(ILogger* log)
    : logger(log), running(false), backupInProgress(false), lastBackupTime(0) {
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
        
        if (!backupInProgress && elapsedSinceLastBackup >= config.debounceTimeMs) {
            executeBackup();
        }
    }
}

void RealTimeBackupManager::processFileChange(const FileChangeEvent& event) {
    logger->debug("File change detected: " + event.filePath + ", Type: " + std::to_string(static_cast<int>(event.changeType)));
    
    // 这里可以添加额外的事件处理逻辑，比如过滤不需要备份的文件
    // 目前我们简单地将所有事件视为需要备份的信号
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
