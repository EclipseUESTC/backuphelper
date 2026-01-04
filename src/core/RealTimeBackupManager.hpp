#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

// 前向声明
class FileSystemMonitor;
class ILogger;
class Filter;

// 实时备份配置
struct RealTimeBackupConfig {
    std::string sourceDir;          // 源目录
    std::string backupDir;          // 备份目录
    std::vector<std::shared_ptr<Filter>> filters; // 过滤条件
    bool compressEnabled;           // 是否启用压缩
    bool packageEnabled;            // 是否启用打包
    std::string packageFileName;    // 包文件名
    std::string password;           // 加密密码
    int debounceTimeMs;             // 防抖时间（毫秒），避免频繁备份
};

// 文件变化事件类型
enum class FileChangeType {
    Created,
    Modified,
    Deleted,
    Renamed
};

// 文件变化事件
struct FileChangeEvent {
    std::string filePath;
    FileChangeType changeType;
    std::string oldFilePath; // 用于重命名事件
};

// 实时备份管理器
class RealTimeBackupManager {
private:
    std::unique_ptr<FileSystemMonitor> monitor;
    ILogger* logger;
    RealTimeBackupConfig config;
    
    // 事件队列和线程安全机制
    std::queue<FileChangeEvent> eventQueue;
    std::mutex queueMutex;
    std::condition_variable queueCV;
    
    // 工作线程
    std::thread workerThread;
    std::atomic<bool> running;
    
    // 备份任务控制
    std::atomic<bool> backupInProgress;
    std::mutex backupMutex;
    
    // 防抖机制
    std::atomic<long long> lastBackupTime;
    
    // 处理文件变化事件
    void processFileChange(const FileChangeEvent& event);
    
    // 工作线程函数
    void workerThreadFunc();
    
    // 执行备份
    bool executeBackup();
    
public:
    RealTimeBackupManager(ILogger* log);
    ~RealTimeBackupManager();
    
    // 启动实时备份
    bool start(const RealTimeBackupConfig& config);
    
    // 停止实时备份
    void stop();
    
    // 获取当前状态
    bool isRunning() const;
    bool isBackupInProgress() const;
};