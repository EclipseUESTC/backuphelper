#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <limits>
#include <memory>

// 跨平台头文件包含
#ifdef _WIN32
    // 在包含windows.h之前定义NOMINMAX宏，以避免max宏与std::max冲突
    #define NOMINMAX
    #include <windows.h> // 添加Windows API头文件
    #pragma execution_character_set("utf-8")
#else
    #include <termios.h>
    #include <unistd.h>
    #include <cstring>
#endif

#include "core/BackupEngine.hpp"
#include "core/Filter.hpp"
#include "core/RealTimeBackupManager.hpp"
#include "core/TimerBackupManager.hpp"
#include "utils/ConsoleLogger.hpp"
#include "utils/FileSystem.hpp"

// 配置结构体定义
struct AppConfig {
    std::string sourceDir = "S:/code/backuphelper/test_source";
    std::string backupDir = "S:/code/backuphelper/test_backup";
    std::vector<std::string> excludedPaths;
    std::vector<std::string> includedExtensions;
    bool useFilters = false;
    bool compressEnabled = false;  // 压缩开关，默认为开启
    bool packageEnabled = true;   // 拼接开关，默认为关闭
    std::string packageFileName = "backup.pkg"; // 拼接后的文件名
    std::string password; // 加密/解密密码
};

// 用户界面抽象接口
class IUserInterface {
public:
    virtual ~IUserInterface() = default;
    
    // 初始化界面
    virtual void initialize() = 0;
    
    // 运行界面
    virtual void run() = 0;
    
    // 显示帮助信息
    virtual void showHelp() = 0;
    
    // 执行备份
    virtual void performBackup() = 0;
    
    // 执行还原
    virtual void performRestore() = 0;
    
    // 设置源目录
    virtual void setSourceDirectory() = 0;
    
    // 设置备份目录
    virtual void setBackupDirectory() = 0;
    
    // 设置压缩开关
    virtual void setCompressEnabled() = 0;
    
    // 设置拼接开关
    virtual void setPackageEnabled() = 0;
    
    // 管理过滤规则
    virtual void manageFilters() = 0;
    
    // 显示消息
    virtual void showMessage(const std::string& message) = 0;
    
    // 显示错误
    virtual void showError(const std::string& message) = 0;
    
    // 执行重置操作
    virtual void performReset() = 0;
    
    // 设置加密密码
    virtual void setEncryptionPassword() = 0;
    
    // 删除source文件夹内的所有文件（测试用）
    virtual void deleteSourceFiles() = 0;
};

// 图形界面示例（未来实现）
/*
class GraphicalUserInterface : public IUserInterface {
    // 实现图形界面相关方法
};
*/

// 控制器类 - 处理业务逻辑，与具体界面实现解耦合
class ApplicationController {
private:
    IUserInterface* ui;  // 使用原始指针避免循环依赖
    ConsoleLogger& logger;
    AppConfig config;
    std::unique_ptr<RealTimeBackupManager> realTimeBackupManager;
    std::unique_ptr<TimerBackupManager> timerBackupManager;

public:
    ApplicationController(IUserInterface* ui, ConsoleLogger& logger)
        : ui(ui), logger(logger) {}

    // 设置用户界面（用于后期绑定）
    void setUserInterface(IUserInterface* ui) {
        this->ui = ui;
    }

    // Start application
    void start() {
        if (ui) {
            ui->initialize();
            ui->run();
        }
    }

    // 获取配置
    AppConfig& getConfig() {
        return config;
    }

    // 执行备份操作
    bool executeBackup() {
        logger.info("Backup operation started...");
        logger.info("Source directory: " + config.sourceDir);
        logger.info("Backup directory: " + config.backupDir);
        
        // 创建过滤器
        std::vector<std::shared_ptr<Filter>> filters;
        if (config.useFilters) {
            auto pathFilter = std::make_shared<PathFilter>();
            for (const auto& path : config.excludedPaths) {
                pathFilter->addExcludedPath(path);
            }
            filters.push_back(pathFilter);
            
            // 如果有扩展名过滤，这里可以添加扩展名过滤器
            // auto extensionFilter = std::make_shared<ExtensionFilter>(config.includedExtensions);
            // filters.push_back(extensionFilter);
        }
        
        bool success = BackupEngine::backup(config.sourceDir, config.backupDir, &logger, filters, config.compressEnabled, 
                                           config.packageEnabled, config.packageFileName, config.password);
        
        if (success) {
            logger.info("Backup operation completed successfully.");
            if (ui) ui->showMessage("Backup operation completed successfully");
        } else {
            logger.error("Backup operation failed.");
            if (ui) ui->showError("Backup operation failed");
        }
        
        return success;
    }

