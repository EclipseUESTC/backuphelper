#pragma once
#include <string>
#include <memory>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <set>
#include <regex>
#include "models/File.hpp"

namespace fs = std::filesystem;

class Filter {
public:
    virtual ~Filter() = default;
    virtual bool match(const File& file) const = 0;

    virtual std::string getFilterDescription() const = 0;
};

class PathFilter: public Filter {
private:
    std::vector<std::string> excludedPaths;
    
public:
    PathFilter() : excludedPaths({}) {}
    ~PathFilter() = default;

    void addExcludedPath(const std::string& path);
    bool removeExcludedPath(const std::string& path);
    bool isPathExcluded(const std::string& path) const;
    const std::vector<std::string>& getExcludedPaths() const {
        return this->excludedPaths;
    }
    void clearExcludedPaths() {
        this->excludedPaths.clear();
    }
    bool match(const File& file) const override;

    std::string getFilterDescription() const override;
};

class TypeFilter: public Filter {
private:
    std::set<std::string> includedTypes;

public:
    TypeFilter() : includedTypes({}) {}
    ~TypeFilter() = default;

    void addIncludedType(const std::string& type);
    bool removeIncludedType(const std::string& type);
    bool isTypeIncluded(const std::string& type) const;
    const std::set<std::string>& getIncludedTypes() const {
        return this->includedTypes;
    }
    void clearIncludedTypes() {
        this->includedTypes.clear();
    }
    bool match(const File& file) const override;

    std::string getFilterDescription() const override;
};

class TimeFilter: public Filter {
private:
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
    bool hasTimeRange; // 标记是否设置了时间范围

public:
    TimeFilter() : startTime(std::chrono::system_clock::time_point()), endTime(std::chrono::system_clock::time_point()), hasTimeRange(false) {}
    ~TimeFilter() = default;

    void setTimeRange(std::chrono::system_clock::time_point start, std::chrono::system_clock::time_point end);
    bool match(const File& file) const override;

    std::string getFilterDescription() const override;
};

class SizeFilter: public Filter {
private:
    uint64_t minSize;
    uint64_t maxSize;

public:
    SizeFilter() : minSize(0), maxSize(0) {}
    ~SizeFilter() = default;

    void setSizeRange(uint64_t min, uint64_t max);
    bool match(const File& file) const override;

    std::string getFilterDescription() const override;
};

class ExtensionFilter: public Filter {
private:
    std::vector<std::string> includedExtensions;
    
    // 辅助方法：获取文件扩展名
    std::string getFileExtension(const std::string& fileName) const;
    
public:
    ExtensionFilter() : includedExtensions() {}
    ~ExtensionFilter() = default;
    
    // 添加包含的扩展名
    void addIncludedExtension(const std::string& extension);
    // 移除包含的扩展名
    bool removeIncludedExtension(const std::string& extension);
    // 检查扩展名是否被包含
    bool isExtensionIncluded(const std::string& extension) const;
    // 获取所有包含的扩展名
    const std::vector<std::string>& getIncludedExtensions() const;
    // 清空所有包含的扩展名
    void clearIncludedExtensions();
    
    // 匹配方法实现
    bool match(const File& file) const override;
    // 获取过滤器描述
    std::string getFilterDescription() const override;
};

class NameFilter: public Filter {
private:
    std::vector<std::regex> includePatterns;    // 包含的正则表达式模式
    std::vector<std::regex> excludePatterns;    // 排除的正则表达式模式
    std::vector<std::string> includePatternStrings; // 存储原始字符串用于描述
    std::vector<std::string> excludePatternStrings; // 存储原始字符串用于描述

public:
    NameFilter() : includePatterns(), excludePatterns(), 
                   includePatternStrings(), excludePatternStrings() {}
    ~NameFilter() = default;

    // 添加包含的正则表达式模式
    void addIncludePattern(const std::string& pattern);
    // 移除包含的正则表达式模式
    bool removeIncludePattern(const std::string& pattern);
    // 添加排除的正则表达式模式
    void addExcludePattern(const std::string& pattern);
    // 移除排除的正则表达式模式
    bool removeExcludePattern(const std::string& pattern);
    // 清空所有包含模式
    void clearIncludePatterns();
    // 清空所有排除模式
    void clearExcludePatterns();
    
    // 匹配方法实现
    bool match(const File& file) const override;
    // 获取过滤器描述
    std::string getFilterDescription() const override;
};