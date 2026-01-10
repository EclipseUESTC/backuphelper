// src/utils/ConsoleLogger.cpp
#include "ConsoleLogger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

static std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// 清屏函数
static void clearConsole() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void ConsoleLogger::info(const std::string& message) {
    // 检查消息是否与备份相关
    if (isBackupMessage(message)) {
        // 对于定时备份消息，每条消息都清屏并显示菜单
        if (isTimerBackupMessage(message)) {
            clearAndDisplayMenu();
        }
        // 对于实时备份消息，只有重要消息才清屏并显示菜单
        else if (isRealTimeBackupMessage(message)) {
            // 实时备份的开始和完成消息需要清屏并显示菜单
            if (message.find("starting") != std::string::npos || message.find("completed") != std::string::npos) {
                clearAndDisplayMenu();
            }
        }
    }
    std::cout << "[" << getCurrentTime() << "] [INFO] " << message << std::endl;
}

void ConsoleLogger::error(const std::string& message) {
    // 检查消息是否与备份相关
    if (isBackupMessage(message)) {
        // 对于备份错误消息，清屏并显示菜单
        clearAndDisplayMenu();
    }
    std::cerr << "[" << getCurrentTime() << "] [ERROR] " << message << std::endl;
}

void ConsoleLogger::warn(const std::string& message) {
    // 检查消息是否与备份相关
    if (isBackupMessage(message)) {
        // 对于备份警告消息，清屏并显示菜单
        clearAndDisplayMenu();
    }
    std::cout << "[" << getCurrentTime() << "] [WARN] " << message << std::endl;
}

void ConsoleLogger::debug(const std::string& message) {
    // 调试消息不进行清屏，避免频繁刷新
    std::cout << "[" << getCurrentTime() << "] [DEBUG] " << message << std::endl;
}

void ConsoleLogger::setLogLevel(LogLevel level) {
    currentLevel = level;
}

LogLevel ConsoleLogger::getLogLevel() const {
    return currentLevel;
}

void ConsoleLogger::log(LogLevel level, const std::string& message) {
    switch (level) {
        case LogLevel::DEBUG:
            std::cout << "[" << getCurrentTime() << "] [DEBUG] " << message << std::endl;
            break;
        case LogLevel::INFO:
            std::cout << "[" << getCurrentTime() << "] [INFO] " << message << std::endl;
            break;
        case LogLevel::WARNING:
            std::cout << "[" << getCurrentTime() << "] [WARN] " << message << std::endl;
            break;
        case LogLevel::ERROR_LEVEL:
            std::cerr << "[" << getCurrentTime() << "] [ERROR] " << message << std::endl;
            break;
    }
}