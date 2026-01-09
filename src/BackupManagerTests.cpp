#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <atomic>
#include "core/RealTimeBackupManager.hpp"
#include "core/TimerBackupManager.hpp"
#include "utils/ConsoleLogger.hpp"

namespace fs = std::filesystem;

// RealTimeBackupManager类测试用例
class RealTimeBackupManagerTest : public ::testing::Test {
protected:
    fs::path testDir = fs::temp_directory_path() / "realtime_backup_test";
    fs::path sourceDir = testDir / "source";
    fs::path backupDir = testDir / "backup";
    
    void SetUp() override {
        // 创建测试目录结构
        fs::create_directories(sourceDir);
        fs::create_directories(backupDir);
        
        // 创建初始测试文件
        std::ofstream(sourceDir / "initial.txt") << "Initial content";
    }
    
    void TearDown() override {
        // 清理测试目录
        fs::remove_all(testDir);
    }
};

// 测试实时备份功能
TEST_F(RealTimeBackupManagerTest, RealTimeBackup) {
    ConsoleLogger logger;
    RealTimeBackupManager backupManager(&logger);
    
    // 创建实时备份配置
    RealTimeBackupConfig config;
    config.sourceDir = sourceDir.string();
    config.backupDir = backupDir.string();
    config.compressEnabled = false;
    config.packageEnabled = false;
    config.debounceTimeMs = 1000;
    
    // 启动实时备份
    backupManager.start(config);
    
    // 等待监控器初始化
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // 创建新文件，应该触发实时备份
    std::ofstream(sourceDir / "new_file.txt") << "New file content";
    
    // 等待备份完成
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 验证新文件已被备份
    EXPECT_TRUE(fs::exists(backupDir / "new_file.txt"));
    
    // 修改现有文件，应该触发实时备份
    std::ofstream(sourceDir / "initial.txt", std::ios::out | std::ios::trunc) << "Modified content";
    
    // 等待备份完成
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 验证文件已被修改
    std::ifstream modifiedFile(backupDir / "initial.txt");
    std::string content((std::istreambuf_iterator<char>(modifiedFile)), std::istreambuf_iterator<char>());
    EXPECT_EQ(content, "Modified content");
    
    // 停止实时备份
    backupManager.stop();
}

// TimerBackupManager类测试用例
class TimerBackupManagerTest : public ::testing::Test {
protected:
    fs::path testDir = fs::temp_directory_path() / "timer_backup_test";
    fs::path sourceDir = testDir / "source";
    fs::path backupDir = testDir / "backup";
    
    void SetUp() override {
        // 创建测试目录结构
        fs::create_directories(sourceDir);
        fs::create_directories(backupDir);
        
        // 创建初始测试文件
        std::ofstream(sourceDir / "initial.txt") << "Initial content";
    }
    
    void TearDown() override {
        // 清理测试目录
        fs::remove_all(testDir);
    }
};

// 测试定时备份功能
TEST_F(TimerBackupManagerTest, TimerBackup) {
    ConsoleLogger logger;
    // 创建定时备份管理器
    TimerBackupManager backupManager(&logger);
    
    // 创建定时备份配置
    TimerBackupConfig config;
    config.sourceDir = sourceDir.string();
    config.backupDir = backupDir.string();
    config.compressEnabled = false;
    config.packageEnabled = false;
    config.intervalSeconds = 2;
    
    // 启动定时备份
    backupManager.start(config);
    
    // 等待第一次备份完成
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // 验证初始文件已被备份
    EXPECT_TRUE(fs::exists(backupDir / "initial.txt"));
    
    // 创建新文件
    std::ofstream(sourceDir / "new_file.txt") << "New file content";
    
    // 等待第二次备份完成
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // 验证新文件已被备份
    EXPECT_TRUE(fs::exists(backupDir / "new_file.txt"));
    
    // 停止定时备份
    backupManager.stop();
}

// 测试定时备份的取消功能
TEST_F(TimerBackupManagerTest, TimerBackupCancel) {
    ConsoleLogger logger;
    // 创建定时备份管理器
    TimerBackupManager backupManager(&logger);
    
    // 创建定时备份配置
    TimerBackupConfig config;
    config.sourceDir = sourceDir.string();
    config.backupDir = backupDir.string();
    config.compressEnabled = false;
    config.packageEnabled = false;
    config.intervalSeconds = 5;
    
    // 启动定时备份
    backupManager.start(config);
    
    // 立即停止备份，应该不会执行任何备份
    backupManager.stop();
    
    // 验证备份目录为空
    EXPECT_TRUE(fs::is_empty(backupDir));
}

// 测试备份引擎的基本功能
TEST(BackupEngineTest, BackupEngineBasic) {
    ConsoleLogger logger;
    fs::path testDir = fs::temp_directory_path() / "backup_engine_test";
    fs::path sourceDir = testDir / "source";
    fs::path backupDir = testDir / "backup";
    
    // 创建测试目录结构
    fs::create_directories(sourceDir);
    fs::create_directories(backupDir);
    
    // 创建测试文件
    std::ofstream(sourceDir / "test.txt") << "Test content";
    
    // 清理测试目录
    fs::remove_all(testDir);
}
