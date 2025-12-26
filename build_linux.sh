#!/bin/bash

# 构建脚本，用于在Linux系统上编译和运行BackupHelper项目

echo "=== BackupHelper Linux Build Script ==="

# 检查是否已安装必要的依赖
check_dependencies() {
    echo "Checking dependencies..."
    
    # 检查CMake是否安装
    if ! command -v cmake &> /dev/null; then
        echo "ERROR: CMake is not installed. Please install it with 'sudo apt install cmake' or similar."
        return 1
    fi
    
    # 检查C++编译器是否安装
    if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
        echo "ERROR: No C++ compiler found. Please install GCC or Clang."
        return 1
    fi
    
    # 检查OpenSSL是否安装
    if ! pkg-config --exists openssl; then
        echo "ERROR: OpenSSL is not installed. Please install it with 'sudo apt install libssl-dev' or similar."
        return 1
    fi
    
    echo "All dependencies are installed."
    return 0
}

# 创建构建目录
create_build_dir() {
    echo "Creating build directory..."
    mkdir -p build
    cd build
}

# 运行CMake配置
run_cmake() {
    echo "Running CMake configuration..."
    cmake ..
    if [ $? -ne 0 ]; then
        echo "ERROR: CMake configuration failed."
        return 1
    fi
}

# 编译项目
build_project() {
    echo "Building project..."
    make -j$(nproc)
    if [ $? -ne 0 ]; then
        echo "ERROR: Build failed."
        return 1
    fi
}

# 运行测试
run_tests() {
    echo "Running tests..."
    ./FilterTests
    if [ $? -ne 0 ]; then
        echo "ERROR: Tests failed."
        return 1
    fi
}

# 显示使用说明
show_usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  -h, --help      Show this help message"
    echo "  -t, --test      Run tests after building"
    echo "  -c, --clean     Clean build directory before building"
    echo "  -r, --run       Run the application after building"
}

# 解析命令行参数
TEST=0
CLEAN=0
RUN=0

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_usage
            exit 0
            ;;
        -t|--test)
            TEST=1
            shift
            ;;
        -c|--clean)
            CLEAN=1
            shift
            ;;
        -r|--run)
            RUN=1
            shift
            ;;
        *)
            echo "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# 清理构建目录（如果需要）
if [ $CLEAN -eq 1 ]; then
    echo "Cleaning build directory..."
    rm -rf build
fi

# 主构建流程
check_dependencies || exit 1
create_build_dir || exit 1
run_cmake || exit 1
build_project || exit 1

# 运行测试（如果需要）
if [ $TEST -eq 1 ]; then
    run_tests || exit 1
fi

# 回到项目根目录
cd ..

# 运行应用程序（如果需要）
if [ $RUN -eq 1 ]; then
    echo "Running application..."
    ./build/BackupHelper
fi

echo "=== Build completed successfully! ==="
echo "You can run the application with: ./build/BackupHelper"
echo "You can run the tests with: ./build/FilterTests"