    // 执行还原操作
    bool executeRestore() {
        logger.info("Restore operation started...");
        logger.info("Backup directory: " + config.backupDir);
        logger.info("Restore directory: " + config.sourceDir);
        
        // 创建过滤器
        std::vector<std::shared_ptr<Filter>> filters;
        if (config.useFilters) {
            auto pathFilter = std::make_shared<PathFilter>();
            for (const auto& path : config.excludedPaths) {
                pathFilter->addExcludedPath(path);
            }
            filters.push_back(pathFilter);
            
            // 如果有扩展名过滤，这里可以添加扩展名过滤器
            // auto extensionFilter = std::make_shared<ExtensionFilter>(config.includedExtensions);
            // filters.push_back(extensionFilter);
        }
        
        bool success = BackupEngine::restore(config.backupDir, config.sourceDir, &logger, filters, config.compressEnabled, 
                                            config.packageEnabled, config.packageFileName, config.password);
        
        if (success) {
            logger.info("Restore operation completed successfully.");
            if (ui) ui->showMessage("Restore operation completed successfully");
        } else {
            logger.error("Restore operation failed.");
            if (ui) ui->showError("Restore operation failed");  
        }
        
        return success;
    }

    // 获取日志记录器
    ConsoleLogger& getLogger() {
        return logger;
    }
    
    // 实时备份相关方法
    bool startRealTimeBackup() {
        logger.info("Starting real-time backup...");
        
        // 检查是否已启动定时备份
        if (timerBackupManager && timerBackupManager->isRunning()) {
            logger.error("Cannot start real-time backup while timer backup is running.");
            return false;
        }
        
        // 初始化实时备份管理器
        if (!realTimeBackupManager) {
            realTimeBackupManager = std::make_unique<RealTimeBackupManager>(&logger);
        }
        
        // 创建实时备份配置
        RealTimeBackupConfig rtConfig;
        rtConfig.sourceDir = config.sourceDir;
        rtConfig.backupDir = config.backupDir;
        rtConfig.filters = {}; // 使用空过滤器，或从配置中获取
        rtConfig.compressEnabled = config.compressEnabled;
        rtConfig.packageEnabled = config.packageEnabled;
        rtConfig.packageFileName = config.packageFileName;
        rtConfig.password = config.password;
        rtConfig.debounceTimeMs = 5000; // 5秒防抖时间
        
        // 启动实时备份
        bool success = realTimeBackupManager->start(rtConfig);
        if (success) {
            logger.info("Real-time backup started successfully.");
        } else {
            logger.error("Failed to start real-time backup.");
        }
        return success;
    }
    
    void stopRealTimeBackup() {
        if (realTimeBackupManager) {
            realTimeBackupManager->stop();
            logger.info("Real-time backup stopped.");
        }
    }
    
    bool isRealTimeBackupRunning() const {
        return realTimeBackupManager && realTimeBackupManager->isRunning();
    }
    
    // 定时备份相关方法
    bool startTimerBackup(int intervalSeconds) {
        logger.info("Starting timer backup...");
        
        // 检查是否已启动实时备份
        if (realTimeBackupManager && realTimeBackupManager->isRunning()) {
            logger.error("Cannot start timer backup while real-time backup is running.");
            return false;
        }
        
        // 初始化定时备份管理器
        if (!timerBackupManager) {
            timerBackupManager = std::make_unique<TimerBackupManager>(&logger);
        }
        
        // 创建定时备份配置
        TimerBackupConfig tbConfig;
        tbConfig.sourceDir = config.sourceDir;
        tbConfig.backupDir = config.backupDir;
        tbConfig.filters = {}; // 使用空过滤器，或从配置中获取
        tbConfig.compressEnabled = config.compressEnabled;
        tbConfig.packageEnabled = config.packageEnabled;
        tbConfig.packageFileName = config.packageFileName;
        tbConfig.password = config.password;
        tbConfig.intervalSeconds = intervalSeconds;
        
        // 启动定时备份
        bool success = timerBackupManager->start(tbConfig);
        if (success) {
            logger.info("Timer backup started successfully with interval: " + std::to_string(intervalSeconds) + " seconds");
        } else {
            logger.error("Failed to start timer backup.");
        }
        return success;
    }
    
    void stopTimerBackup() {
        if (timerBackupManager) {
            timerBackupManager->stop();
            logger.info("Timer backup stopped.");
        }
    }
    
    void pauseTimerBackup() {
        if (timerBackupManager) {
            timerBackupManager->pause();
        }
    }
    
    void resumeTimerBackup() {
        if (timerBackupManager) {
            timerBackupManager->resume();
        }
    }
    
