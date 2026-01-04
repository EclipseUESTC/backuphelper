#pragma once
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <thread>

// 前向声明
struct FileChangeEvent;

// 文件系统监控器基类
class FileSystemMonitor {
public:
    using EventCallback = std::function<void(const FileChangeEvent&)>;
    
    FileSystemMonitor() = default;
    virtual ~FileSystemMonitor() = default;
    
    // 禁用拷贝和移动
    FileSystemMonitor(const FileSystemMonitor&) = delete;
    FileSystemMonitor& operator=(const FileSystemMonitor&) = delete;
    FileSystemMonitor(FileSystemMonitor&&) = delete;
    FileSystemMonitor& operator=(FileSystemMonitor&&) = delete;
    
    // 添加监控目录
    virtual bool addWatchDirectory(const std::string& directory) = 0;
    
    // 移除监控目录
    virtual bool removeWatchDirectory(const std::string& directory) = 0;
    
    // 开始监控
    virtual bool start() = 0;
    
    // 停止监控
    virtual void stop() = 0;
    
    // 设置事件回调
    void setEventCallback(EventCallback callback) {
        eventCallback = callback;
    }
    
protected:
    EventCallback eventCallback;
    std::atomic<bool> running;
    std::thread monitorThread;
};

// 工厂函数，创建平台特定的文件系统监控器
std::unique_ptr<FileSystemMonitor> createFileSystemMonitor();
