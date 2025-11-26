#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>
#include <fstream>
#include <string>
#include "core/Filter.hpp"
#include "core/models/File.hpp"

namespace fs = std::filesystem;
using namespace std::chrono;

// PathFilter测试用例
class PathFilterTest : public ::testing::Test {
protected:
    PathFilter filter;
    fs::path testDir = fs::temp_directory_path() / "backup_test";
    fs::path excludedDir = testDir / "excluded";
    fs::path includedDir = testDir / "included";
    
    void SetUp() override {
        // 创建测试目录结构
        fs::create_directories(excludedDir);
        fs::create_directories(includedDir);
        
        // 创建测试文件
        std::ofstream(excludedDir / "file1.txt") << "test content";
        std::ofstream(includedDir / "file2.txt") << "test content";
    }
    
    void TearDown() override {
        // 清理测试目录
        fs::remove_all(testDir);
    }
};

TEST_F(PathFilterTest, TestAddAndRemoveExcludedPath) {
    std::string excludedPath = excludedDir.string() + std::string(1, fs::path::preferred_separator);
    
    // 测试添加排除路径
    filter.addExcludedPath(excludedPath);
    EXPECT_EQ(1, filter.getExcludedPaths().size());
    EXPECT_TRUE(filter.isPathExcluded(excludedPath));
    
    // 测试移除排除路径
    EXPECT_TRUE(filter.removeExcludedPath(excludedPath));
    EXPECT_EQ(0, filter.getExcludedPaths().size());
    EXPECT_FALSE(filter.isPathExcluded(excludedPath));
    
    // 测试移除排除路径
    EXPECT_TRUE(filter.removeExcludedPath(excludedPath));
    EXPECT_EQ(0, filter.getExcludedPaths().size());
    EXPECT_FALSE(filter.isPathExcluded(excludedPath));
}

TEST_F(PathFilterTest, TestMatchIncludedFile) {
    File includedFile(includedDir / "file2.txt");
    
    // 没有排除路径，应该匹配
    EXPECT_TRUE(filter.match(includedFile));
}

TEST_F(PathFilterTest, TestMatchExcludedFile) {
    std::string excludedPath = excludedDir.string() + std::string(1, fs::path::preferred_separator);
    filter.addExcludedPath(excludedPath);
   // 测试匹配排除路径下的文件
    File excludedFile(excludedDir / "file1.txt");
    // 排除路径中的文件，应该不匹配
    EXPECT_FALSE(filter.match(excludedFile));
}

TEST_F(PathFilterTest, TestMatchExcludedDirectory) {
    std::string excludedPath = excludedDir.string() + std::string(1, fs::path::preferred_separator);
    filter.addExcludedPath(excludedPath);
   // 测试匹配排除路径下的目录
    File excludedDirFile(excludedDir);
    // 排除目录，应该不匹配
    EXPECT_FALSE(filter.match(excludedDirFile));
}

// TypeFilter测试用例
class TypeFilterTest : public ::testing::Test {
protected:
    TypeFilter filter;
    fs::path testDir = fs::temp_directory_path() / "backup_test_type";
    
    void SetUp() override {
        fs::create_directories(testDir);
        std::ofstream fileStream((testDir / "file.txt").string());
        fileStream << "test content";
        fileStream.close();
    }
    
    void TearDown() override {
        fs::remove_all(testDir);
    }
};

TEST_F(TypeFilterTest, TestAddAndRemoveIncludedType) {
    // 测试添加包含类型
    filter.addIncludedType("regular");
    EXPECT_EQ(1, filter.getIncludedTypes().size());
    EXPECT_TRUE(filter.isTypeIncluded("regular"));
    
    // 测试移除包含类型
    EXPECT_TRUE(filter.removeIncludedType("regular"));
    EXPECT_EQ(0, filter.getIncludedTypes().size());
    EXPECT_FALSE(filter.isTypeIncluded("regular"));
}

TEST_F(TypeFilterTest, TestMatchIncludedType) {
    filter.addIncludedType("regular");
    File regularFile(testDir / "file.txt");
    
    // 包含类型为regular，应该匹配
    EXPECT_TRUE(filter.match(regularFile));
}

