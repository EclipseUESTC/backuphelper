#pragma once
#include <string>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR_LEVEL
};

class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void info(const std::string& message) = 0;

    virtual void error(const std::string& message) = 0;

    virtual void warn(const std::string& message) = 0;
    
    virtual void debug(const std::string& message) = 0;
    
    virtual void setLogLevel(LogLevel level) = 0;
    
    virtual LogLevel getLogLevel() const = 0;
    
    virtual void log(LogLevel level, const std::string& message) = 0;
};

