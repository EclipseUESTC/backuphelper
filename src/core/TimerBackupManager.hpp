#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>

// 前向声明
class ILogger;
class Filter;

// 定时备份配置
struct TimerBackupConfig {
    std::string sourceDir;          // 源目录
    std::string backupDir;          // 备份目录
    std::vector<std::shared_ptr<Filter>> filters; // 过滤条件
    bool compressEnabled;           // 是否启用压缩
    bool packageEnabled;            // 是否启用打包
    std::string packageFileName;    // 包文件名
    std::string password;           // 加密密码
    int intervalSeconds;            // 备份间隔（秒）
};

// 定时备份管理器
class TimerBackupManager {
private:
    ILogger* logger;
    TimerBackupConfig config;
    
    // 线程安全机制
    std::thread timerThread;
    std::atomic<bool> running;
    std::atomic<bool> paused;
    std::atomic<bool> interrupted; // 备份中断标志
    std::mutex mutex;
    std::condition_variable cv;
    
    // 执行备份
    bool executeBackup();
    
    // 定时器线程函数
    void timerThreadFunc();
    
public:
    TimerBackupManager(ILogger* log);
    ~TimerBackupManager();
    
    // 启动定时备份
    bool start(const TimerBackupConfig& config);
    
    // 停止定时备份
    void stop();
    
    // 暂停定时备份
    void pause();
    
    // 恢复定时备份
    void resume();
    
    // 获取当前状态
    bool isRunning() const;
    bool isPaused() const;
    
    // 更新备份间隔
    void setInterval(int seconds);
    
    // 更新备份配置
    void updateConfig(const TimerBackupConfig& newConfig);
    
    // 获取当前配置
    const TimerBackupConfig& getConfig() const;
};