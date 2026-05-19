# ZeroTier Direct Client & Tools

基于 ZeroTier 的直连网络客户端和工具集，包含网络接入、端口转发、代理服务器和网络测试工具。
适合没有管理员权限的 Windows 系统上快速搭建 ZeroTier 直连网络。
也适合没有Visual Studio 的 Windows 系统上快速搭建 ZeroTier 直连网络。

## 📋 目录

- [项目简介](#项目简介)
- [快速开始](#快速开始)
- [编译指南](#编译指南)
  - [MinGW-GCC 环境配置](#mingw-gcc-环境配置)
  - [环境准备](#环境准备)
  - [统一编译（推荐）](#统一编译推荐)
  - [单独编译各工具](#单独编译各工具)
- [工具说明](#工具说明)
  - [zt-join - 网络接入工具](#1-zt-join---网络接入工具)
  - [zt-forward - 端口转发工具](#2-zt-forward---端口转发工具)
  - [pylon - SOCKS5/HTTP 代理服务器](#3-pylon---socks5http-代理服务器)
  - [zt-ping - 网络连通性测试](#4-zt-ping---网络连通性测试)
- [使用方法](#使用方法)
- [功能特性](#功能特性)
- [常见问题](#常见问题)

---

## 项目简介

本项目提供一套完整的 ZeroTier 命令行工具集：

### 🛠️ 工具列表

| 工具 | 功能 | 大小 |
|------|------|------|
| **zt-join** | ZeroTier 网络接入配置工具 | ~4.8 MB |
| **zt-forward** | TCP 端口转发工具 | ~2.3 MB |
| **pylon** | SOCKS5 + HTTP CONNECT 代理服务器 | ~4.9 MB |
| **zt-ping** | 网络连通性测试工具 | ~4.8 MB |

### 应用场景

- 🔒 安全的内网穿透访问
- 🌐 通过 ZeroTier 虚拟网络代理外网流量
- 💻 Windows 系统全局代理配置
- 🚀 P2P 直连通信（绕过 NAT/防火墙）
- 🔄 端口转发和内网服务暴露

---

## 项目结构

```
zerotier-direct/
├── libzt/                    # ZeroTier Sockets 子模块 (git submodule)
│   ├── ext/                  # 依赖库 (ZeroTierOne, lwIP, etc.)
│   ├── include/              # 头文件
│   ├── src/                  # 源代码
│   └── CMakeLists.txt        # CMake 配置
├── build/                    # 编译产物目录
│   ├── libzt.a              # libzt 静态库
│   ├── include/             # 头文件
│   │   └── ZeroTierSockets.h
│   ├── zt-join.exe          # 网络接入工具
│   ├── zt-forward.exe       # 端口转发工具
│   ├── pylon.exe            # SOCKS5/HTTP 代理服务器
│   └── zt-ping.exe          # 网络连通性测试
├── zt-data/                  # ZeroTier 节点数据（包含私钥，不提交到 git）
├── 源码文件/
│   ├── zt-join.c            # 网络接入工具源码
│   ├── zt-forward.c         # 端口转发工具源码
│   ├── pylon.c              # SOCKS5/HTTP 代理服务器源码
│   ├── zt-ping.c            # 网络连通性测试源码
├── 构建脚本/
│   ├── build.ps1            # 统一 PowerShell 编译脚本
│   ├── build-libzt.ps1      # libzt 编译脚本
│   ├── Makefile             # GNU Make 构建文件
│   ├── CMakeLists.txt       # CMake 构建配置
│   └── compile-*.ps1        # 各应用独立编译脚本（可选）
└── README.md                 # 项目文档
```

---

## 快速开始

### MinGW-GCC 环境配置

#### 配置编译器路径

本项目统一使用 MinGW GCC 编译所有组件。**在使用任何编译脚本前，请确保 gcc/g++ 在系统 PATH 中**。

##### 方法 1: 添加到系统环境变量（推荐）

将 MinGW 的 bin 目录添加到系统的 `PATH` 环境变量中：

1. 右键 **此电脑** → **属性** → **高级系统设置**
2. 点击 **环境变量**
3. 在 **系统变量** 中找到 `Path`，点击 **编辑**
4. 添加 MinGW 的 bin 目录路径，例如：`C:\mingw64\bin`
5. 点击 **确定** 保存

配置后，可以在新的 PowerShell 窗口中验证：
```powershell
gcc --version
g++ --version
```

##### 方法 2: 使用环境变量指定编译器

在运行编译脚本前，设置 CC 和 CXX 环境变量：

```powershell
# PowerShell
$env:CC = "gcc"
$env:CXX = "g++"
.\build.ps1 all
```

或者在命令提示符中：
```cmd
set CC=gcc
set CXX=g++
```

##### 常见的 MinGW 安装路径

- **MinGW-w64 官方版本**: `C:\mingw64\` 或 `C:\Program Files\mingw-w64\`
- **MSYS2**: `C:\msys64\mingw64\bin\`
- **自定义安装**: 根据你安装时的选择而定

> ⚠️ **重要提示**: 
> - 请确保使用的是 **MinGW-w64**（支持 64 位），而非旧版 MinGW
> - 推荐使用 GCC 10+ 版本以获得更好的 C++11 支持
> - 不同编译器的 ABI 可能不兼容，**请勿混用 Visual Studio 和 MinGW 编译的库**

---

### 前置要求

- **操作系统**: Windows 7+ (推荐 Windows 10/11)
- **编译器**: MinGW GCC 10+（统一编译所有组件）
- **其他工具**: 
  - Git（克隆仓库和初始化子模块）
  - CMake 3.15+（配置 libzt 编译环境）
  - PowerShell（执行编译脚本）

> ✅ **本项目统一使用 MinGW GCC 编译**，避免混用不同编译器导致的兼容性问题。

### 一键运行（如果已有编译好的程序）

``powershell
# 设置 UTF-8 编码
chcp 65001

# 运行 pylon 代理服务器
.\build\pylon.exe
```

程序会自动：
1. 加载 `zt-data` 目录中的节点身份
2. 连接到 ZeroTier 网络
3. 在端口 `5888` 启动 SOCKS5/HTTP 代理

---

## 编译指南

### 环境准备

#### 必需工具

1. **MinGW GCC**（推荐版本：GCC 10+）
   - 下载路径：[MinGW-w64](https://www.mingw-w64.org/)
   - 验证安装：
     ```powershell
     gcc --version
     g++ --version
     ```

2. **Git**
   - 用于克隆 libzt 仓库和初始化子模块

3. **CMake**（3.15+）
   - 用于配置 libzt 编译环境
   - 下载地址：[CMake 官网](https://cmake.org/download/)

4. **PowerShell**
   - Windows 自带，用于执行编译脚本

---

### 统一编译（推荐）

项目提供了三种统一的构建方式，推荐使用 PowerShell 脚本或 Make。

#### 方法 1: PowerShell 脚本（最简单）

```
# 编译所有工具
.\build.ps1 all

# 编译单个工具
.\build.ps1 zt-join      # 仅编译 zt-join
.\build.ps1 zt-forward   # 仅编译 zt-forward
.\build.ps1 pylon        # 仅编译 pylon
.\build.ps1 zt-ping      # 仅编译 zt-ping

# 清理构建目录
.\build.ps1 clean
```

#### 方法 2: GNU Make

```
# 编译所有工具
make all

# 编译单个工具
make zt-join
make zt-forward
make pylon
make zt-ping

# 清理构建目录
make clean
```

#### 方法 3: CMake

```
# 配置并编译
cmake -B build_cmake
cmake --build build_cmake
```

---

### 单独编译各工具

如果需要了解详细的编译过程，可以参考以下各工具的独立编译方法。

#### 步骤 1: 编译 libzt (ZeroTier Sockets)

所有工具都依赖 libzt 静态库，首先需要编译它。

##### 快速编译（推荐）

```
# 初始化子模块
git submodule update --init --recursive

# 运行编译脚本
.\build-libzt.ps1
```

该脚本会自动：
- ✅ 检查子模块是否存在
- ✅ 配置 CMake（MinGW Makefiles + Release 模式）
- ✅ 编译 libzt 静态库
- ✅ 复制 `libzt.a` 到 `build/libzt.a`
- ✅ 复制 `ZeroTierSockets.h` 到 `build/include/`

---

#### 步骤 2: 编译各工具

编译 libzt 后，可以使用统一构建脚本编译所有工具，或者参考以下独立编译方法。

---

## 工具说明

### 1. zt-join - 网络接入工具

**功能**: 生成 ZeroTier 配置文件并加入指定网络

#### 使用方法

**命令行模式**:
```
# 基本用法
.\build\zt-join.exe -n <network_id>

# 指定配置目录
.\build\zt-join.exe -d zt-data -n abcdef1234567890
```

**参数说明**:
- `-d <data_dir>`: 可选，配置文件保存目录（默认：`zt-data`）
- `-n <network_id>`: 必填，ZeroTier 网络 ID（16 位十六进制）

**交互式模式**（不带参数运行）:
```
.\build\zt-join.exe
```

程序会提示输入：
1. 配置目录（默认 `zt-data`）
2. 网络 ID（必须为 16 位十六进制字符串）

#### 工作流程

1. 初始化 ZeroTier 节点存储目录
2. 禁用端口映射（避免 UPnP 问题）
3. 注册事件回调
4. 等待获取 IP 地址（最多 30 秒）
5. 显示获取到的 IP 地址
6. 按回车键退出

#### 输出示例

```
[INFO] ========================================
[INFO]   ZeroTier Network Join Tool
[INFO] ========================================
[INFO] Data directory: ./zt-data
[INFO] Network ID: abcdef1234567890
[INFO] Starting ZeroTier node...
[INFO] Waiting for IP address assignment (timeout: 30s)...
[INFO] Got IPv4 address: 172.22.1.4
[SUCCESS] Successfully joined network!
[INFO] Press Enter to exit...
```

---

### 2. zt-forward - 端口转发工具

**功能**: 将本地端口通过 ZeroTier 转发到目标地址

#### 使用方法

```
# 基本语法
.\build\zt-forward.exe tcp:<local_port>:<target_ip>:<target_port>

# 多个转发规则
.\build\zt-forward.exe tcp:2222:172.22.1.17:22 tcp:8080:172.22.1.18:80
```

**参数格式**: `tcp:<local_port>:<target_ip>:<target_port>`

#### 功能特性

- ✅ 支持多个转发规则同时运行
- ✅ 双向数据转发，保持连接活跃
- ✅ 自动协议检测（SOCKS5、HTTP CONNECT）
- ✅ 智能流量分流（内网走 ZeroTier，外网直连）
- ✅ 多线程并发处理

#### 输出示例

```
[INFO] ========================================
[INFO]   ZeroTier Port Forwarder
[INFO] ========================================
[INFO] Rule 1: 0.0.0.0:2222 -> 172.22.1.17:22
[INFO] Rule 2: 0.0.0.0:8080 -> 172.22.1.18:80
[INFO] Starting ZeroTier node...
[INFO] Got IPv4 address: 172.22.1.4
[INFO] Listening on 0.0.0.0:2222...
[INFO] Listening on 0.0.0.0:8080...
[INFO] New connection from 127.0.0.1:12345
[INFO] → [ZeroTier] 172.22.1.17:22
```

---

### 3. pylon - SOCKS5/HTTP 代理服务器

**功能**: 双协议代理服务器，支持智能流量分流

#### 使用方法

```
# 启动代理服务器
.\build\pylon.exe
```

默认监听端口：**5888**

#### 支持的协议

- ✅ **SOCKS5 代理**: 标准 SOCKS5 协议，适用于支持 SOCKS 的应用
- ✅ **HTTP CONNECT 隧道**: 兼容 Windows 系统代理设置
- ✅ **普通 HTTP 请求**: 支持 GET/POST/PUT/DELETE 等方法的缓存转发

#### 配置 Windows 系统代理

1. 打开 **设置 → 网络和 Internet → 代理**
2. 启用 **使用代理服务器**
3. 配置代理：
   - **地址**: `127.0.0.1`
   - **端口**: `5888`
4. 点击 **保存**

#### 日志示例

```
[INFO] New client connection
[INFO] HTTP CONNECT to github.com:443
[INFO] Destination: github.com:443
[INFO] → [Direct] github.com:443              # 外网直接连接
[INFO] Connection established, forwarding data...

[INFO] New client connection
[INFO] HTTP CONNECT to 172.22.1.17:8080
[INFO] Destination: 172.22.1.17:8080
[INFO] → [ZeroTier] 172.22.1.17:8080          # 内网通过 ZeroTier
[INFO] Connection established, forwarding data...
```

---

### 4. zt-ping - 网络连通性测试

**功能**: 测试 ZeroTier 网络连通性

#### 使用方法

```
# Ping 指定地址
.\build\zt-ping.exe 172.22.1.17

# 指定次数
.\build\zt-ping.exe -c 4 172.22.1.17
```

#### 输出示例

```
[INFO] Pinging 172.22.1.17 via ZeroTier...
[INFO] Reply from 172.22.1.17: time=12ms
[INFO] Reply from 172.22.1.17: time=8ms
[INFO] Reply from 172.22.1.17: time=10ms
[INFO] Reply from 172.22.1.17: time=9ms
[INFO] 4 packets transmitted, 4 received, 0% loss
```

---

## 功能特性

### ✅ 统一的编译系统

- **PowerShell 脚本**: `build.ps1` 支持所有目标的编译
- **GNU Make**: 跨平台兼容的 Makefile
- **CMake**: 现代化的构建配置

### ✅ 双协议支持（pylon）

- **SOCKS5 代理**: 标准 SOCKS5 协议，适用于支持 SOCKS 的应用
- **HTTP CONNECT 代理**: 兼容 Windows 系统代理设置

### ✅ 智能流量分流

| 目标地址类型 | 路由方式 | 示例 |
|-------------|---------|------|
| 内网 IP (172.22.x.x) | 通过 ZeroTier | `172.22.1.17:8080` |
| 外网域名/IP | 直接连接互联网 | `github.com:443` |

### ✅ 清晰的日志输出

- 普通信息使用标准输出（非红色）
- 明确标识连接路径：`[ZeroTier]` 或 `[Direct]`
- 无冗余调试信息

### ✅ 稳定性优化

- 禁用 UPnP/NAT-PMP（避免 MinGW 环境崩溃）
- 事件驱动的网络状态检测
- 自动重连机制

---

## 常见问题

### Q1: 编译时提示找不到 `ZeroTierSockets.h`

**解决方案**:

确保子模块已正确初始化：
```powershell
git submodule update --init --recursive
```

确保头文件已复制到正确位置：
```powershell
# 检查文件是否存在
Test-Path ".\build\include\ZeroTierSockets.h"

# 如果不存在，重新编译 libzt
.\build-libzt.ps1
```

---

### Q2: 运行时提示 "Failed to start ZeroTier node"

**可能原因**:
1. `zt-data` 目录不存在或损坏
2. 节点身份文件缺失

**解决方案**:
```powershell
# 检查 zt-data 目录
ls .\zt-data\

# 应该包含以下文件：
# - identity.public
# - identity.secret
# - zerotier_one.port
```

如果目录为空，首次运行时会自动创建节点身份。

---

### Q3: 无法连接到内网地址（如 172.22.1.17）

**排查步骤**:

1. **确认节点已授权**
   - 访问 [my.zerotier.com](https://my.zerotier.com/)
   - 找到你的网络
   - 在 **Members** 列表中勾选节点的 **Auth?** 复选框

2. **等待配置生效**
   - 授权后等待 10-30 秒
   - 观察工具输出是否显示获取到 IP

3. **检查防火墙**
   - 确保 UDP 端口 9993 未被阻止
   - 允许相关 exe 通过防火墙

4. **验证连通性**
   ```powershell
   .\build\zt-ping.exe 172.22.1.17
   ```

---

### Q4: 编译时出现链接错误

**常见错误及解决方案**:

#### 错误 1: `undefined reference to '__imp_zts_*'`

**原因**: 未定义 `ADD_EXPORTS` 宏

**解决方案**: 确保编译时添加了 `-DADD_EXPORTS` 参数（已由构建脚本自动处理）

#### 错误 2: `undefined reference to '_snprintf'`

**原因**: MinGW 的 snprintf 符号与 MSVCRT 不兼容

**解决方案**: 确保添加了 `-Wl,--defsym,__imp__snprintf=_snprintf` 参数（已由构建脚本自动处理）

#### 错误 3: 找不到 C++ 运行时库

**原因**: 未静态链接 libstdc++

**解决方案**: 确保添加了 `-static-libgcc -static-libstdc++` 参数（已由构建脚本自动处理）

---

### Q5: 如何重置节点身份？

如果需要更换节点 ID 或重新生成身份：

```
# 1. 停止正在运行的程序

# 2. 备份当前身份（可选）
Move-Item .\zt-data .\zt-data-backup

# 3. 删除旧身份
Remove-Item .\zt-data -Recurse -Force

# 4. 重新运行 zt-join 或 pylon（会自动创建新身份）
.\build\zt-join.exe -n <network_id>

# 5. 在 my.zerotier.com 重新授权新节点
```

⚠️ **警告**: 节点身份包含私钥 (`identity.secret`)，请妥善保管！

---

### Q6: 如何测试代理是否工作？

**方法 1: 使用 curl**
```
# 测试 SOCKS5 代理
curl --socks5 127.0.0.1:5888 https://www.example.com

# 测试 HTTP 代理
curl --proxy http://127.0.0.1:5888 https://www.example.com
```

**方法 2: 浏览器测试**
1. 配置浏览器使用代理 `127.0.0.1:5888`
2. 访问任意网站
3. 观察 pylon 输出是否有连接记录

**方法 3: 访问内网服务**
```
# 假设内网有 Web 服务器在 172.22.1.17:8080
# 配置系统代理后，浏览器访问：
http://172.22.1.17:8080
```

---

## 技术栈

| 组件 | 技术 | 说明 |
|------|------|------|
| **libzt** | C++ / CMake / MinGW | ZeroTier 官方网络库（MinGW 编译） |
| **zt-join** | C99 / Winsock2 / MinGW | 网络接入配置工具 |
| **zt-forward** | C99 / Winsock2 / MinGW | TCP 端口转发工具 |
| **pylon** | C99 / Winsock2 / MinGW | SOCKS5 + HTTP 代理实现 |
| **zt-ping** | C99 / Winsock2 / MinGW | 网络连通性测试工具 |
| **编译工具** | MinGW GCC (g++) | 统一使用 MinGW 编译所有组件 |
| **网络协议** | SOCKS5, HTTP CONNECT | 双协议支持 |
| **操作系统** | Windows 7+ | 使用 Windows API |

---

## 编译流程总结

```
graph LR
    A[初始化 git 子模块] --> B[运行 build-libzt.ps1]
    B --> C[自动生成 libzt.a 和头文件]
    C --> D[运行 build.ps1 all]
    D --> E[生成所有工具]
    E --> F[zt-join.exe]
    E --> G[zt-forward.exe]
    E --> H[pylon.exe]
    E --> I[zt-ping.exe]
```

**关键点**：
1. ✅ 统一使用 MinGW GCC 编译（避免 ABI 兼容性问题）
2. ✅ 使用 `g++` 而非 `gcc` 链接（因为 libzt.a 是 C++ 编译的）
3. ✅ 添加 `-static-libgcc -static-libstdc++`（减少依赖）
4. ✅ 解决 snprintf 符号问题（`--defsym` 参数）
5. ✅ 定义 `ADD_EXPORTS` 宏（避免 dllimport 错误）
6. ✅ 使用 git 子模块管理 libzt 源码，无需手动克隆

---

## 参考资料

- [ZeroTier 官网](https://www.zerotier.com/)
- [libzt GitHub 仓库](https://github.com/zerotier/libzt)
- [ZeroTier One 客户端](https://www.zerotier.com/download/)
- [SOCKS5 协议规范 (RFC 1928)](https://datatracker.ietf.org/doc/html/rfc1928)
- [HTTP CONNECT 方法](https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods/CONNECT)

---

## 许可证

本项目基于 ZeroTier 开源库构建，遵循相应的开源许可证。

---

## 贡献

欢迎提交 Issue 和 Pull Request！

如有问题或建议，请通过以下方式联系：
- 提交 GitHub Issue
- 参考 ZeroTier 官方文档