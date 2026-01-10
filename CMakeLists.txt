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
enable_testing()

# === 打包配置 ===
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# Google Test配置
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

if(WIN32)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

find_package(GTest REQUIRED)

# 诊断信息
message(STATUS "=== Google Test诊断信息 ===")
if(TARGET gtest)
    message(STATUS "找到目标: gtest")
    get_target_property(GTEST_INCLUDES gtest INTERFACE_INCLUDE_DIRECTORIES)
    message(STATUS "gtest包含目录: ${GTEST_INCLUDES}")
endif()

if(TARGET GTest::gtest)
    message(STATUS "找到目标: GTest::gtest")
endif()

# 项目配置...
set(CMAKE_CXX_STANDARD 17)

# 测试目标
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

# TaskTests (需要gmock)
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

# 关键修复：使用正确的目标名称
set(TEST_TARGETS FilterTests FileTests EncryptionTests FilePackagerTests TaskTests HuffmanCompressorTests BackupManagerTests)

foreach(test_target IN LISTS TEST_TARGETS)
    if(TARGET gtest)
        if(${test_target} STREQUAL "TaskTests")
            target_link_libraries(${test_target} PRIVATE gtest gtest_main gmock gmock_main)
            message(STATUS "链接 ${test_target} 到 gtest gtest_main gmock gmock_main")
        else()
            target_link_libraries(${test_target} PRIVATE gtest gtest_main)
            message(STATUS "链接 ${test_target} 到 gtest gtest_main")
        endif()
    elseif(TARGET GTest::gtest)
        if(${test_target} STREQUAL "TaskTests")
            target_link_libraries(${test_target} PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)
            message(STATUS "链接 ${test_target} 到 GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main")
        else()
            target_link_libraries(${test_target} PRIVATE GTest::gtest GTest::gtest_main)
            message(STATUS "链接 ${test_target} 到 GTest::gtest GTest::gtest_main")
        endif()
    else()
        # 手动设置包含目录作为最后手段
        message(WARNING "未找到标准目标，尝试手动配置 ${test_target}")
        target_include_directories(${test_target} PRIVATE ${gtest_SOURCE_DIR}/googletest/include)
        target_link_libraries(${test_target} PRIVATE ${gtest_BINARY_DIR}/libgtest.a)
        if(${test_target} STREQUAL "TaskTests")
            target_include_directories(${test_target} PRIVATE ${gtest_SOURCE_DIR}/googlemock/include)
            target_link_libraries(${test_target} PRIVATE ${gtest_BINARY_DIR}/libgmock.a)
        endif()
    endif()
    
    # 为所有测试目标链接OpenSSL和Threads
    find_package(OpenSSL REQUIRED)
    if(OPENSSL_FOUND)
        target_include_directories(${test_target} PRIVATE ${OPENSSL_INCLUDE_DIR})
        target_link_libraries(${test_target} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    endif()
    
    find_package(Threads REQUIRED)
    target_link_libraries(${test_target} PRIVATE Threads::Threads)
    
    # 链接filesystem库（非MSVC平台需要）
    if(NOT MSVC)
        target_link_libraries(${test_target} PRIVATE stdc++fs)
    endif()
    
    # 注册测试
    add_test(NAME ${test_target} COMMAND ${test_target})
    message(STATUS "已添加测试: ${test_target}")
endforeach()

# 主应用程序目标
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

# 查找并链接OpenSSL库
find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    message(STATUS "OpenSSL found: ${OPENSSL_VERSION}")
    target_include_directories(BackupHelper PRIVATE ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(BackupHelper PRIVATE OpenSSL::SSL OpenSSL::Crypto)
else()
    message(FATAL_ERROR "OpenSSL not found. Please install OpenSSL.")
endif()

# 链接线程库
find_package(Threads REQUIRED)
target_link_libraries(BackupHelper PRIVATE Threads::Threads)

# 链接filesystem库（非MSVC平台需要）
if(NOT MSVC)
    target_link_libraries(BackupHelper PRIVATE stdc++fs)
endif()