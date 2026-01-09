cmake_minimum_required(VERSION 3.16)
project(BackupHelper)

# === 强制启用UTF-8编码支持 ===
if (MSVC)
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    message(STATUS "Enabled MSVC UTF-8 code compile")
else()  
    add_compile_options(-finput-charset=UTF-8)
    message(STATUS "Enabled GCC/Clang UTF-8 code compile")
endif()

# === 打包配置 ===
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# === 设置 C++ 标准 ===
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# === Google Test 配置 ===
# 使用 FetchContent 自动下载 GoogleTest（推荐方式）
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# 只在 Windows 上设置这个选项
if(WIN32)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

# 下载 GoogleTest
FetchContent_MakeAvailable(googletest)

# 诊断信息 - 修复后的版本
message(STATUS "=== Google Test 诊断信息 ===")

# 检查 FetchContent 提供的目标
if(TARGET gtest)
    message(STATUS "✓ 找到 FetchContent 目标: gtest")
endif()

if(TARGET gtest_main)
    message(STATUS "✓ 找到 FetchContent 目标: gtest_main")
endif()

if(TARGET gmock)
    message(STATUS "✓ 找到 FetchContent 目标: gmock")
endif()

if(TARGET gmock_main)
    message(STATUS "✓ 找到 FetchContent 目标: gmock_main")
endif()

# 注意：不要使用 find_package(GTest REQUIRED)，这会和 FetchContent 冲突

# === 测试目标配置 ===
# 创建测试目标列表
set(TEST_TARGETS
    FilterTests
    FileTests
    EncryptionTests
    FilePackagerTests
    HuffmanCompressorTests
    BackupManagerTests
    TaskTests
)

# FilterTests
add_executable(FilterTests 
    src/FilterTests.cpp
    src/core/Filter.cpp 
    src/core/models/File.cpp 
    src/utils/ConsoleLogger.cpp 
    src/utils/FileSystem.cpp
    src/utils/HuffmanCompressor.cpp
    src/utils/FileSystemMonitor.cpp
)
target_include_directories(FilterTests PRIVATE ${CMAKE_SOURCE_DIR}/src)

# FileTests
add_executable(FileTests 
    src/FileTests.cpp
    src/core/models/File.cpp 
    src/utils/FileSystem.cpp
    src/utils/HuffmanCompressor.cpp
)
target_include_directories(FileTests PRIVATE ${CMAKE_SOURCE_DIR}/src)

# EncryptionTests
add_executable(EncryptionTests 
    src/EncryptionTests.cpp
    src/utils/Encryption.cpp 
    src/utils/FileSystem.cpp
    src/utils/HuffmanCompressor.cpp
    src/core/models/File.cpp
)
target_include_directories(EncryptionTests PRIVATE ${CMAKE_SOURCE_DIR}/src)

# FilePackagerTests
add_executable(FilePackagerTests 
    src/FilePackagerTests.cpp
    src/utils/FilePackager.cpp 
    src/core/models/File.cpp 
    src/utils/FileSystem.cpp
    src/utils/HuffmanCompressor.cpp
)
target_include_directories(FilePackagerTests PRIVATE ${CMAKE_SOURCE_DIR}/src)

# HuffmanCompressorTests
add_executable(HuffmanCompressorTests 
    src/HuffmanCompressorTests.cpp
    src/utils/HuffmanCompressor.cpp
    src/utils/FileSystem.cpp
    src/core/models/File.cpp
)
target_include_directories(HuffmanCompressorTests PRIVATE ${CMAKE_SOURCE_DIR}/src)

# BackupManagerTests
add_executable(BackupManagerTests 
    src/BackupManagerTests.cpp
    src/core/RealTimeBackupManager.cpp
    src/core/TimerBackupManager.cpp
    src/core/BackupEngine.cpp
    src/core/models/File.cpp
    src/utils/ConsoleLogger.cpp
    src/utils/FileSystem.cpp
    src/utils/FileSystemMonitor.cpp
    src/utils/HuffmanCompressor.cpp
    src/utils/FilePackager.cpp
    src/utils/Encryption.cpp
    src/core/tasks/BackupTask.cpp
    src/core/tasks/RestoreTask.cpp
    src/core/Filter.cpp
)
target_include_directories(BackupManagerTests PRIVATE ${CMAKE_SOURCE_DIR}/src)

