#pragma once
#include "ILogger.hpp"
#include <string>

class ConsoleLogger : public ILogger {
public:
    void info(const std::string& message) override;

    void error(const std::string& message) override;

    void warn(const std::string& message) override;
};