#pragma once
#include <iostream>
#include <string>

class Logger {
public:
    static void info(const std::string& message);

    static void error(const std::string& message);

    static void debug(const std::string& message);

private:
    static void log(const std::string& level, const std::string& message);
};