    bool isTimerBackupRunning() const {
        return timerBackupManager && timerBackupManager->isRunning();
    }
    
    bool isTimerBackupPaused() const {
        return timerBackupManager && timerBackupManager->isPaused();
    }
    
    void updateTimerBackupInterval(int seconds) {
        if (timerBackupManager) {
            timerBackupManager->setInterval(seconds);
        }
    }
    
    // 更新定时备份配置
    void updateTimerBackupConfig() {
        if (timerBackupManager && timerBackupManager->isRunning()) {
            TimerBackupConfig tbConfig;
            tbConfig.sourceDir = config.sourceDir;
            tbConfig.backupDir = config.backupDir;
            
            // 创建过滤器，与executeBackup方法保持一致
            std::vector<std::shared_ptr<Filter>> filters;
            if (config.useFilters) {
                auto pathFilter = std::make_shared<PathFilter>();
                for (const auto& path : config.excludedPaths) {
                    pathFilter->addExcludedPath(path);
                }
                filters.push_back(pathFilter);
            }
            tbConfig.filters = filters;
            
            tbConfig.compressEnabled = config.compressEnabled;
            tbConfig.packageEnabled = config.packageEnabled;
            tbConfig.packageFileName = config.packageFileName;
            tbConfig.password = config.password;
            tbConfig.intervalSeconds = timerBackupManager->getConfig().intervalSeconds; // 保留当前间隔
            
            timerBackupManager->updateConfig(tbConfig);
        }
    }
};

// 命令行界面实现 - 作为IUserInterface的具体实现
class CommandLineInterface : public IUserInterface {
private:
    ApplicationController& controller;
    int argc;
    char** argv;

public:
    CommandLineInterface(ApplicationController& controller, int argc, char** argv)
        : controller(controller), argc(argc), argv(argv) {}

    void initialize() override {
        // 命令行界面初始化
    }

