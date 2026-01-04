#include "FileSystemMonitor.hpp"
#include "../core/RealTimeBackupManager.hpp"
#include <iostream>
#include <filesystem>
#include <mutex>
#include <unordered_map>

// 跨平台头文件包含
#ifdef _WIN32
    #include <windows.h>
    #include <string>
    #include <vector>
    
    // Windows平台的文件系统监控器实现
    class WindowsFileSystemMonitor : public FileSystemMonitor {
    private:
        std::vector<HANDLE> changeHandles;
        std::unordered_map<HANDLE, std::string> handleToDirectory;
        std::mutex handlesMutex;
        
        // 监控线程函数
        void monitorThreadFunc() {
            while (running) {
                DWORD waitResult = WaitForMultipleObjects(
                    static_cast<DWORD>(changeHandles.size()),
                    changeHandles.data(),
                    FALSE,
                    1000 // 1秒超时，允许定期检查running标志
                );
                
                if (waitResult == WAIT_TIMEOUT) {
                    continue;
                }
                
                if (waitResult >= WAIT_OBJECT_0 && waitResult < WAIT_OBJECT_0 + changeHandles.size()) {
                    DWORD index = waitResult - WAIT_OBJECT_0;
                    HANDLE hChange = changeHandles[index];
                    
                    // 获取对应的目录路径
                    std::string directory;
                    {
                        std::lock_guard<std::mutex> lock(handlesMutex);
                        auto it = handleToDirectory.find(hChange);
                        if (it != handleToDirectory.end()) {
                            directory = it->second;
                        } else {
                            continue;
                        }
                    }
                    
                    // 处理目录变化
                    handleDirectoryChanges(hChange, directory);
                }
            }
        }
        
        // 处理目录变化
        void handleDirectoryChanges(HANDLE hChange, const std::string& directory) {
            char buffer[4096];
            DWORD bytesRead = 0;
            
            // 设置重叠结构，用于超时
            OVERLAPPED overlapped = {0};
            overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
            
            if (!overlapped.hEvent) {
                return;
            }
            
            try {
                while (running) {
                    // 重置事件
                    ResetEvent(overlapped.hEvent);
                    
                    // 异步调用ReadDirectoryChangesW
                    BOOL result = ReadDirectoryChangesW(
                        hChange,
                        buffer,
                        sizeof(buffer),
                        TRUE, // 监控子目录
                        FILE_NOTIFY_CHANGE_FILE_NAME |
                        FILE_NOTIFY_CHANGE_DIR_NAME |
                        FILE_NOTIFY_CHANGE_ATTRIBUTES |
                        FILE_NOTIFY_CHANGE_SIZE |
                        FILE_NOTIFY_CHANGE_LAST_WRITE |
                        FILE_NOTIFY_CHANGE_SECURITY,
                        &bytesRead,
                        &overlapped,
                        nullptr
                    );
                    
                    if (!result) {
                        DWORD error = GetLastError();
                        if (error != ERROR_IO_PENDING) {
                            // 发生错误，退出循环
                            break;
                        }
                    }
                    
                    // 等待事件或超时
                    DWORD waitResult = WaitForSingleObject(overlapped.hEvent, 500); // 500ms超时
                    if (waitResult == WAIT_TIMEOUT) {
                        continue; // 超时，继续循环，检查running标志
                    } else if (waitResult != WAIT_OBJECT_0) {
                        break; // 发生错误，退出循环
                    }
                    
                    // 获取实际读取的字节数
                    if (!GetOverlappedResult(hChange, &overlapped, &bytesRead, FALSE)) {
                        break; // 发生错误，退出循环
                    }
                    
                    if (bytesRead == 0) {
                        continue; // 没有数据，继续循环
                    }
                    
                    FILE_NOTIFY_INFORMATION* pNotifyInfo = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);
                    
                    while (pNotifyInfo != nullptr && running) {
                        // 转换文件名
                        std::wstring wFileName(pNotifyInfo->FileName, pNotifyInfo->FileNameLength / sizeof(WCHAR));
                        std::string fileName(wFileName.begin(), wFileName.end());
                        std::string filePath = directory + "/" + fileName;
                        
                        FileChangeEvent event;
                        event.filePath = filePath;
                        
                        // 确定变化类型
                        switch (pNotifyInfo->Action) {
                            case FILE_ACTION_ADDED:
                                event.changeType = FileChangeType::Created;
                                break;
                            case FILE_ACTION_MODIFIED:
                                event.changeType = FileChangeType::Modified;
                                break;
                            case FILE_ACTION_REMOVED:
                                event.changeType = FileChangeType::Deleted;
                                break;
                            case FILE_ACTION_RENAMED_NEW_NAME:
                                event.changeType = FileChangeType::Renamed;
                                break;
                            default:
                                pNotifyInfo = pNotifyInfo->NextEntryOffset ? 
                                    reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                                        reinterpret_cast<char*>(pNotifyInfo) + pNotifyInfo->NextEntryOffset
                                    ) : nullptr;
                                continue;
                        }
                        
                        // 调用回调
                        if (eventCallback) {
                            eventCallback(event);
                        }
                        
                        // 处理下一个通知
                        if (pNotifyInfo->NextEntryOffset == 0) {
                            break;
                        }
                        pNotifyInfo = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                            reinterpret_cast<char*>(pNotifyInfo) + pNotifyInfo->NextEntryOffset
                        );
                    }
                }
            } catch (...) {
                // 捕获所有异常，确保资源被释放
            }
            
            // 关闭事件句柄
            CloseHandle(overlapped.hEvent);
        }
        
    public:
        WindowsFileSystemMonitor() : FileSystemMonitor() {
            running = false;
        }
        
        ~WindowsFileSystemMonitor() override {
            stop();
        }
        
        bool addWatchDirectory(const std::string& directory) override {
            if (!std::filesystem::is_directory(directory)) {
                return false;
            }
            
            // 转换为宽字符串
            std::wstring wDirectory(directory.begin(), directory.end());
            
            // 创建文件监控句柄
            HANDLE hChange = FindFirstChangeNotificationW(
                wDirectory.c_str(),
                TRUE, // 监控子目录
                FILE_NOTIFY_CHANGE_FILE_NAME |
                FILE_NOTIFY_CHANGE_DIR_NAME |
                FILE_NOTIFY_CHANGE_ATTRIBUTES |
                FILE_NOTIFY_CHANGE_SIZE |
                FILE_NOTIFY_CHANGE_LAST_WRITE |
                FILE_NOTIFY_CHANGE_SECURITY
            );
            
            if (hChange == INVALID_HANDLE_VALUE) {
                return false;
            }
            
            // 添加到列表
            {
                std::lock_guard<std::mutex> lock(handlesMutex);
                changeHandles.push_back(hChange);
                handleToDirectory[hChange] = directory;
            }
            
            return true;
        }
        
        bool removeWatchDirectory(const std::string& directory) override {
            std::lock_guard<std::mutex> lock(handlesMutex);
            
            for (auto it = changeHandles.begin(); it != changeHandles.end(); ++it) {
                auto dirIt = handleToDirectory.find(*it);
                if (dirIt != handleToDirectory.end() && dirIt->second == directory) {
                    FindCloseChangeNotification(*it);
                    handleToDirectory.erase(dirIt);
                    changeHandles.erase(it);
                    return true;
                }
            }
            
            return false;
        }
        
        bool start() override {
            if (running) {
                return true;
            }
            
            if (changeHandles.empty()) {
                return false;
            }
            
            running = true;
            monitorThread = std::thread(&WindowsFileSystemMonitor::monitorThreadFunc, this);
            return true;
        }
        
        void stop() override {
            if (!running) {
                return;
            }
            
            running = false;
            
            if (monitorThread.joinable()) {
                monitorThread.join();
            }
            
            // 关闭所有监控句柄
            std::lock_guard<std::mutex> lock(handlesMutex);
            for (HANDLE h : changeHandles) {
                FindCloseChangeNotification(h);
            }
            changeHandles.clear();
            handleToDirectory.clear();
        }
    };