TEST_F(TypeFilterTest, TestMatchExcludedType) {
    filter.addIncludedType("directory");
    File regularFile(testDir / "file.txt");
    
    // 包含类型为directory，regular文件不匹配
    EXPECT_FALSE(filter.match(regularFile));
}

TEST_F(TypeFilterTest, TestMatchDirectoryType) {
    filter.addIncludedType("directory");
    File dirFile(testDir);
    
    // 包含类型为directory，目录应该匹配
    EXPECT_TRUE(filter.match(dirFile));
}

// SizeFilter测试用例
class SizeFilterTest : public ::testing::Test {
protected:
    SizeFilter filter;
    fs::path testDir = fs::temp_directory_path() / "backup_test_size";
    
    void SetUp() override {
        fs::create_directories(testDir);
        
        // 创建不同大小的文件
        std::ofstream smallFile((testDir / "small.txt").string());
        smallFile << "123";
        smallFile.close();
        
        std::ofstream mediumFile((testDir / "medium.txt").string());
        mediumFile << std::string(100, 'a');
        mediumFile.close();
        
        std::ofstream largeFile((testDir / "large.txt").string());
        largeFile << std::string(1000, 'b');
        largeFile.close();
    }
    
    void TearDown() override {
        fs::remove_all(testDir);
    }
};

TEST_F(SizeFilterTest, TestMatchAllFilesWhenNoRangeSet) {
    // 没有设置大小范围，应该匹配所有文件
    File smallFile(testDir / "small.txt");
    File mediumFile(testDir / "medium.txt");
    File largeFile(testDir / "large.txt");
    
    EXPECT_TRUE(filter.match(smallFile));
    EXPECT_TRUE(filter.match(mediumFile));
    EXPECT_TRUE(filter.match(largeFile));
}

TEST_F(SizeFilterTest, TestMatchFilesWithinRange) {
    // 设置大小范围为50-200字节
    filter.setSizeRange(50, 200);
    
    File smallFile(testDir / "small.txt"); // ~4字节
    File mediumFile(testDir / "medium.txt"); // ~100字节
    File largeFile(testDir / "large.txt"); // ~1000字节
    
    EXPECT_FALSE(filter.match(smallFile)); // 太小，不匹配
    EXPECT_TRUE(filter.match(mediumFile)); // 在范围内，匹配
    EXPECT_FALSE(filter.match(largeFile)); // 太大，不匹配
}

TEST_F(SizeFilterTest, TestMatchFilesWithMinSize) {
    // 设置最小大小为100字节
    filter.setSizeRange(100, 0);
    
    File smallFile(testDir / "small.txt"); // ~4字节
    File mediumFile(testDir / "medium.txt"); // ~100字节
    File largeFile(testDir / "large.txt"); // ~1000字节
    
    EXPECT_FALSE(filter.match(smallFile)); // 小于最小大小，不匹配
    EXPECT_TRUE(filter.match(mediumFile)); // 等于最小大小，匹配
    EXPECT_TRUE(filter.match(largeFile)); // 大于最小大小，匹配
}

TEST_F(SizeFilterTest, TestMatchFilesWithMaxSize) {
    // 设置最大大小为100字节
    filter.setSizeRange(0, 100);
    
    File smallFile(testDir / "small.txt"); // ~4字节
    File mediumFile(testDir / "medium.txt"); // ~100字节
    File largeFile(testDir / "large.txt"); // ~1000字节
    
    EXPECT_TRUE(filter.match(smallFile)); // 小于最大大小，匹配
    EXPECT_TRUE(filter.match(mediumFile)); // 等于最大大小，匹配
    EXPECT_FALSE(filter.match(largeFile)); // 大于最大大小，不匹配
}

// NameFilter测试用例
class NameFilterTest : public ::testing::Test {
protected:
    NameFilter filter;
    fs::path testDir = fs::temp_directory_path() / "backup_test_name";
    