    void run() override {
        // 首先尝试解析命令行参数
        if (argc > 1) {
            if (!parseArguments()) {
                return;
            }
        }

        // 如果没有有效的命令行参数，则显示交互式菜单
        int choice;
        do {
            // 跨平台清屏命令
            #ifdef _WIN32
                system("cls");
            #else
                system("clear");
            #endif
            displayMenu();
            std::cout << "Please choose your operation [0-19]: ";
            
            // 输入验证
            while (!(std::cin >> choice)) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "Invalid input, please enter a number [0-19]: ";
            }
            
            handleUserChoice(choice);
        } while (choice != 0);
    }

    void showHelp() override {
        std::cout << "=== Backup Helper Help Information ===\n";
        std::cout << "Usage: BackupHelper [options] [command]\n\n";
        std::cout << "Commands:\n";
        std::cout << "  backup, -b      Execute backup operation\n";
        std::cout << "  restore, -r     Execute restore operation\n";
        std::cout << "  reset, -rs      Reset environment: clear source and backup directories, then copy test_source to source\n";
        std::cout << "  -h, --help      Show this help information\n\n";
        std::cout << "Options:\n";
        std::cout << "  --source <path> Set source directory path\n";
        std::cout << "  --backup <path> Set backup directory path\n";
        std::cout << "  --compress      Enable compression for backup\n";
        std::cout << "  --no-compress   Disable compression for backup\n";
        std::cout << "  --package       Enable file packaging\n";
        std::cout << "  --no-package    Disable file packaging\n";
        std::cout << "  --package-name  Set package file name (default: backup.pkg)\n";
        std::cout << "  --password <pwd> Set password for encryption/decryption\n\n";
        std::cout << "Examples:\n";
        std::cout << "  BackupHelper backup               Execute backup operation with default paths\n";
        std::cout << "  BackupHelper -r                   Execute restore operation\n";
        std::cout << "  BackupHelper --source ./data -b   Execute backup operation from specified source directory\n";
        std::cout << "  BackupHelper --backup ./backup -r Execute restore operation to specified backup directory\n";
        std::cout << "  BackupHelper --compress -b        Execute backup with compression enabled\n";
        std::cout << "  BackupHelper --no-compress -b     Execute backup with compression disabled\n";
        std::cout << "  BackupHelper --package -b         Execute backup with file packaging enabled\n";
        std::cout << "  BackupHelper --compress --package -b Execute backup with both compression and packaging enabled\n";
        std::cout << "  BackupHelper --package-name mybackup.pkg -b Execute backup with custom package name\n";
        std::cout << "  BackupHelper --password mysecret -b Execute backup with encryption enabled\n";
        std::cout << "  BackupHelper --password mysecret -r Execute restore with decryption\n";
        waitForEnter();
    }

    void performBackup() override {
        controller.executeBackup();
        waitForEnter();
    }

    void performRestore() override {
        // 检查是否有加密的备份文件
        bool hasEncryptedFiles = false;
        AppConfig& config = controller.getConfig();
        
        // 扫描备份目录，检查是否有.enc文件
        auto files = FileSystem::getAllFiles(config.backupDir);
        for (const auto& file : files) {
            std::string filePath = file.getFilePath().string();
            if (filePath.size() > 4 && filePath.substr(filePath.size() - 4) == ".enc") {
                hasEncryptedFiles = true;
                std::cout << "Detected encrypted document. Current Password:" << std::endl;
                break;
            }
        }
        
        // 如果有加密文件且配置中没有密码，则提示用户输入
        while (hasEncryptedFiles && !config.password.empty()) {
            std::cout << "\nEncrypted backup files found. Enter password: ";
            
            std::string tempPassword;
            // 清除输入缓冲区中的换行符
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            
            // 跨平台密码输入（无回显）
            #ifdef _WIN32
                // Windows实现
                HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);
                DWORD mode;
                GetConsoleMode(hConsole, &mode);
                SetConsoleMode(hConsole, mode & ~ENABLE_ECHO_INPUT);
                
                std::getline(std::cin, tempPassword);
                
                // 恢复控制台模式
                SetConsoleMode(hConsole, mode);
            #else
                // Linux/Unix实现
                struct termios oldt, newt;
                tcgetattr(STDIN_FILENO, &oldt);
                newt = oldt;
                newt.c_lflag &= ~(ECHO);
                tcsetattr(STDIN_FILENO, TCSANOW, &newt);
                
                std::getline(std::cin, tempPassword);
                
                // 恢复终端设置
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            #endif
            
            std::cout << "\n";
            
            // 临时设置密码
            if (tempPassword == config.password) {
                std::cout << "password match, restore continues in a second." << std::endl;
                break;
            };
        }
        
        controller.executeRestore();
        waitForEnter();
    }
    
    void performReset() override {
        // 要复制的源目录
        std::string testSourceDir = "./test_source";
        
        // 获取当前配置
        AppConfig& config = controller.getConfig();
        
        // 验证源目录存在
        std::cout << "Checking if test_source directory exists: " << testSourceDir << "...\n";
        if (!FileSystem::exists(testSourceDir)) {
            std::cout << "ERROR: test_source directory does not exist: " << testSourceDir << std::endl;
            waitForEnter();
            return;
        }
        
        // 1. 清空backup目录
        std::cout << "Clearing backup directory: " << config.backupDir << "...\n";
        if (FileSystem::clearDirectory(config.backupDir)) {
            std::cout << "Backup directory cleared successfully\n";
        } else {
            std::cout << "Failed to clear backup directory\n";
        }
        
        // 2. 清空source目录
        std::cout << "Clearing source directory: " << config.sourceDir << "...\n";
        if (FileSystem::clearDirectory(config.sourceDir)) {
            std::cout << "Source directory cleared successfully\n";
        } else {
            std::cout << "Failed to clear source directory\n";
        }
        
        // 3. 将test_source目录内容复制到source目录
        std::cout << "Copying contents of " << testSourceDir << " to " << config.sourceDir << "...\n";
        if (FileSystem::copyDirectory(testSourceDir, config.sourceDir)) {
            std::cout << "Contents copied successfully\n";
        } else {
            std::cout << "Failed to copy contents from test_source\n";
        }
        
        waitForEnter();
    }

    void setSourceDirectory() override {
        AppConfig& config = controller.getConfig();
        std::string newPath;
        std::cout << "Enter new source directory path (Current: " << config.sourceDir << ", press Enter to keep unchanged): ";
        std::cin.ignore();
        std::getline(std::cin, newPath);
        if (!newPath.empty()) {
            // 验证路径是否存在或可创建
            if (FileSystem::exists(newPath) || FileSystem::createDirectories(newPath)) {
                config.sourceDir = newPath;
                std::cout << "Source directory updated to: " << config.sourceDir << "\n";
                // 更新运行中的备份配置
                controller.updateTimerBackupConfig();
            } else {
                std::cout << "Warning: Path does not exist and cannot be created: " << newPath << "\n";
                std::cout << "Please check if you have permission to access this location.\n";
                std::cout << "Source directory remains unchanged: " << config.sourceDir << "\n";
            }
        }
        waitForEnter();
    }

    void setBackupDirectory() override {
        AppConfig& config = controller.getConfig();
        std::string newPath;
        std::cout << "Enter new backup directory path (Current: " << config.backupDir << ", press Enter to keep unchanged): ";
        std::cin.ignore();
        std::getline(std::cin, newPath);
        if (!newPath.empty()) {
            // 验证路径是否存在或可创建
            if (FileSystem::exists(newPath) || FileSystem::createDirectories(newPath)) {
                config.backupDir = newPath;
                std::cout << "Backup directory updated to: " << config.backupDir << "\n";
                // 更新运行中的备份配置
                controller.updateTimerBackupConfig();
            } else {
                std::cout << "Warning: Path does not exist and cannot be created: " << newPath << "\n";
                std::cout << "Please check if you have permission to access this location.\n";
                std::cout << "Backup directory remains unchanged: " << config.backupDir << "\n";
            }
        }
        waitForEnter();
    }

    void showMessage(const std::string& message) override {
        std::cout << "[Info] " << message << "\n";
    }

    void showError(const std::string& message) override {
        std::cout << "[Error] " << message << "\n";
    }

    void manageFilters() override {
        AppConfig& config = controller.getConfig();
        int choice;
        
        do {
            // 跨平台清屏命令
            #ifdef _WIN32
                system("cls");
            #else
                system("clear");
            #endif
            std::cout << "=== Filter Management ===\n";
            std::cout << "Filter Status: " << (config.useFilters ? "Enabled" : "Disabled") << "\n\n";
            std::cout << "[1] Toggle Filter Status\n";
            std::cout << "[2] Manage Excluded Paths\n";
            std::cout << "[3] Manage Included Extensions\n";
            std::cout << "[0] Back to Main Menu\n";
            std::cout << "Please choose an operation [0-3]: ";
            
            while (!(std::cin >> choice)) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "Invalid input, please enter a number [0-3]: ";
            }
            
            switch (choice) {
                case 1:
                    config.useFilters = !config.useFilters;
                    std::cout << "Filters " << (config.useFilters ? "enabled" : "disabled") << ".\n";
                    waitForEnter();
                    break;
                case 2:
                    manageExcludedPaths(config);
                    break;
                case 3:
                    manageIncludedExtensions(config);
                    break;
                case 0:
                    break;
                default:
                    std::cout << "Invalid selection, please try again.\n";
                    waitForEnter();
            }
        } while (choice != 0);
    }

    void setCompressEnabled() override {
        AppConfig& config = controller.getConfig();
        std::cout << "\n=== Compression Settings ===\n";
        std::cout << "Current status: " << (config.compressEnabled ? "Enabled" : "Disabled") << "\n";
        
        // 直接切换状态，无需询问
        config.compressEnabled = !config.compressEnabled;
        std::cout << "Compression status updated to: " << (config.compressEnabled ? "Enabled" : "Disabled") << "\n";
        
        // 更新运行中的备份配置
        controller.updateTimerBackupConfig();
        
        waitForEnter();
    }

    void setPackageEnabled() override {
        AppConfig& config = controller.getConfig();
        std::cout << "\n=== File Packaging Settings ===\n";
        std::cout << "Current status: " << (config.packageEnabled ? "Enabled" : "Disabled") << "\n";
        
        // 直接切换状态，无需询问
        config.packageEnabled = !config.packageEnabled;
        std::cout << "File packaging status updated to: " << (config.packageEnabled ? "Enabled" : "Disabled") << "\n";
        
        if (config.packageEnabled) {
            // 如果启用了打包，可以选择是否修改包文件名
            std::cout << "Package file name (current: " << config.packageFileName << ", press Enter to keep): ";
            std::string input;
            std::cin.ignore();
            std::getline(std::cin, input);
            if (!input.empty()) {
                config.packageFileName = input;
                std::cout << "Package file name updated to: " << config.packageFileName << "\n";
            }
        }
        
        // 更新运行中的备份配置
        controller.updateTimerBackupConfig();
        
        waitForEnter();
    }

