#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <atomic>
#include "core/tasks/BackupTask.hpp"
#include "core/tasks/RestoreTask.hpp"
#include "utils/ILogger.hpp"

namespace fs = std::filesystem;

// 模拟ILogger接口
class MockLogger : public ILogger {
public:
    MOCK_METHOD(void, info, (const std::string& message), (override));
    MOCK_METHOD(void, error, (const std::string& message), (override));
    MOCK_METHOD(void, warn, (const std::string& message), (override));
    MOCK_METHOD(void, debug, (const std::string& message), (override));
    MOCK_METHOD(void, setLogLevel, (LogLevel level), (override));
    MOCK_METHOD(LogLevel, getLogLevel, (), (const, override));
    MOCK_METHOD(void, log, (LogLevel level, const std::string& message), (override));
};

// Task测试用例基类
class TaskTest : public ::testing::Test {
protected:
    fs::path testDir = fs::temp_directory_path() / "backup_task_test";
    fs::path sourceDir = testDir / "source";
    fs::path backupDir = testDir / "backup";
    fs::path restoreDir = testDir / "restore";
    fs::path packageFile = backupDir / "backup.pkg";
    std::string testPassword = "StrongPassword123!";
    
    std::shared_ptr<MockLogger> mockLogger;
    std::atomic<bool> interruptFlag{false};
    
