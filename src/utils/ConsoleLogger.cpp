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

void ConsoleLogger::info(const std::string& message) {
    std::cout << "[" << getCurrentTime() << "] [INFO] " << message << std::endl;
}

void ConsoleLogger::error(const std::string& message) {
    std::cerr << "[" << getCurrentTime() << "] [ERROR] " << message << std::endl;
}

void ConsoleLogger::warn(const std::string& message) {
    std::cout << "[" << getCurrentTime() << "] [WARN] " << message << std::endl;
}

void ConsoleLogger::debug(const std::string& message) {
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