    void SetUp() override {
        fs::create_directories(testDir);
        
        // 创建不同名称的文件
        std::ofstream file1((testDir / "file1.txt").string());
        file1 << "test content";
        file1.close();
        
        std::ofstream file2((testDir / "file2.jpg").string());
        file2 << "test content";
        file2.close();
        
        std::ofstream file3((testDir / "backup.log").string());
        file3 << "log content";
        file3.close();
        
        std::ofstream file4((testDir / "temp.tmp").string());
        file4 << "temp content";
        file4.close();
    }
    
    void TearDown() override {
        fs::remove_all(testDir);
    }
};

TEST_F(NameFilterTest, TestMatchAllFilesWhenNoPatternSet) {
    // 没有设置模式，应该匹配所有文件
    File txtFile(testDir / "file1.txt");
    File jpgFile(testDir / "file2.jpg");
    File logFile(testDir / "backup.log");
    File tmpFile(testDir / "temp.tmp");
    
    EXPECT_TRUE(filter.match(txtFile));
    EXPECT_TRUE(filter.match(jpgFile));
    EXPECT_TRUE(filter.match(logFile));
    EXPECT_TRUE(filter.match(tmpFile));
}

TEST_F(NameFilterTest, TestIncludePattern) {
    // 设置包含模式：只匹配.txt文件
    filter.addIncludePattern(".*\\.txt$");
    
    File txtFile(testDir / "file1.txt");
    File jpgFile(testDir / "file2.jpg");
    File logFile(testDir / "backup.log");
    
    EXPECT_TRUE(filter.match(txtFile)); // 匹配.txt，应该匹配
    EXPECT_FALSE(filter.match(jpgFile)); // 不匹配.txt，不应该匹配
    EXPECT_FALSE(filter.match(logFile)); // 不匹配.txt，不应该匹配
}

TEST_F(NameFilterTest, TestExcludePattern) {
    // 设置排除模式：不匹配.log文件
    filter.addExcludePattern(".*\\.log$");
    
    File txtFile(testDir / "file1.txt");
    File jpgFile(testDir / "file2.jpg");
    File logFile(testDir / "backup.log");
    
    EXPECT_TRUE(filter.match(txtFile)); // 不匹配排除模式，应该匹配
    EXPECT_TRUE(filter.match(jpgFile)); // 不匹配排除模式，应该匹配
    EXPECT_FALSE(filter.match(logFile)); // 匹配排除模式，不应该匹配
}

TEST_F(NameFilterTest, TestIncludeAndExcludePattern) {
    // 设置包含模式：匹配所有文件，排除模式：不匹配.tmp文件
    filter.addIncludePattern(".*");
    filter.addExcludePattern(".*\\.tmp$");
    
    File txtFile(testDir / "file1.txt");
    File jpgFile(testDir / "file2.jpg");
    File tmpFile(testDir / "temp.tmp");
    
    EXPECT_TRUE(filter.match(txtFile)); // 匹配包含模式，不匹配排除模式，应该匹配
    EXPECT_TRUE(filter.match(jpgFile)); // 匹配包含模式，不匹配排除模式，应该匹配
    EXPECT_FALSE(filter.match(tmpFile)); // 匹配包含模式，但也匹配排除模式，不应该匹配
}

// TimeFilter测试用例
class TimeFilterTest : public ::testing::Test {
protected:
    TimeFilter filter;
    fs::path testDir = fs::temp_directory_path() / "backup_test_time";
    
    void SetUp() override {
        fs::create_directories(testDir);
    }
    
    void TearDown() override {
        fs::remove_all(testDir);
    }
};

TEST_F(TimeFilterTest, TestMatchAllFilesWhenNoRangeSet) {
    // 创建测试文件
    std::ofstream(testDir / "file.txt") << "test content";
    File testFile(testDir / "file.txt");
    
    // 没有设置时间范围，应该匹配
    EXPECT_TRUE(filter.match(testFile));
}