private:
    void manageExcludedPaths(AppConfig& config) {
        int choice;
        
        do {
            // 跨平台清屏命令
            #ifdef _WIN32
                system("cls");
            #else
                system("clear");
            #endif
            std::cout << "=== Excluded Paths Management ===\n\n";
            if (config.excludedPaths.empty()) {
                std::cout << "No excluded paths defined.\n\n";
            } else {
                std::cout << "Current excluded paths:\n";
                for (size_t i = 0; i < config.excludedPaths.size(); ++i) {
                    std::cout << "[" << (i + 1) << "] " << config.excludedPaths[i] << "\n";
                }
                std::cout << "\n";
            }
            
            std::cout << "[1] Add excluded path\n";
            std::cout << "[2] Remove excluded path\n";
            std::cout << "[0] Back to Filter Menu\n";
            std::cout << "Please choose an operation [0-2]: ";
            
            while (!(std::cin >> choice)) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "Invalid input, please enter a number [0-2]: ";
            }
            
            switch (choice) {
                case 1: {
                    std::string path;
                    std::cout << "Enter path to exclude: ";
                    std::cin.ignore();
                    std::getline(std::cin, path);
                    config.excludedPaths.push_back(path);
                    std::cout << "Path added to excluded list.\n";
                    waitForEnter();
                    break;
                }
                case 2: {
                    if (config.excludedPaths.empty()) {
                        std::cout << "No excluded paths to remove.\n";
                        waitForEnter();
                        break;
                    }
                    
                    size_t index;
                    std::cout << "Enter index of path to remove (1-" << config.excludedPaths.size() << "): ";
                    while (!(std::cin >> index) || index < 1 || index > config.excludedPaths.size()) {
                        std::cin.clear();
                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                        std::cout << "Invalid input, please enter a number between 1 and " << config.excludedPaths.size() << ": ";
                    }
                    
                    config.excludedPaths.erase(config.excludedPaths.begin() + index - 1);
                    std::cout << "Path removed from excluded list.\n";
                    waitForEnter();
                    break;
                }
                case 0:
                    break;
                default:
                    std::cout << "Invalid selection, please try again.\n";
                    waitForEnter();
            }
        } while (choice != 0);
    }

    void manageIncludedExtensions(AppConfig& config) {
        int choice;
        
        do {
            // 跨平台清屏命令
            #ifdef _WIN32
                system("cls");
            #else
                system("clear");
            #endif
            std::cout << "=== Included Extensions Management ===\n\n";
            if (config.includedExtensions.empty()) {
                std::cout << "No included extensions defined (all files included).\n\n";
            } else {
                std::cout << "Current included extensions:\n";
                for (size_t i = 0; i < config.includedExtensions.size(); ++i) {
                    std::cout << "[" << (i + 1) << "] " << config.includedExtensions[i] << "\n";
                }
                std::cout << "\n";
            }
            
            std::cout << "[1] Add included extension\n";
            std::cout << "[2] Remove included extension\n";
            std::cout << "[0] Back to Filter Menu\n";
            std::cout << "Please choose an operation [0-2]: ";
            
            while (!(std::cin >> choice)) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "Invalid input, please enter a number [0-2]: ";
            }
            
            switch (choice) {
                case 1: {
                    std::string ext;
                    std::cout << "Enter extension to include (e.g., .txt): ";
                    std::cin.ignore();
                    std::getline(std::cin, ext);
                    config.includedExtensions.push_back(ext);
                    std::cout << "Extension added to included list.\n";
                    waitForEnter();
                    break;
                }
                case 2: {
                    if (config.includedExtensions.empty()) {
                        std::cout << "No included extensions to remove.\n";
                        waitForEnter();
                        break;
                    }
                    
                    size_t index;
                    std::cout << "Enter index of extension to remove (1-" << config.includedExtensions.size() << "): ";
                    while (!(std::cin >> index) || index < 1 || index > config.includedExtensions.size()) {
                        std::cin.clear();
                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                        std::cout << "Invalid input, please enter a number between 1 and " << config.includedExtensions.size() << ": ";
                    }
                    
                    config.includedExtensions.erase(config.includedExtensions.begin() + index - 1);
                    std::cout << "Extension removed from included list.\n";
                    waitForEnter();
                    break;
                }
                case 0:
                    break;
                default:
                    std::cout << "Invalid selection, please try again.\n";
                    waitForEnter();
            }
        } while (choice != 0);
    }

