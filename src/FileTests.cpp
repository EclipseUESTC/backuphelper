#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include "core/models/File.hpp"

namespace fs = std::filesystem;
using namespace std::chrono;

// File类测试用例
class FileTest : public ::testing::Test {
protected:
    fs::path testDir = fs::temp_directory_path() / "backup_file_test";
    fs::path testFile = testDir / "test.txt";
    fs::path testDirPath = testDir / "subdir";
    fs::path testSymlink = testDir / "symlink.txt";
    
    void SetUp() override {
        // 创建测试目录
        fs::create_directories(testDir);
        fs::create_directories(testDirPath);
        
        // 创建测试文件
        std::ofstream(testFile) << "This is a test file.";
        
        // 创建符号链接 - 在Windows上可能需要管理员权限，所以使用try-catch
        try {
            fs::create_symlink(testFile, testSymlink);
        } catch (const std::exception& e) {
            // 忽略符号链接创建失败，因为在Windows上可能没有权限
            // 测试会跳过需要符号链接的测试用例
        }
    }
    
    void TearDown() override {
        // 清理测试目录
        fs::remove_all(testDir);
    }
};

// 测试默认构造函数
TEST_F(FileTest, DefaultConstructor) {
    File file;
    EXPECT_FALSE(file.exists());
    EXPECT_FALSE(file.isRegularFile());
    EXPECT_FALSE(file.isDirectory());
}

// 测试带路径的构造函数
TEST_F(FileTest, PathConstructor) {
    File file(testFile);
    EXPECT_TRUE(file.exists());
    EXPECT_TRUE(file.isRegularFile());
    EXPECT_EQ(file.getFilePath(), testFile);
    EXPECT_EQ(file.getFileName(), "test.txt");
}

// 测试initialize方法
TEST_F(FileTest, InitializeMethod) {
    File file;
    file.initialize(testFile);
    EXPECT_TRUE(file.exists());
    EXPECT_TRUE(file.isRegularFile());
    EXPECT_EQ(file.getFilePath(), testFile);
    EXPECT_EQ(file.getFileName(), "test.txt");
}

// 测试基本属性访问
TEST_F(FileTest, BasicProperties) {
    File file(testFile);
    
    // 测试文件大小
    EXPECT_GT(file.getFileSize(), 0);
    
    // 测试文件类型
    EXPECT_EQ(file.getFileType(), fs::file_type::regular);
    
    // 测试文件名称
    EXPECT_EQ(file.getFileName(), "test.txt");
    
    // 测试文件路径
    EXPECT_EQ(file.getFilePath(), testFile);
}

// 测试文件类型判断
TEST_F(FileTest, FileTypeChecks) {
    // 测试普通文件
    File regularFile(testFile);
    EXPECT_TRUE(regularFile.isRegularFile());
    EXPECT_FALSE(regularFile.isDirectory());
    EXPECT_FALSE(regularFile.isSymbolicLink());
    
    // 测试目录
    File dirFile(testDirPath);
    EXPECT_TRUE(dirFile.isDirectory());
    EXPECT_FALSE(dirFile.isRegularFile());
    EXPECT_FALSE(dirFile.isSymbolicLink());
    
    // 测试符号链接 - 只在符号链接存在时测试
    if (fs::exists(testSymlink) && fs::is_symlink(testSymlink)) {
        File symlinkFile(testSymlink);
        EXPECT_TRUE(symlinkFile.isSymbolicLink());
        EXPECT_FALSE(symlinkFile.isRegularFile());
        EXPECT_FALSE(symlinkFile.isDirectory());
    }
}

// 测试时间戳访问
TEST_F(FileTest, TimeStamps) {
    File file(testFile);
    
    // 获取当前时间
    auto now = system_clock::now();
    
    // 测试时间戳是否有效
    EXPECT_LE(file.getCreationTime(), now);
    EXPECT_LE(file.getLastModifiedTime(), now);
    EXPECT_LE(file.getLastAccessTime(), now);
    
    // 测试文件修改后时间戳变化
    auto oldModifiedTime = file.getLastModifiedTime();
    
    // 修改文件内容，确保文件流被正确关闭
    {
        std::ofstream fileStream(testFile, std::ios::out | std::ios::trunc);
        fileStream << "Updated content " << std::time(nullptr);
        // 文件流超出作用域，会被自动关闭，确保文件修改时间被更新
    }
    
    // 等待100ms确保时间戳变化
    std::this_thread::sleep_for(milliseconds(100));
    
    // 重新创建File对象来获取更新后的时间戳
    File updatedFile(testFile);
    
    // 验证修改时间已更新
    EXPECT_GT(updatedFile.getLastModifiedTime(), oldModifiedTime);
}

