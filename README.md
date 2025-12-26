# 数据备份与还原软件 (C++版本)

一款基于C++开发的支持多种文件类型、元数据保留、自定义筛选、打包、压缩、加密和图形界面的本地数据备份与还原工具。

## 🚀 功能特性

- **基本功能**
  - 数据备份：将目录树备份到指定位置
  - 数据还原：从备份位置恢复到指定目录

- **扩展功能**
  - 支持特殊文件类型（符号链接、管道等）
  - 保留文件元数据（权限、时间、属主等）
  - 自定义备份筛选（路径、类型、名称、时间、大小）
  - 打包解包：将所有文件合并为单个备份文件
  - 压缩解压：支持 Haff / LZ77 压缩算法
  - 加密解密：支持 AES / DES 加密
  - 图形界面：基于 Qt 的友好界面

## 🛠️ 安装与运行

### 环境要求
- C++17 兼容编译器 (GCC 9+, Clang 10+, MSVC 2019+)
- CMake 3.16+
- OpenSSL 1.1.x+ (加密功能)
- 系统支持：Windows、Linux

### Windows 构建步骤
```bash
git clone [<项目地址>](https://github.com/EclipseUESTC/backuphelper.git)
cd <项目目录>
mkdir build && cd build
cmake ..
cmake --build . --config Release
.Release\BackupHelper.exe
```

### Linux 构建步骤

#### 1. 安装依赖
```bash
# Ubuntu/Debian 系统
sudo apt update
sudo apt install build-essential cmake libssl-dev

# CentOS/RHEL 系统
sudo yum groupinstall "Development Tools"
sudo yum install cmake3 openssl-devel
```

#### 2. 编译和运行
```bash
git clone [<项目地址>](https://github.com/EclipseUESTC/backuphelper.git)
cd <项目目录>

# 使用提供的构建脚本
chmod +x build_linux.sh
./build_linux.sh

# 或手动构建
mkdir build && cd build
cmake ..
make -j$(nproc)
./BackupHelper
```

#### 3. 构建脚本选项
```bash
./build_linux.sh [options]
Options:
  -h, --help      Show this help message
  -t, --test      Run tests after building
  -c, --clean     Clean build directory before building
  -r, --run       Run the application after building
```