private:
    // Parse command line arguments
    bool parseArguments() {
        AppConfig& config = controller.getConfig();
        std::vector<std::string> args(argv + 1, argv + argc);

        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i] == "-h" || args[i] == "--help") {
                showHelp();
                return false;
            } else if (args[i] == "backup" || args[i] == "-b") {
                performBackup();
                return false;
            } else if (args[i] == "restore" || args[i] == "-r") {
                performRestore();
                return false;
            } else if (args[i] == "reset" || args[i] == "-rs") {
                performReset();
                return false;
            } else if (args[i] == "--source" && i + 1 < args.size()) {
                config.sourceDir = args[++i];
            } else if (args[i] == "--backup" && i + 1 < args.size()) {
                config.backupDir = args[++i];
            } else if (args[i] == "--compress") {
                config.compressEnabled = true;
            } else if (args[i] == "--no-compress") {
                config.compressEnabled = false;
            } else if (args[i] == "--package") {
                config.packageEnabled = true;
            } else if (args[i] == "--no-package") {
                config.packageEnabled = false;
            } else if (args[i] == "--package-name" && i + 1 < args.size()) {
                config.packageFileName = args[++i];
            } else if (args[i] == "--password" && i + 1 < args.size()) {
                config.password = args[++i];
            }
        }
        return true;
    }

    // Display interactive menu
    void displayMenu() {
        AppConfig& config = controller.getConfig();
        std::cout << "=== Backup Helper ===\n";
        std::cout << "[1] Perform Backup\n";
        std::cout << "[2] Perform Restore\n";
        std::cout << "[3] Start Real-Time Backup (Current: " << (controller.isRealTimeBackupRunning() ? "Running" : "Stopped") << ")\n";
        std::cout << "[4] Stop Real-Time Backup\n";
        std::cout << "[5] Start Timer Backup\n";
        std::cout << "[6] Stop Timer Backup (Current: " << (controller.isTimerBackupRunning() ? (controller.isTimerBackupPaused() ? "Paused" : "Running") : "Stopped") << ")\n";
        std::cout << "[7] Pause/Resume Timer Backup\n";
        std::cout << "[8] Change Source Directory (Current: " << config.sourceDir << ")\n";
        std::cout << "[9] Change Backup Directory (Current: " << config.backupDir << ")\n";
        std::cout << "[10] Manage Filters (" << (config.useFilters ? "Enabled" : "Disabled") << ")\n";
        std::cout << "[11] Toggle Compression (Current: " << (config.compressEnabled ? "Enabled" : "Disabled") << ")\n";
        std::cout << "[12] Toggle File Packaging (Current: " << (config.packageEnabled ? "Enabled" : "Disabled") << ")\n";
        std::cout << "[13] Set Encryption Password (Current: " << (config.password.empty() ? "Not Set" : "Set") << ")\n";
        std::cout << "[14] Show Help\n";
        std::cout << "[15] Reset Environment\n";
        std::cout << "[16] Delete Source Files (Test)\n";
        std::cout << "[17] Set Source to /home/huang-nan/backup_test\n";
        std::cout << "[18] Set Source to /home/huang-nan/backup_source\n";
        std::cout << "[19] Delete All Files in Backup Directory\n";
        std::cout << "[0] Exit Program\n";
    }

    // Handle user selection
    void handleUserChoice(int choice) {
        switch (choice) {
            case 1:
                performBackup();
                break;
            case 2:
                performRestore();
                break;
            case 3:
                {
                    std::cout << "=== Start Real-Time Backup ===\n";
                    if (controller.startRealTimeBackup()) {
                        std::cout << "Real-time backup started successfully.\n";
                        std::cout << "Monitoring directory: " << controller.getConfig().sourceDir << "\n";
                    } else {
                        std::cout << "Failed to start real-time backup.\n";
                    }
                    waitForEnter();
                }
                break;
            case 4:
                {
                    std::cout << "=== Stop Real-Time Backup ===\n";
                    controller.stopRealTimeBackup();
                    std::cout << "Real-time backup stopped.\n";
                    waitForEnter();
                }
                break;
            case 5:
                {
                    std::cout << "=== Start Timer Backup ===\n";
                    std::cout << "Enter backup interval in seconds: ";
                    int interval;
                    std::cin >> interval;
                    
                    // 清空输入缓冲区中的换行符，避免影响定时器备份的输入检测
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    
                    if (interval > 0) {
                        if (controller.startTimerBackup(interval)) {
                            std::cout << "Timer backup started with interval: " << interval << " seconds.\n";
                        } else {
                            std::cout << "Failed to start timer backup.\n";
                        }
                    } else {
                        std::cout << "Invalid interval. Please enter a positive number.\n";
                    }
                    waitForEnter();
                }
                break;
            case 6:
                {
                    std::cout << "=== Stop Timer Backup ===\n";
                    if (controller.isTimerBackupRunning()) {
                        controller.stopTimerBackup();
                        std::cout << "Timer backup stopped.\n";
                    } else {
                        std::cout << "Timer backup is not running.\n";
                    }
                    waitForEnter();
                }
                break;
            case 7:
                {
                    std::cout << "=== Pause/Resume Timer Backup ===\n";
                    if (controller.isTimerBackupRunning()) {
                        if (controller.isTimerBackupPaused()) {
                            controller.resumeTimerBackup();
                            std::cout << "Timer backup resumed.\n";
                        } else {
                            controller.pauseTimerBackup();
                            std::cout << "Timer backup paused.\n";
                        }
                    } else {
                        std::cout << "Timer backup is not running.\n";
                    }
                    waitForEnter();
                }
                break;
            case 8:
                setSourceDirectory();
                break;
            case 9:
                setBackupDirectory();
                break;
            case 10:
                manageFilters();
                break;
            case 11:
                setCompressEnabled();
                break;
            case 12:
                setPackageEnabled();
                break;
            case 13:
                setEncryptionPassword();
                break;
            case 14:
                showHelp();
                break;
            case 15:
                performReset();
                break;
            case 16:
                deleteSourceFiles();
                break;
            case 17:
                {
                    AppConfig& config = controller.getConfig();
                    config.sourceDir = "/home/huang-nan/backup_test";
                    std::cout << "Source directory set to: " << config.sourceDir << "\n";
                    waitForEnter();
                }
                break;
            case 18:
                {
                    AppConfig& config = controller.getConfig();
                    config.sourceDir = "/home/huang-nan/backup_source";
                    std::cout << "Source directory set to: " << config.sourceDir << "\n";
                    waitForEnter();
                }
                break;
            case 19:
                {
                    AppConfig& config = controller.getConfig();
                    std::cout << "Deleting all files in backup directory: " << config.backupDir << "\n";
                    if (FileSystem::clearDirectory(config.backupDir)) {
                        std::cout << "All files in backup directory have been deleted successfully.\n";
                    } else {
                        std::cout << "Failed to delete files in backup directory.\n";
                    }
                    waitForEnter();
                }
                break;
            case 0:
                std::cout << "Thank you for using Backup Helper, goodbye!\n";
                // 确保在退出前停止所有备份
                controller.stopRealTimeBackup();
                controller.stopTimerBackup();
                break;
            default:
                std::cout << "Invalid selection, please try again.\n";
                waitForEnter();
        }
    }

    // Wait for user to press Enter
    void waitForEnter() {
        std::cout << "\nPress Enter to continue...";
        std::cin.ignore();
        std::cin.get();
    }

    // Set encryption password
    void setEncryptionPassword() override {
        AppConfig& config = controller.getConfig(); // 添加config变量的获取
        std::cout << "=== Set Encryption Password ===\n";
        std::cout << "Current password: " << (config.password.empty() ? "Not Set" : "Set") << "\n";
        
        std::cout << "Enter new password (press Enter to remove password): ";
        std::string newPassword;
        
        // 清除输入缓冲区中的换行符
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        
        // 跨平台密码输入（无回显）
        #ifdef _WIN32
            // Windows实现
            HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);
            DWORD mode;
            GetConsoleMode(hConsole, &mode);
            SetConsoleMode(hConsole, mode & ~ENABLE_ECHO_INPUT);
            
            std::getline(std::cin, newPassword);
            
            // 恢复控制台模式
            SetConsoleMode(hConsole, mode);
        #else
            // Linux/Unix实现
            struct termios oldt, newt;
            tcgetattr(STDIN_FILENO, &oldt);
            newt = oldt;
            newt.c_lflag &= ~(ECHO);
            tcsetattr(STDIN_FILENO, TCSANOW, &newt);
            
            std::getline(std::cin, newPassword);
            
            // 恢复终端设置
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        #endif
        
        std::cout << "\n";
        
        config.password = newPassword;
        
        if (config.password.empty()) {
            std::cout << "Password removed successfully.\n";
        } else {
            std::cout << "Password set successfully.\n";
        }
        
        // 更新运行中的备份配置
        controller.updateTimerBackupConfig();
        
        waitForEnter();
    }
    
    void deleteSourceFiles() override {
        AppConfig& config = controller.getConfig();
        std::cout << "=== Delete Source Files (Test) ===\n";
        std::cout << "This will delete ALL files in the source directory: " << config.sourceDir << "\n";
        std::cout << "WARNING: This operation cannot be undone!\n";
        
        std::string confirm;
        std::cout << "Type 'DELETE' to confirm deletion: ";
        std::cin >> confirm;
        
        if (confirm == "DELETE") {
            // 使用FileSystem类的clearDirectory方法删除source目录中的所有文件
            if (FileSystem::clearDirectory(config.sourceDir)) {
                std::cout << "All files in source directory have been deleted successfully.\n";
            } else {
                std::cout << "Failed to delete files in source directory.\n";
            }
        } else {
            std::cout << "Deletion cancelled.\n";
        }
        
        waitForEnter();
    }
};

int main(int argc, char* argv[]) {
    ConsoleLogger logger;
    
    // Using correct initialization order to avoid circular dependency
    // 1. First create controller with null interface pointer
    ApplicationController controller(nullptr, logger);
    
    // 2. Create command line interface and pass controller reference
    CommandLineInterface cli(controller, argc, argv);
    
    // 3. Set interface to controller
    controller.setUserInterface(&cli);
    
    // 启动应用
    controller.start();

    return 0;
}