    void SetUp() override {
        // 创建测试目录结构
        fs::create_directories(sourceDir / "subdir1");
        fs::create_directories(sourceDir / "subdir2");
        fs::create_directories(backupDir);
        fs::create_directories(restoreDir);
        
        // 创建测试文件
        std::ofstream(sourceDir / "file1.txt") << "Content of file 1";
        std::ofstream(sourceDir / "file2.txt") << "Content of file 2";
        std::ofstream(sourceDir / "subdir1" / "file3.txt") << "Content of file 3 in subdir1";
        std::ofstream(sourceDir / "subdir2" / "file4.txt") << "Content of file 4 in subdir2";
        
        // 创建模拟日志器
        mockLogger = std::make_shared<MockLogger>();
        
        // 设置默认的日志器期望 - 允许所有日志方法调用
        EXPECT_CALL(*mockLogger, log(::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Return());
        EXPECT_CALL(*mockLogger, setLogLevel(::testing::_))
            .WillRepeatedly(::testing::Return());
        EXPECT_CALL(*mockLogger, getLogLevel())
            .WillRepeatedly(::testing::Return(LogLevel::INFO));
        
        // 允许所有info方法调用
        EXPECT_CALL(*mockLogger, info(::testing::_))
            .WillRepeatedly(::testing::Return());
        // 允许所有error方法调用
        EXPECT_CALL(*mockLogger, error(::testing::_))
            .WillRepeatedly(::testing::Return());
        // 允许所有warn方法调用
        EXPECT_CALL(*mockLogger, warn(::testing::_))
            .WillRepeatedly(::testing::Return());
        // 允许所有debug方法调用
        EXPECT_CALL(*mockLogger, debug(::testing::_))
            .WillRepeatedly(::testing::Return());
    }
    
    void TearDown() override {
        // 清理测试目录
        fs::remove_all(testDir);
    }
    
    // 辅助函数：比较两个目录的内容
    bool compareDirectories(const fs::path& dir1, const fs::path& dir2) {
        // 遍历第一个目录
        for (const auto& entry : fs::recursive_directory_iterator(dir1)) {
            // 计算相对路径
            fs::path relativePath = fs::relative(entry.path(), dir1);
            fs::path targetPath = dir2 / relativePath;
            
            // 检查目标路径是否存在
            if (!fs::exists(targetPath)) {
                return false;
            }
            
            // 如果是文件，比较内容
            if (fs::is_regular_file(entry)) {
                std::ifstream file1(entry.path());
                std::ifstream file2(targetPath);
                std::string content1((std::istreambuf_iterator<char>(file1)), std::istreambuf_iterator<char>());
                std::string content2((std::istreambuf_iterator<char>(file2)), std::istreambuf_iterator<char>());
                
                if (content1 != content2) {
                    return false;
                }
            }
        }
        
        return true;
    }
};

// 测试基本备份任务
TEST_F(TaskTest, BackupTaskBasic) {
    // 创建空过滤器列表
    std::vector<std::shared_ptr<Filter>> filters;
    
    // 创建备份任务
    BackupTask backupTask(sourceDir.string(), backupDir.string(), mockLogger.get(), 
                         filters, true, false, "backup.pkg", "");
    
    // 执行备份任务
    EXPECT_TRUE(backupTask.execute());
    
    // 验证备份任务状态
    EXPECT_EQ(backupTask.getStatus(), TaskStatus::COMPLETED);
    
    // 验证备份目录中包含源目录的内容
    EXPECT_TRUE(compareDirectories(sourceDir, backupDir));
}

// 测试带打包的备份任务
TEST_F(TaskTest, BackupTaskWithPackage) {
    // 创建空过滤器列表
    std::vector<std::shared_ptr<Filter>> filters;
    
    // 创建带打包的备份任务
    BackupTask backupTask(sourceDir.string(), backupDir.string(), mockLogger.get(), 
                         filters, true, true, "backup.pkg", "");
    
    // 执行备份任务
    EXPECT_TRUE(backupTask.execute());
    
    // 验证备份任务状态
    EXPECT_EQ(backupTask.getStatus(), TaskStatus::COMPLETED);
    
    // 验证包文件已生成 - 非加密备份生成普通.pkg文件
    EXPECT_TRUE(fs::exists(packageFile));
    EXPECT_GT(fs::file_size(packageFile), 0);
}

// 测试带加密的备份任务
TEST_F(TaskTest, BackupTaskWithEncryption) {
    // 创建空过滤器列表
    std::vector<std::shared_ptr<Filter>> filters;
    
    // 创建带加密的备份任务
    BackupTask backupTask(sourceDir.string(), backupDir.string(), mockLogger.get(), 
                         filters, true, true, "backup.pkg", testPassword);
    
    // 执行备份任务
    EXPECT_TRUE(backupTask.execute());
    
    // 验证备份任务状态
    EXPECT_EQ(backupTask.getStatus(), TaskStatus::COMPLETED);
    
    // 验证包文件已生成 - 加密备份会生成.enc文件
    fs::path encryptedPackageFile = backupDir / "backup.pkg.enc";
    EXPECT_TRUE(fs::exists(encryptedPackageFile));
    EXPECT_GT(fs::file_size(encryptedPackageFile), 0);
}

// 测试基本还原任务
TEST_F(TaskTest, RestoreTaskBasic) {
    // 首先执行备份任务，生成备份数据
    std::vector<std::shared_ptr<Filter>> filters;
    BackupTask backupTask(sourceDir.string(), backupDir.string(), mockLogger.get(), filters, true, false);
    EXPECT_TRUE(backupTask.execute());
    
    // 创建还原任务
    RestoreTask restoreTask(backupDir.string(), restoreDir.string(), mockLogger.get(), filters, true, false);
    
    // 执行还原任务
    EXPECT_TRUE(restoreTask.execute());
    
    // 验证还原任务状态
    EXPECT_EQ(restoreTask.getStatus(), TaskStatus::COMPLETED);
    
    // 验证还原目录中包含源目录的内容
    EXPECT_TRUE(compareDirectories(sourceDir, restoreDir));
}

// 测试带打包的还原任务
TEST_F(TaskTest, RestoreTaskWithPackage) {
    // 首先执行带打包的备份任务，生成备份包
    std::vector<std::shared_ptr<Filter>> filters;
    BackupTask backupTask(sourceDir.string(), backupDir.string(), mockLogger.get(), 
                         filters, true, true, "backup.pkg", "");
    EXPECT_TRUE(backupTask.execute());
    
    // 创建带打包的还原任务
    RestoreTask restoreTask(backupDir.string(), restoreDir.string(), mockLogger.get(), 
                          filters, true, true, "backup.pkg", "");
    
    // 执行还原任务
    EXPECT_TRUE(restoreTask.execute());
    
    // 验证还原任务状态
    EXPECT_EQ(restoreTask.getStatus(), TaskStatus::COMPLETED);
    
    // 验证还原目录中包含源目录的内容
    EXPECT_TRUE(compareDirectories(sourceDir, restoreDir));
}

// 测试带加密的还原任务
TEST_F(TaskTest, RestoreTaskWithEncryption) {
    // 首先执行带加密的备份任务，生成加密备份包
    std::vector<std::shared_ptr<Filter>> filters;
    BackupTask backupTask(sourceDir.string(), backupDir.string(), mockLogger.get(), 
                         filters, true, true, "backup.pkg", testPassword);
    EXPECT_TRUE(backupTask.execute());
    
    // 创建带加密的还原任务
    RestoreTask restoreTask(backupDir.string(), restoreDir.string(), mockLogger.get(), 
                          filters, true, true, "backup.pkg", testPassword);
    
    // 执行还原任务
    EXPECT_TRUE(restoreTask.execute());
    
    // 验证还原任务状态
    EXPECT_EQ(restoreTask.getStatus(), TaskStatus::COMPLETED);
    
    // 验证还原目录中包含源目录的内容
    EXPECT_TRUE(compareDirectories(sourceDir, restoreDir));
}

// 测试中断标志
TEST_F(TaskTest, BackupTaskInterrupt) {
    // 创建空过滤器列表
    std::vector<std::shared_ptr<Filter>> filters;
    
    // 创建带中断标志的备份任务
    BackupTask backupTask(sourceDir.string(), backupDir.string(), mockLogger.get(), 
                         filters, true, false, "backup.pkg", "", &interruptFlag);
    
    // 设置中断标志
    interruptFlag = true;
    
    // 执行备份任务，应该返回false表示被中断
    EXPECT_FALSE(backupTask.execute());
    
    // 验证备份任务状态 - 被中断的任务应该是CANCELLED状态
    EXPECT_EQ(backupTask.getStatus(), TaskStatus::CANCELLED);
}

// 测试使用错误密码还原
TEST_F(TaskTest, RestoreTaskWithWrongPassword) {
    // 首先执行带加密的备份任务，生成加密备份包
    std::vector<std::shared_ptr<Filter>> filters;
    BackupTask backupTask(sourceDir.string(), backupDir.string(), mockLogger.get(), 
                         filters, true, true, "backup.pkg", testPassword);
    EXPECT_TRUE(backupTask.execute());
    
    // 创建带错误密码的还原任务
    std::string wrongPassword = "WrongPassword!";
    RestoreTask restoreTask(backupDir.string(), restoreDir.string(), mockLogger.get(), 
                          filters, true, true, "backup.pkg", wrongPassword);
    
    // 执行还原任务，应该失败
    EXPECT_FALSE(restoreTask.execute());
    
    // 验证还原任务状态
    EXPECT_EQ(restoreTask.getStatus(), TaskStatus::FAILED);
}

// 测试不存在的源目录备份
TEST_F(TaskTest, BackupTaskNonExistentSource) {
    // 创建不存在的源目录路径
    fs::path nonExistentSource = testDir / "non_existent_source";
    
    // 创建备份任务
    std::vector<std::shared_ptr<Filter>> filters;
    BackupTask backupTask(nonExistentSource.string(), backupDir.string(), mockLogger.get(), filters);
    
    // 执行备份任务，应该失败
    EXPECT_FALSE(backupTask.execute());
    
    // 验证备份任务状态
    EXPECT_EQ(backupTask.getStatus(), TaskStatus::FAILED);
}

// 测试不存在的备份目录还原
TEST_F(TaskTest, RestoreTaskNonExistentBackup) {
    // 创建不存在的备份目录路径
    fs::path nonExistentBackup = testDir / "non_existent_backup";
    
    // 创建还原任务
    std::vector<std::shared_ptr<Filter>> filters;
    RestoreTask restoreTask(nonExistentBackup.string(), restoreDir.string(), mockLogger.get(), filters);
    
    // 执行还原任务，应该失败
    EXPECT_FALSE(restoreTask.execute());
    
    // 验证还原任务状态
    EXPECT_EQ(restoreTask.getStatus(), TaskStatus::FAILED);
}

// 测试带过滤器的备份任务
TEST_F(TaskTest, BackupTaskWithFilters) {
    // 创建路径过滤器，排除subdir2目录
    std::vector<std::shared_ptr<Filter>> filters;
    auto pathFilter = std::make_shared<PathFilter>();
    std::string excludedPath = (sourceDir / "subdir2").string() + std::string(1, fs::path::preferred_separator);
    pathFilter->addExcludedPath(excludedPath);
    filters.push_back(pathFilter);
    
    // 创建带过滤器的备份任务
    BackupTask backupTask(sourceDir.string(), backupDir.string(), mockLogger.get(), filters, true, false);
    
    // 执行备份任务
    EXPECT_TRUE(backupTask.execute());
    
    // 验证备份任务状态
    EXPECT_EQ(backupTask.getStatus(), TaskStatus::COMPLETED);
    
    // 验证subdir1中的文件被备份
    EXPECT_TRUE(fs::exists(backupDir / "subdir1" / "file3.txt"));
    
    // 验证subdir2被排除，没有被备份
    EXPECT_FALSE(fs::exists(backupDir / "subdir2"));
}

// 测试任务状态初始值
TEST_F(TaskTest, TaskStatusInitialValue) {
    // 创建空过滤器列表
    std::vector<std::shared_ptr<Filter>> filters;
    
    // 创建备份任务并检查初始状态
    BackupTask backupTask(sourceDir.string(), backupDir.string(), mockLogger.get(), filters);
    EXPECT_EQ(backupTask.getStatus(), TaskStatus::PENDING);
    
    // 创建还原任务并检查初始状态
    RestoreTask restoreTask(backupDir.string(), restoreDir.string(), mockLogger.get(), filters);
    EXPECT_EQ(restoreTask.getStatus(), TaskStatus::PENDING);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
