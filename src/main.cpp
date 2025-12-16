#pragma execution_character_set("utf-8")
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <limits>
#include <memory>
#include "core/BackupEngine.hpp"
#include "core/Filter.hpp"
#include "utils/ConsoleLogger.hpp"

// 配置结构体定义
struct AppConfig {
    std::string sourceDir = "S:/code/backuphelper/testdata/source";
    std::string backupDir = "S:/code/backuphelper/testdata/backup";
    std::vector<std::string> excludedPaths;
    std::vector<std::string> includedExtensions;
    bool useFilters = false;
    bool compressEnabled = true;  // 压缩开关，默认为开启
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
    
    // 管理过滤规则
    virtual void manageFilters() = 0;
    
    // 显示消息
    virtual void showMessage(const std::string& message) = 0;
    
    // 显示错误
    virtual void showError(const std::string& message) = 0;
};

// 图形界面示例（未来实现）
/*
class GraphicalUserInterface : public IUserInterface {
    // 实现图形界面相关方法
};
*/

// 控制器类 - 处理业务逻辑，与具体界面实现解耦
class ApplicationController {
private:
    IUserInterface* ui;  // 使用原始指针避免循环依赖
    ConsoleLogger& logger;
    AppConfig config;

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
        
        bool success = BackupEngine::backup(config.sourceDir, config.backupDir, &logger, filters, config.compressEnabled);
        
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
        
        bool success = BackupEngine::restore(config.backupDir, config.sourceDir, &logger, filters, config.compressEnabled);
        
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
            system("cls");  // 在Windows系统清屏
            displayMenu();
            std::cout << "Please choose your operation [0-5]: ";
            
            // 输入验证
            while (!(std::cin >> choice)) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "Invalid input, please enter a number [0-7]: ";
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
        std::cout << "  -h, --help      Show this help information\n\n";
        std::cout << "Options:\n";
        std::cout << "  --source <path> Set source directory path\n";
        std::cout << "  --backup <path> Set backup directory path\n";
        std::cout << "  --compress      Enable compression for backup\n";
        std::cout << "  --no-compress   Disable compression for backup\n\n";
        std::cout << "Examples:\n";
        std::cout << "  BackupHelper backup               Execute backup operation with default paths\n";
        std::cout << "  BackupHelper -r                   Execute restore operation\n";
        std::cout << "  BackupHelper --source ./data -b   Execute backup operation from specified source directory\n";
        std::cout << "  BackupHelper --backup ./backup -r Execute restore operation to specified backup directory\n";
        std::cout << "  BackupHelper --compress -b        Execute backup with compression enabled\n";
        std::cout << "  BackupHelper --no-compress -b     Execute backup with compression disabled\n";
        waitForEnter();
    }

    void performBackup() override {
        controller.executeBackup();
        waitForEnter();
    }

    void performRestore() override {
        controller.executeRestore();
        waitForEnter();
    }

    void setSourceDirectory() override {
        AppConfig& config = controller.getConfig();
        std::string newPath;
        std::cout << "Enter new source directory path (Current: " << config.sourceDir << ", press Enter to keep unchanged): ";
        std::cin.ignore();
        std::getline(std::cin, newPath);
        if (!newPath.empty()) {
            config.sourceDir = newPath;
            std::cout << "Source directory updated to: " << config.sourceDir << "\n";
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
            config.backupDir = newPath;
            std::cout << "Backup directory updated to: " << config.backupDir << "\n";
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
            system("cls");
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
        
        std::string input;
        std::cout << "Enable compression? (y/n, default: y): ";
        std::cin.ignore();
        std::getline(std::cin, input);
        
        if (!input.empty()) {
            char choice = std::tolower(input[0]);
            config.compressEnabled = (choice == 'y' || choice == '1');
            std::cout << "Compression status updated to: " << (config.compressEnabled ? "Enabled" : "Disabled") << "\n";
        } else {
            std::cout << "Keeping current status unchanged.\n";
        }
        waitForEnter();
    }

private:
    void manageExcludedPaths(AppConfig& config) {
        int choice;
        
        do {
            system("cls");
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
            system("cls");
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
            } else if (args[i] == "--source" && i + 1 < args.size()) {
                config.sourceDir = args[++i];
            } else if (args[i] == "--backup" && i + 1 < args.size()) {
                config.backupDir = args[++i];
            } else if (args[i] == "--compress") {
                config.compressEnabled = true;
            } else if (args[i] == "--no-compress") {
                config.compressEnabled = false;
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
        std::cout << "[3] Change Source Directory (Current: " << config.sourceDir << ")\n";
        std::cout << "[4] Change Backup Directory (Current: " << config.backupDir << ")\n";
        std::cout << "[5] Manage Filters (" << (config.useFilters ? "Enabled" : "Disabled") << ")\n";
        std::cout << "[6] Toggle Compression (Current: " << (config.compressEnabled ? "Enabled" : "Disabled") << ")\n";
        std::cout << "[7] Show Help\n";
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
                setSourceDirectory();
                break;
            case 4:
                setBackupDirectory();
                break;
            case 5:
                manageFilters();
                break;
            case 6:
                setCompressEnabled();
                break;
            case 7:
                showHelp();
                break;
            case 0:
                std::cout << "Thank you for using Backup Helper, goodbye!\n";
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