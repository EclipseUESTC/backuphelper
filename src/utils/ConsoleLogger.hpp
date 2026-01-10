#pragma once
#include "ILogger.hpp"
#include <string>
#include <functional>

class ConsoleLogger : public ILogger {
private:
    LogLevel currentLevel = LogLevel::INFO;
    
    // 定义菜单显示回调函数类型
    using MenuDisplayCallback = std::function<void()>;
    MenuDisplayCallback menuDisplayCallback;
    
    // 检查消息是否与实时备份或定时备份相关
    bool isBackupMessage(const std::string& message) const {
        return message.find("backup") != std::string::npos;
    }
    
    // 检查消息是否与实时备份相关
    bool isRealTimeBackupMessage(const std::string& message) const {
        return message.find("real-time") != std::string::npos || message.find("Real-time") != std::string::npos;
    }
    
    // 检查消息是否与定时备份相关
    bool isTimerBackupMessage(const std::string& message) const {
        return message.find("timer") != std::string::npos || message.find("Timer") != std::string::npos;
    }
    
    // 清屏并显示菜单
    void clearAndDisplayMenu() const {
        // 跨平台清屏命令
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
        
        // 如果有菜单显示回调，调用它
        if (menuDisplayCallback) {
            menuDisplayCallback();
        }
    }
    
public:
    void info(const std::string& message) override;

    void error(const std::string& message) override;

    void warn(const std::string& message) override;
    
    void debug(const std::string& message) override;
    
    void setLogLevel(LogLevel level) override;
    
    LogLevel getLogLevel() const override;
    
    void log(LogLevel level, const std::string& message) override;
    
    // 设置菜单显示回调函数
    void setMenuDisplayCallback(MenuDisplayCallback callback) {
        menuDisplayCallback = callback;
    }

};