cmake_minimum_required(VERSION 3.16)
project(BackupHelper)

# === 解决C2001错误的UTF-8配置 ===
if(MSVC)
    # 对于MSVC编译器：设置源文件和执行字符集为UTF-8
    add_compile_options(/utf-8)
    message(STATUS "已启用MSVC UTF-8编码支持")
else()
    # 对于GCC/Clang编译器：设置输入字符集为UTF-8
    add_compile_options(-finput-charset=UTF-8)
    message(STATUS "已启用GCC/Clang UTF-8编码支持")
    
endif()

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 收集源代码文件
file(GLOB_RECURSE SOURCES "src/*.cpp")

# 添加可执行文件
add_executable(BackupHelper ${SOURCES})

# 设置包含目录
target_include_directories(BackupHelper PRIVATE ${CMAKE_SOURCE_DIR}/src)

# 如果需要，可以添加Qt依赖
# find_package(Qt5 COMPONENTS Widgets REQUIRED)
# target_link_libraries(BackupHelper Qt5::Widgets)

# 设置输出目录
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# 在Windows上设置特定选项
if(WIN32)
    # 设置Windows特定编译选项
    add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()
