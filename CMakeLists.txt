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

FetchContent_MakeAvailable(googletest)

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
add_executable(FilterTests 
    src/FilterTests.cpp
    src/core/Filter.cpp 
    src/core/models/File.cpp 
    src/utils/ConsoleLogger.cpp 
    src/utils/FileSystem.cpp
)
target_include_directories(FilterTests PRIVATE ${CMAKE_SOURCE_DIR}/src)

# 关键修复：使用正确的目标名称
if(TARGET gtest)
    target_link_libraries(FilterTests PRIVATE gtest gtest_main)
    message(STATUS "链接到 gtest 和 gtest_main")
elseif(TARGET GTest::gtest)
    target_link_libraries(FilterTests PRIVATE GTest::gtest GTest::gtest_main)
    message(STATUS "链接到 GTest::gtest 和 GTest::gtest_main")
else()
    # 手动设置包含目录作为最后手段
    message(WARNING "未找到标准目标，尝试手动配置")
    target_include_directories(FilterTests PRIVATE ${gtest_SOURCE_DIR}/googletest/include)
    target_link_libraries(FilterTests PRIVATE ${gtest_BINARY_DIR}/libgtest.a)
endif()

# 注册测试
add_test(NAME FilterTests COMMAND FilterTests)
message(STATUS "已添加测试: FilterTests")
