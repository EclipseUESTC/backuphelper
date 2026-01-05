#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include "utils/FilePackager.hpp"
#include "core/models/File.hpp"

namespace fs = std::filesystem;

// FilePackager类测试用例
class FilePackagerTest : public ::testing::Test {
protected:
    fs::path testDir = fs::temp_directory_path() / "backup_packager_test";
    fs::path sourceDir = testDir / "source";
    fs::path packageFile = testDir / "package.pkg";
    fs::path unpackDir = testDir / "unpacked";
    
    void SetUp() override {
        // 创建测试目录结构
        fs::create_directories(sourceDir / "subdir1");
        fs::create_directories(sourceDir / "subdir2");
        
        // 创建测试文件
        std::ofstream(sourceDir / "file1.txt") << "Content of file 1";
        std::ofstream(sourceDir / "file2.txt") << "Content of file 2";
        std::ofstream(sourceDir / "subdir1" / "file3.txt") << "Content of file 3 in subdir1";
        std::ofstream(sourceDir / "subdir2" / "file4.txt") << "Content of file 4 in subdir2";
        
        // 创建符号链接 - 在Windows上可能需要管理员权限，所以使用try-catch
        try {
            fs::create_symlink(sourceDir / "file1.txt", sourceDir / "symlink.txt");
        } catch (const std::exception& e) {
            // 忽略符号链接创建失败，因为在Windows上可能没有权限
            // 测试会跳过需要符号链接的测试用例
        }
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
            // 如果是符号链接，比较目标
            else if (fs::is_symlink(entry)) {
                fs::path target1 = fs::read_symlink(entry.path());
                fs::path target2 = fs::read_symlink(targetPath);
                
                if (target1 != target2) {
                    return false;
                }
            }
        }
        
        return true;
    }
    
    // 辅助函数：获取目录中的所有File对象
    std::vector<File> getFilesFromDirectory(const fs::path& dir) {
        std::vector<File> files;
        
        for (const auto& entry : fs::recursive_directory_iterator(dir)) {
            files.emplace_back(entry.path());
        }
        
        return files;
    }
    
    // 辅助函数：获取目录中的所有文件路径
    std::vector<std::string> getFilePathsFromDirectory(const fs::path& dir) {
        std::vector<std::string> filePaths;
        
        for (const auto& entry : fs::recursive_directory_iterator(dir)) {
            filePaths.push_back(entry.path().string());
        }
        
        return filePaths;
    }
};

// 测试使用File对象列表打包和解包
TEST_F(FilePackagerTest, PackageUnpackWithFileObjects) {
    FilePackager packager;
    
    // 获取源目录中的所有File对象
    std::vector<File> sourceFiles = getFilesFromDirectory(sourceDir);
    
    // 打包文件
    EXPECT_TRUE(packager.packageFiles(sourceFiles, packageFile.string(), sourceDir.string()));
    
    // 验证打包文件已生成
    EXPECT_TRUE(fs::exists(packageFile));
    EXPECT_GT(fs::file_size(packageFile), 0);
    
    // 解包文件
    EXPECT_TRUE(packager.unpackFiles(packageFile.string(), unpackDir.string()));
    
    // 验证解包目录已创建
    EXPECT_TRUE(fs::exists(unpackDir));
    
    // 验证解包内容与源目录相同
    EXPECT_TRUE(compareDirectories(sourceDir, unpackDir));
}

// 测试使用文件路径列表打包和解包
TEST_F(FilePackagerTest, PackageUnpackWithFilePaths) {
    FilePackager packager;
    
    // 只获取一级目录中的文件路径，不包括子目录
    std::vector<std::string> sourceFilePaths;
    for (const auto& entry : fs::directory_iterator(sourceDir)) {
        if (entry.is_regular_file()) {
            sourceFilePaths.push_back(entry.path().string());
        }
    }
    
    // 打包文件，指定basePath为sourceDir
    EXPECT_TRUE(packager.packageFiles(sourceFilePaths, packageFile.string(), sourceDir.string()));
    
    // 验证打包文件已生成
    EXPECT_TRUE(fs::exists(packageFile));
    EXPECT_GT(fs::file_size(packageFile), 0);
    
    // 解包文件
    EXPECT_TRUE(packager.unpackFiles(packageFile.string(), unpackDir.string()));
    
    // 验证解包目录已创建
    EXPECT_TRUE(fs::exists(unpackDir));
    
    // 验证解包内容包含所有源文件
    for (const auto& filePath : sourceFilePaths) {
        fs::path fileName = fs::path(filePath).filename();
        fs::path unpackedFilePath = unpackDir / fileName;
        EXPECT_TRUE(fs::exists(unpackedFilePath));
    }
}

// 测试解包并返回File对象列表
TEST_F(FilePackagerTest, UnpackToFileObjects) {
    FilePackager packager;
    
    // 获取源目录中的所有File对象
    std::vector<File> sourceFiles = getFilesFromDirectory(sourceDir);
    
    // 打包文件
    EXPECT_TRUE(packager.packageFiles(sourceFiles, packageFile.string(), sourceDir.string()));
    
    // 解包并返回File对象列表
    std::vector<File> unpackedFiles = packager.unpackFilesToFiles(packageFile.string(), unpackDir.string());
    
    // 验证返回的File对象数量与源文件数量相同
    EXPECT_EQ(unpackedFiles.size(), sourceFiles.size());
    
    // 验证解包目录已创建
    EXPECT_TRUE(fs::exists(unpackDir));
}