# TaskTests (需要 gmock)
add_executable(TaskTests 
    src/TaskTests.cpp
    src/core/tasks/BackupTask.cpp 
    src/core/tasks/RestoreTask.cpp 
    src/core/Filter.cpp 
    src/core/models/File.cpp 
    src/utils/FilePackager.cpp 
    src/utils/Encryption.cpp 
    src/utils/ConsoleLogger.cpp 
    src/utils/FileSystem.cpp
    src/utils/HuffmanCompressor.cpp
    src/utils/FileSystemMonitor.cpp
)
target_include_directories(TaskTests PRIVATE ${CMAKE_SOURCE_DIR}/src)

# === 配置测试目标链接 ===
enable_testing()

# 查找 OpenSSL（所有测试目标共享）
find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    message(STATUS "✓ 找到 OpenSSL: ${OPENSSL_VERSION}")
else()
    message(FATAL_ERROR "✗ OpenSSL 未找到，请安装 OpenSSL")
endif()

# 查找线程库
find_package(Threads REQUIRED)

foreach(test_target IN LISTS TEST_TARGETS)
    message(STATUS "配置测试目标: ${test_target}")
    
    # === 链接 GoogleTest ===
    if(${test_target} STREQUAL "TaskTests")
        # TaskTests 需要 GMock
        target_link_libraries(${test_target} PRIVATE gmock)
        message(STATUS "  - 链接到 gmock (包含 gtest)")
    else()
        # 其他测试只需要 GTest
        target_link_libraries(${test_target} PRIVATE gtest gtest_main)
        message(STATUS "  - 链接到 gtest 和 gtest_main")
    endif()
    
    # === 链接 OpenSSL ===
    target_include_directories(${test_target} PRIVATE ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(${test_target} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    message(STATUS "  - 链接到 OpenSSL")
    
    # === 链接线程库 ===
    target_link_libraries(${test_target} PRIVATE Threads::Threads)
    message(STATUS "  - 链接到线程库")
    
    # === 链接 filesystem 库 ===
    if(NOT MSVC)
        target_link_libraries(${test_target} PRIVATE stdc++fs)
        message(STATUS "  - 链接到 stdc++fs")
    endif()
    
    # === 注册测试 ===
    add_test(NAME ${test_target} COMMAND ${test_target})
    message(STATUS "  - 已注册为测试: ${test_target}")
endforeach()

# === 主应用程序目标 ===
add_executable(BackupHelper
    src/main.cpp
    src/core/BackupEngine.cpp
    src/core/Filter.cpp
    src/core/models/File.cpp
    src/core/tasks/BackupTask.cpp
    src/core/tasks/RestoreTask.cpp
    src/core/RealTimeBackupManager.cpp
    src/core/TimerBackupManager.cpp
    src/utils/ConsoleLogger.cpp
    src/utils/FileSystem.cpp
    src/utils/HuffmanCompressor.cpp
    src/utils/FilePackager.cpp
    src/utils/Encryption.cpp
    src/utils/FileSystemMonitor.cpp
)

target_include_directories(BackupHelper PRIVATE ${CMAKE_SOURCE_DIR}/src)

# 主程序也链接相同的库
target_link_libraries(BackupHelper PRIVATE Threads::Threads)
if(OPENSSL_FOUND)
    target_include_directories(BackupHelper PRIVATE ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(BackupHelper PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()
if(NOT MSVC)
    target_link_libraries(BackupHelper PRIVATE stdc++fs)
endif()

message(STATUS "")
message(STATUS "=== 配置总结 ===")
message(STATUS "项目: ${PROJECT_NAME}")
message(STATUS "C++ 标准: ${CMAKE_CXX_STANDARD}")
message(STATUS "测试目标数量: ${TEST_TARGETS}")
message(STATUS "使用 GoogleTest: 通过 FetchContent 下载")
message(STATUS "OpenSSL: ${OPENSSL_VERSION}")
message(STATUS "配置完成！")