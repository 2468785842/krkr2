# KiriKiroid2 Android10+ 交叉编译指南

本指南支持在 **Windows** 和 **Linux** 环境中交叉编译 KiriKiroid2 的 Android 版本，支持以下架构：

- `arm64-v8a`
- `x86_64`

---

## 依赖工具

请确保以下工具已正确安装：
- `bison`
- `python3`
- `ninja@latest`
- `cmake@22+`
- `vcpkg@latest`
- `AndroidSDK@33+`
- `AndroidNDK@27.2.12479018`
- `jdk@17`
- `NASM(FFmpeg需要)`: 仅 x86_64 架构

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

- Windows
```bash
./gradlew.bat assemble
```
- Linux
```bash
./gradlew assemble
```

> **生成的二进制文件位置**:  
> - Debug: `out/app/outputs/apk/debug`  
> - Release: `out/app/outputs/apk/release`  

#### SO 动态库编译

为支持多架构，使用以下模板命令生成动态库：

1. 设置架构变量：  
   - **`arm64-v8a`**: `ANDROID_ABI="arm64-v8a"`  
   - **`x86_64`**: `ANDROID_ABI="x86_64"`

2. 执行命令：  

   - **Release 版本**:  
     ```bash
     cmake -GNinja -B out/cmake-build-android-${ANDROID_ABI}-release \
           -DCMAKE_BUILD_TYPE=Release \
           -DANDROID_ABI="${ANDROID_ABI}" \
           -DANDROID_PLATFORM=29
     cmake --build out/cmake-build-android-${ANDROID_ABI}-release
     ```
   - **Debug 版本**:  
     ```bash
     cmake -GNinja -B out/cmake-build-android-${ANDROID_ABI}-debug \
           -DCMAKE_BUILD_TYPE=Debug \
           -DANDROID_ABI="${ANDROID_ABI}" \
           -DANDROID_PLATFORM=29
     cmake --build out/cmake-build-android-${ANDROID_ABI}-debug
     ```

**示例**：  
对于 `arm64-v8a` Release 构建：  
```bash
cmake -GNinja -B out/cmake-build-android-arm64-v8a-release \
      -DCMAKE_BUILD_TYPE=Release \
      -DANDROID_ABI="arm64-v8a" \
      -DANDROID_PLATFORM=29
cmake --build out/cmake-build-android-arm64-v8a-release
```

---

### 插件资源

查看相关插件和工具库：[wamsoft GitHub 仓库](https://github.com/orgs/wamsoft/repositories?type=all)