// 测试文件数据操作
TEST_F(FileTest, FileDataOperations) {
    File file(testFile);
    
    // 测试loadFileData方法
    EXPECT_TRUE(file.loadFileData());
    const auto& data = file.getFileData();
    EXPECT_GT(data.size(), 0);
    
    // 测试setFileData方法
    std::vector<char> newData = {'n', 'e', 'w', ' ', 'd', 'a', 't', 'a'};
    file.setFileData(newData);
    EXPECT_EQ(file.getFileData(), newData);
    
    // 测试saveFileData方法
    EXPECT_TRUE(file.saveFileData());
    
    // 验证文件内容已更新
    File updatedFile(testFile);
    EXPECT_TRUE(updatedFile.loadFileData());
    EXPECT_EQ(updatedFile.getFileData(), newData);
}

// 测试路径操作
TEST_F(FileTest, PathOperations) {
    File file(testFile);
    
    // 测试getRelativePath方法
    fs::path relativePath = file.getRelativePath(testDir);
    // 在Windows上，相对路径可能是".\test.txt"，在Linux上是"test.txt"
    // 所以我们只测试文件名部分是否正确
    EXPECT_EQ(relativePath.filename(), "test.txt");
    
    // 测试子目录中的文件
    fs::path subFile = testDirPath / "subtest.txt";
    std::ofstream(subFile) << "Subdirectory test file.";
    File subDirFile(subFile);
    relativePath = subDirFile.getRelativePath(testDir);
    // 只测试相对路径的文件名和父目录是否正确
    EXPECT_EQ(relativePath.filename(), "subtest.txt");
    EXPECT_EQ(relativePath.parent_path().filename(), "subdir");
}

// 测试字符串表示
TEST_F(FileTest, StringRepresentation) {
    File file(testFile);
    std::string str = file.toString();
    EXPECT_GT(str.size(), 0);
    EXPECT_NE(str.find("test.txt"), std::string::npos);
}

// 测试比较操作
TEST_F(FileTest, ComparisonOperations) {
    File file1(testFile);
    File file2(testFile);
    File file3(testDirPath);
    
    // 测试相等比较
    EXPECT_EQ(file1, file2);
    EXPECT_FALSE(file1 != file2);
    
    // 测试不等比较
    EXPECT_NE(file1, file3);
    EXPECT_FALSE(file1 == file3);
}

// 测试链接信息
TEST_F(FileTest, LinkInformation) {
    // 测试符号链接 - 只在符号链接存在时测试
    if (fs::exists(testSymlink) && fs::is_symlink(testSymlink)) {
        File symlinkFile(testSymlink);
        EXPECT_TRUE(symlinkFile.isSymbolicLink());
        EXPECT_EQ(symlinkFile.getSymlinkTarget(), testFile);
    }
    
    // 测试硬链接计数
    File regularFile(testFile);
    EXPECT_GT(regularFile.getHardLinkCount(), 0);
    EXPECT_FALSE(regularFile.getIsHardLink());
}

// 测试不存在的文件
TEST_F(FileTest, NonExistentFile) {
    fs::path nonExistentFile = testDir / "non_existent.txt";
    File file(nonExistentFile);
    
    EXPECT_FALSE(file.exists());
    EXPECT_FALSE(file.isRegularFile());
    EXPECT_FALSE(file.isDirectory());
    EXPECT_EQ(file.getFileSize(), 0);
}

// 测试文件权限和属主
TEST_F(FileTest, PermissionsAndOwner) {
    File file(testFile);
    
    // 测试权限（应该有一些默认权限）
    EXPECT_GT(file.getPermissions(), 0);
    
    // 测试属主ID（在Windows上可能返回0）
    EXPECT_NO_THROW(file.getOwnerId());
    EXPECT_NO_THROW(file.getGroupId());
}

// 测试特殊文件类型判断
TEST_F(FileTest, SpecialFileTypes) {
    File regularFile(testFile);
    
    EXPECT_FALSE(regularFile.isFIFO());
    EXPECT_FALSE(regularFile.isCharacterDevice());
    EXPECT_FALSE(regularFile.isBlockDevice());
    EXPECT_FALSE(regularFile.isSocket());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
