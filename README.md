# KiriKiroid2 Android10+ 交叉编译指南

本指南支持在 **Windows** 和 **Linux** 环境中交叉编译 KiriKiroid2 的 Android 版本，支持以下架构：

- `arm64-v8a`
- `armeabi-v7a`

---

## 依赖工具

请确保以下工具已正确安装：

- `AndroidSDK@33+`
- `AndroidNDK@27.2.12479018`
- `gradle@8.10`
- `jdk@21`

---

## 编译环境配置

### 环境变量

配置以下环境变量：

- `ANDROID_SDK`: `/path/to/androidsdk`
- `ANDROID_NDK`: `/path/to/androidndk`

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

- **Release 版本**:  
  ```bash
  gradle assembleRelease
  ```
- **Debug 版本**:  
  ```bash
  gradle assembleDebug
  ```
- **全部版本**:  
  ```bash
  gradle assemble
  ```

> **生成的二进制文件位置**:  
> - Debug: `out/app/outputs/apk/debug`  
> - Release: `out/app/outputs/apk/release`  

---

### 插件资源

查看相关插件和工具库：[wamsoft GitHub 仓库](https://github.com/orgs/wamsoft/repositories?type=all)