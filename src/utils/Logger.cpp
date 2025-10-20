// src/utils/Logger.cpp
#include "Logger.hpp"
#include <iostream>

void Logger::log(const std::string& level, const std::string& message) {
    std::cout << "[" << level << "] " << message << std::endl;
}

void Logger::info(const std::string& message) {
    log("INFO", message);
}

void Logger::error(const std::string& message) {
    std::cerr << "[ERROR] " << message << std::endl;
}

void Logger::debug(const std::string& message) {
#ifdef DEBUG
    log("DEBUG", message);
#endif
}