#include "TimerBackupManager.hpp"
#include "BackupEngine.hpp"
#include "utils/ILogger.hpp"
#include "utils/FileSystem.hpp"
#include <iostream>

TimerBackupManager::TimerBackupManager(ILogger* log) 
    : logger(log), running(false), paused(false), interrupted(false) {
}

TimerBackupManager::~TimerBackupManager() {
    stop();
}

bool TimerBackupManager::start(const TimerBackupConfig& config) {
    if (running) {
        logger->error("Timer backup is already running.");
        return false;
    }
    
    // 检查源目录是否存在
    if (!FileSystem::exists(config.sourceDir)) {
        logger->error("Source directory not found: " + config.sourceDir);
        logger->error("Timer backup cannot start without a valid source directory.");
        return false;
    }
    
    this->config = config;
    running = true;
    paused = false;
    interrupted = false;
    
    // 启动定时器线程
    timerThread = std::thread(&TimerBackupManager::timerThreadFunc, this);
    
    // 使用cout直接输出提示信息，确保用户能够看到
    std::cout << "[Info] Timer backup started with interval: " << config.intervalSeconds << " seconds" << std::endl;
    std::cout << "[Info] Use menu option 6 to stop timer backup." << std::endl;
    
    // 同时记录到日志
    logger->info("Timer backup started with interval: " + std::to_string(config.intervalSeconds) + " seconds");
    logger->info("Use menu option 6 to stop timer backup.");
    return true;
}

void TimerBackupManager::stop() {
    if (running) {
        logger->info("Stopping timer backup...");
        
        // 首先设置interrupted为true，中断正在进行的备份
        interrupted = true;
        // 然后设置running为false，确保不再执行新的备份
        running = false;
        
        // 唤醒等待的线程
        cv.notify_one();
        
        // 等待定时器线程结束
        if (timerThread.joinable()) {
            timerThread.join();
        }
        
        logger->info("Timer backup stopped.");
    }
}

void TimerBackupManager::pause() {
    if (running && !paused) {
        paused = true;
        logger->info("Timer backup paused.");
    }
}

void TimerBackupManager::resume() {
    if (running && paused) {
        paused = false;
        cv.notify_one();
        logger->info("Timer backup resumed.");
    }
}

bool TimerBackupManager::isRunning() const {
    return running;
}

bool TimerBackupManager::isPaused() const {
    return paused;
}

void TimerBackupManager::setInterval(int seconds) {
    if (seconds > 0) {
        std::lock_guard<std::mutex> lock(mutex);
        config.intervalSeconds = seconds;
        logger->info("Timer backup interval updated to: " + std::to_string(seconds) + " seconds");
    }
}

void TimerBackupManager::updateConfig(const TimerBackupConfig& newConfig) {
    // 更新配置
    std::lock_guard<std::mutex> lock(mutex);
    config = newConfig;
    logger->info("Timer backup configuration updated.");
}

const TimerBackupConfig& TimerBackupManager::getConfig() const {
    // 返回配置的常量引用，外部只能读取，不能修改
    return config;
}

bool TimerBackupManager::executeBackup() {
    logger->debug("Timer backup triggered.");
    
    try {
        // 检查是否已被停止或中断
        if (!running || interrupted) {
            return false;
        }
        
        // 复制配置到局部变量，避免在备份过程中配置被修改
        TimerBackupConfig localConfig;
        {
            std::lock_guard<std::mutex> lock(mutex);
            localConfig = config;
        }
        
        // 检查源目录是否存在
        if (!FileSystem::exists(localConfig.sourceDir)) {
            logger->warn("Source directory not found: " + localConfig.sourceDir);
            logger->warn("Skipping backup, will try again after interval.");
            return false;
        }
        
        // 再次检查是否已被停止或中断
        if (!running || interrupted) {
            return false;
        }
        
        // 调用BackupEngine执行备份
        bool success = BackupEngine::backup(
            localConfig.sourceDir,
            localConfig.backupDir,
            logger,
            localConfig.filters,
            localConfig.compressEnabled,
            localConfig.packageEnabled,
            localConfig.packageFileName,
            localConfig.password,
            &interrupted
        );
        
        if (success) {
            logger->info("Timer backup completed successfully.");
        } else {
            logger->error("Timer backup failed.");
        }
        
        return success;
    } catch (const std::exception& e) {
        logger->error("Exception during timer backup: " + std::string(e.what()));
        return false;
    }
}

void TimerBackupManager::timerThreadFunc() {
    logger->debug("Timer backup thread started.");
    
    while (running) {
        // 检查是否已被中断
        if (interrupted) {
            break;
        }
        
        // 执行一次备份
        if (!paused && running && !interrupted) {
            executeBackup();
        }
        
        // 无论备份成功还是失败，都等待指定间隔，或者直到running变为false或被中断
        std::unique_lock<std::mutex> lock(mutex);
        // 获取当前间隔时间
        int intervalSeconds = config.intervalSeconds;
        // 等待指定间隔，或者直到running变为false或被中断
        cv.wait_for(lock, std::chrono::seconds(intervalSeconds), [this]() {
            return !running || interrupted;
        });
    }
    
    // 重置中断标志
    interrupted = false;
    logger->debug("Timer backup thread exiting.");
}