#pragma execution_character_set("utf-8")
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <limits>
#include "core/BackupEngine.hpp"
#include "utils/ConsoleLogger.hpp"

// 配置结构体定义
struct AppConfig {
    std::string sourceDir = "S:/code/backuphelper/testdata/source";
    std::string backupDir = "S:/code/backuphelper/testdata/backup";
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
        
        bool success = BackupEngine::backup(config.sourceDir, config.backupDir, &logger);
        
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
        
        bool success = BackupEngine::restore(config.backupDir, config.sourceDir, &logger);
        
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
                std::cout << "Invalid input, please enter a number [0-5]: ";
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
        std::cout << "  --backup <path> Set backup directory path\n\n";
        std::cout << "Examples:\n";
        std::cout << "  BackupHelper backup               Execute backup operation with default paths\n";
        std::cout << "  BackupHelper -r                   Execute restore operation\n";
        std::cout << "  BackupHelper --source ./data -b   Execute backup operation from specified source directory\n";
        std::cout << "  BackupHelper --backup ./backup -r Execute restore operation to specified backup directory\n";
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
        std::cout << "[5] Show Help\n";
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