#else
    // Linux平台的文件系统监控器实现
    #include <sys/inotify.h>
    #include <unistd.h>
    #include <vector>
    #include <string>
    #include <unordered_map>
    
    class LinuxFileSystemMonitor : public FileSystemMonitor {
    private:
        int inotifyFd;
        std::unordered_map<int, std::string> wdToDirectory;
        std::mutex wdMutex;
        
        // 监控线程函数
        void monitorThreadFunc() {
            char buffer[4096];
            
            while (running) {
                // 使用select实现超时，确保定期检查running标志
                fd_set readfds;
                FD_ZERO(&readfds);
                FD_SET(inotifyFd, &readfds);
                
                struct timeval timeout;
                timeout.tv_sec = 1; // 1秒超时
                timeout.tv_usec = 0;
                
                int ret = select(inotifyFd + 1, &readfds, nullptr, nullptr, &timeout);
                if (ret < 0) {
                    if (running) {
                        std::cerr << "Error in select: " << strerror(errno) << std::endl;
                    }
                    continue;
                } else if (ret == 0) {
                    // 超时，继续循环，检查running标志
                    continue;
                }
                
                // 检查inotifyFd是否可读
                if (FD_ISSET(inotifyFd, &readfds)) {
                    int length = read(inotifyFd, buffer, sizeof(buffer));
                    if (length < 0) {
                        if (running) {
                            std::cerr << "Error reading inotify events: " << strerror(errno) << std::endl;
                        }
                        continue;
                    } else if (length == 0) {
                        // 文件描述符被关闭，退出循环
                        break;
                    }
                    
                    int i = 0;
                    while (i < length && running) {
                        inotify_event* event = reinterpret_cast<inotify_event*>(&buffer[i]);
                        
                        // 获取对应的目录路径
                        std::string directory;
                        {
                            std::lock_guard<std::mutex> lock(wdMutex);
                            auto it = wdToDirectory.find(event->wd);
                            if (it != wdToDirectory.end()) {
                                directory = it->second;
                            } else {
                                i += sizeof(inotify_event) + event->len;
                                continue;
                            }
                        }
                        
                        // 构建文件路径
                        std::string fileName(event->name);
                        std::string filePath = directory + "/" + fileName;
                        
                        FileChangeEvent fileEvent;
                        fileEvent.filePath = filePath;
                        
                        // 确定变化类型
                        if (event->mask & IN_CREATE) {
                            fileEvent.changeType = FileChangeType::Created;
                        } else if (event->mask & IN_MODIFY) {
                            fileEvent.changeType = FileChangeType::Modified;
                        } else if (event->mask & IN_DELETE) {
                            fileEvent.changeType = FileChangeType::Deleted;
                        } else if (event->mask & IN_MOVED_TO) {
                            fileEvent.changeType = FileChangeType::Renamed;
                        } else {
                            i += sizeof(inotify_event) + event->len;
                            continue;
                        }
                        
                        // 调用回调
                        if (eventCallback) {
                            eventCallback(fileEvent);
                        }
                        
                        i += sizeof(inotify_event) + event->len;
                    }
                }
            }
        }
        
    public:
        LinuxFileSystemMonitor() : FileSystemMonitor() {
            running = false;
            inotifyFd = inotify_init();
            if (inotifyFd < 0) {
                throw std::runtime_error("Failed to initialize inotify");
            }
        }
        
        ~LinuxFileSystemMonitor() override {
            stop();
            if (inotifyFd >= 0) {
                close(inotifyFd);
            }
        }
        
        bool addWatchDirectory(const std::string& directory) override {
            if (!std::filesystem::is_directory(directory)) {
                return false;
            }
            
            int wd = inotify_add_watch(
                inotifyFd,
                directory.c_str(),
                IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_TO | IN_MOVED_FROM | IN_DELETE_SELF
            );
            
            if (wd < 0) {
                return false;
            }
            
            {
                std::lock_guard<std::mutex> lock(wdMutex);
                wdToDirectory[wd] = directory;
            }
            
            return true;
        }
        
        bool removeWatchDirectory(const std::string& directory) override {
            std::lock_guard<std::mutex> lock(wdMutex);
            
            for (auto it = wdToDirectory.begin(); it != wdToDirectory.end(); ++it) {
                if (it->second == directory) {
                    inotify_rm_watch(inotifyFd, it->first);
                    wdToDirectory.erase(it);
                    return true;
                }
            }
            
            return false;
        }
        
        bool start() override {
            if (running) {
                return true;
            }
            
            if (wdToDirectory.empty()) {
                return false;
            }
            
            running = true;
            monitorThread = std::thread(&LinuxFileSystemMonitor::monitorThreadFunc, this);
            return true;
        }
        
        void stop() override {
            if (!running) {
                return;
            }
            
            running = false;
            
            if (monitorThread.joinable()) {
                // 向inotify文件描述符写入数据，唤醒监控线程
                write(inotifyFd, "", 0);
                monitorThread.join();
            }
            
            // 移除所有监控
            {
                std::lock_guard<std::mutex> lock(wdMutex);
                for (auto& pair : wdToDirectory) {
                    inotify_rm_watch(inotifyFd, pair.first);
                }
                wdToDirectory.clear();
            }
        }
    };
#endif

// 工厂函数实现
std::unique_ptr<FileSystemMonitor> createFileSystemMonitor() {
    #ifdef _WIN32
        return std::make_unique<WindowsFileSystemMonitor>();
    #else
        return std::make_unique<LinuxFileSystemMonitor>();
    #endif
}
