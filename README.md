# KiriKiroid2 Android10+ 交叉编译指南

本指南支持在 **Windows** 和 **Linux** 环境中交叉编译 KiriKiroid2 的 Android 版本，支持以下架构：

- `arm64-v8a`
- `x86_64`

---

## 依赖工具

请确保以下工具已正确安装：
- [ninja@latest](https://github.com/ninja-build/ninja/releases)
- [cmake@3.31.1+](https://cmake.org/download/)
- [vcpkg@latest](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started)
- [AndroidSDK@33](https://developer.android.com/tools/releases/platform-tools)
- [AndroidNDK@27.2.12479018](https://developer.android.com/ndk/downloads)
- [jdk@17](https://jdk.java.net/archive/)
- `bison@3.8.2+`: 非 Windows 主机平台
- `python3`: 非 Windows 主机平台
- `NASM@latest`: 非 Windows 主机平台
---

## 编译环境配置

### 环境变量

配置以下环境变量：

- `VCPKG_ROOT`: `/path/to/vcpkg`
- `ANDROID_SDK`: `/path/to/androidsdk`
- `ANDROID_NDK`: `/path/to/androidndk`

> ⚠️ **注意**  
> 在 Windows 上，环境变量路径必须使用 `/` 或 `\\`，避免使用单一的 `\`。  
> 
> - **错误示例**: `D:\vcpkg` (cmake 不转义 `\`，导致路径错误)  
> - **正确示例**: `D:/vcpkg`

---

### 编译步骤

#### 配置签名证书

在 `android/app` 目录下，准备以下文件：

1. **`sign.keystore`**: 签名证书文件  
2. **`sign.properties`**: 签名信息文件，格式如下：
   ```properties
   SIGN_KEY_ALIAS = sign        # 别名
   SIGN_KEY_PASS = 123456       # 密码
   SIGN_STORE_PASS = 123456     # 存储密码
   ```

#### APK 编译

执行以下命令生成 APK 文件：

- Windows: `./gradlew.bat assemble`
- Linux: `./gradlew assemble`

> **生成的二进制文件位置**:  
> - Debug: `out/app/outputs/apk/debug`  
> - Release: `out/app/outputs/apk/release`  

### 插件资源

查看相关插件和工具库：[wamsoft GitHub 仓库](https://github.com/orgs/wamsoft/repositories?type=all)

### 代码格式化

clang-format: `clang-format -i --verbose $(find ./cpp -regex ".+\.\(cpp\|cc\|h\|hpp\|inc\)")`
