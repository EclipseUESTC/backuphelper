#pragma once
#include "ILogger.hpp"
#include <string>

class ConsoleLogger : public ILogger {
private:
    LogLevel currentLevel = LogLevel::INFO;
public:
    void info(const std::string& message) override;

    void error(const std::string& message) override;

    void warn(const std::string& message) override;
    
    void debug(const std::string& message) override;
    
    void setLogLevel(LogLevel level) override;
    
    LogLevel getLogLevel() const override;
    
    void log(LogLevel level, const std::string& message) override;

};