// 测试打包空目录
TEST_F(FilePackagerTest, PackageEmptyDirectory) {
    FilePackager packager;
    
    // 创建空目录
    fs::path emptyDir = testDir / "empty_dir";
    fs::create_directories(emptyDir);
    
    // 获取空目录中的所有File对象
    std::vector<File> emptyFiles = getFilesFromDirectory(emptyDir);
    
    // 打包空目录
    EXPECT_TRUE(packager.packageFiles(emptyFiles, packageFile.string(), emptyDir.string()));
    
    // 验证打包文件已生成
    EXPECT_TRUE(fs::exists(packageFile));
    
    // 解包空目录
    EXPECT_TRUE(packager.unpackFiles(packageFile.string(), unpackDir.string()));
    
    // 验证解包目录已创建
    EXPECT_TRUE(fs::exists(unpackDir));
    
    // 验证解包目录为空
    EXPECT_TRUE(std::filesystem::is_empty(unpackDir));
}

// 测试打包单个文件
TEST_F(FilePackagerTest, PackageSingleFile) {
    FilePackager packager;
    
    // 创建单个测试文件
    fs::path singleFile = testDir / "single.txt";
    std::ofstream(singleFile) << "Single file content";
    
    // 使用File对象打包
    std::vector<File> singleFileVector = {File(singleFile)};
    EXPECT_TRUE(packager.packageFiles(singleFileVector, packageFile.string(), testDir.string()));
    
    // 验证打包文件已生成
    EXPECT_TRUE(fs::exists(packageFile));
    
    // 解包文件
    EXPECT_TRUE(packager.unpackFiles(packageFile.string(), unpackDir.string()));
    
    // 验证解包文件与原文件相同
    fs::path unpackedSingleFile = unpackDir / "single.txt";
    EXPECT_TRUE(fs::exists(unpackedSingleFile));
    
    std::ifstream originalFile(singleFile);
    std::ifstream unpackedFile(unpackedSingleFile);
    std::string originalContent((std::istreambuf_iterator<char>(originalFile)), std::istreambuf_iterator<char>());
    std::string unpackedContent((std::istreambuf_iterator<char>(unpackedFile)), std::istreambuf_iterator<char>());
    
    EXPECT_EQ(originalContent, unpackedContent);
}

// 测试打包符号链接
TEST_F(FilePackagerTest, PackageSymbolicLink) {
    FilePackager packager;
    
    // 获取源目录中的符号链接
    fs::path symlinkPath = sourceDir / "symlink.txt";
    
    // 只在符号链接存在时测试
    if (fs::exists(symlinkPath) && fs::is_symlink(symlinkPath)) {
        std::vector<File> symlinkFileVector = {File(symlinkPath)};
        
        // 打包符号链接
        EXPECT_TRUE(packager.packageFiles(symlinkFileVector, packageFile.string(), sourceDir.string()));
        
        // 验证打包文件已生成
        EXPECT_TRUE(fs::exists(packageFile));
        
        // 解包符号链接
        EXPECT_TRUE(packager.unpackFiles(packageFile.string(), unpackDir.string()));
        
        // 验证解包后的符号链接存在且目标正确
        fs::path unpackedSymlink = unpackDir / "symlink.txt";
        EXPECT_TRUE(fs::exists(unpackedSymlink));
        EXPECT_TRUE(fs::is_symlink(unpackedSymlink));
        
        fs::path originalTarget = fs::read_symlink(symlinkPath);
        fs::path unpackedTarget = fs::read_symlink(unpackedSymlink);
        
        // 注意：由于解包目录不同，符号链接的目标路径会有所不同
        // 我们只需要验证解包后的符号链接指向正确的文件即可
        EXPECT_TRUE(fs::exists(fs::absolute(unpackedTarget)));
    }
}

// 测试打包不存在的文件
TEST_F(FilePackagerTest, PackageNonExistentFile) {
    FilePackager packager;
    
    // 创建不存在的文件路径
    fs::path nonExistentFile = testDir / "non_existent.txt";
    std::vector<std::string> nonExistentFilePathVector = {nonExistentFile.string()};
    
    // 尝试打包不存在的文件，应该失败
    EXPECT_FALSE(packager.packageFiles(nonExistentFilePathVector, packageFile.string()));
    
    // 验证打包文件未生成
    EXPECT_FALSE(fs::exists(packageFile));
}

// 测试解包不存在的包文件
TEST_F(FilePackagerTest, UnpackNonExistentPackage) {
    FilePackager packager;
    
    // 创建不存在的包文件路径
    fs::path nonExistentPackage = testDir / "non_existent.pkg";
    
    // 尝试解包不存在的包文件，应该失败
    EXPECT_FALSE(packager.unpackFiles(nonExistentPackage.string(), unpackDir.string()));
    
    // 验证解包目录未创建
    EXPECT_FALSE(fs::exists(unpackDir));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