TEST_F(TimeFilterTest, TestMatchFilesWithinTimeRange) {
    // 获取当前时间
    auto now = system_clock::now();
    auto oneHourAgo = now - hours(1);
    auto twoHoursAgo = now - hours(2);
    
    // 设置时间范围为过去1小时到现在
    filter.setTimeRange(oneHourAgo, now);
    
    // 创建测试文件
    std::ofstream(testDir / "recent.txt") << "test content";
    File recentFile(testDir / "recent.txt");
    
    // 最近创建的文件应该匹配
    EXPECT_TRUE(filter.match(recentFile));
    
    // 修改时间为过去2小时 - 使用兼容C++17的方式
    std::ofstream oldFileStream((testDir / "old.txt").string());
    oldFileStream << "old content";
    oldFileStream.close();
    
    // 注意：在C++17中，我们无法直接将系统时间点转换为文件时间点
    // 这里我们使用一个间接方法：先获取当前文件时间，然后减去相应的时长
    auto nowFileTime = std::filesystem::file_time_type::clock::now();
    auto twoHours = std::chrono::duration_cast<std::filesystem::file_time_type::duration>(std::chrono::hours(2));
    auto oldFileTime = nowFileTime - twoHours;
    std::filesystem::last_write_time(testDir / "old.txt", oldFileTime);
    File oldFile(testDir / "old.txt");
    
    // 旧文件不应该匹配
    EXPECT_FALSE(filter.match(oldFile));
}

// 测试Filter在实际备份场景中的使用
TEST(FilterIntegrationTest, TestMultipleFilters) {
    // 创建测试目录和文件
    fs::path testDir = fs::temp_directory_path() / "backup_integration";
    fs::create_directories(testDir / "docs");
    fs::create_directories(testDir / "images");
    fs::create_directories(testDir / "temp");
    
    std::ofstream(testDir / "docs" / "file1.txt") << "document content";
    std::ofstream(testDir / "images" / "image1.jpg") << "image content";
    std::ofstream(testDir / "temp" / "temp1.tmp") << "temp content";
    
    // 创建过滤器
    PathFilter pathFilter;
    TypeFilter typeFilter;
    NameFilter nameFilter;
    
    // 配置过滤器：
    // 1. 排除temp目录
    std::string tempPath = (testDir / "temp").string() + std::string(1, fs::path::preferred_separator);
    pathFilter.addExcludedPath(tempPath);
    
    // 2. 只包含regular和directory类型
    typeFilter.addIncludedType("regular");
    typeFilter.addIncludedType("directory");
    
    // 3. 排除.tmp文件
    nameFilter.addExcludePattern(".*\\.tmp$");
    
    // 测试不同文件
    File docFile(testDir / "docs" / "file1.txt");
    File imgFile(testDir / "images" / "image1.jpg");
    File tempFile(testDir / "temp" / "temp1.tmp");
    File tempDirFile(testDir / "temp");
    File docsDirFile(testDir / "docs");
    
    // 文档文件应该通过所有过滤器
    EXPECT_TRUE(pathFilter.match(docFile));
    EXPECT_TRUE(typeFilter.match(docFile));
    EXPECT_TRUE(nameFilter.match(docFile));
    
    // 图片文件应该通过所有过滤器
    EXPECT_TRUE(pathFilter.match(imgFile));
    EXPECT_TRUE(typeFilter.match(imgFile));
    EXPECT_TRUE(nameFilter.match(imgFile));
    
    // temp目录下的文件应该被PathFilter排除
    EXPECT_FALSE(pathFilter.match(tempFile));
    EXPECT_TRUE(typeFilter.match(tempFile));
    EXPECT_FALSE(nameFilter.match(tempFile));
    
    // temp目录应该被PathFilter排除
    EXPECT_FALSE(pathFilter.match(tempDirFile));
    EXPECT_TRUE(typeFilter.match(tempDirFile));
    EXPECT_TRUE(nameFilter.match(tempDirFile));
    
    // docs目录应该通过所有过滤器
    EXPECT_TRUE(pathFilter.match(docsDirFile));
    EXPECT_TRUE(typeFilter.match(docsDirFile));
    EXPECT_TRUE(nameFilter.match(docsDirFile));
    
    // 清理测试目录
    fs::remove_all(testDir);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
