## KiriKiroid2 Android10^ 交叉编译
* 支持 Windows & Linux 交叉编译: android arm64 kirikiroid2

## 依赖工具
* `tar@latest`
* `ninja@latest`
* `cmake@22^`
* `vcpkg@latest`
* `AndroidSDK@33^`
* `AndroidNDK@27.1.12297006`
* `gradle@8.10`
* `jdk@17`

## aarch64-android编译

### 环境变量
* "VCPKG_ROOT": "/path/to/vcpkg"
* "ANDROID_SDK": "/path/to/androidsdk"
* "ANDROID_NDK": "/path/to/androidndk"
* "ANDROID_NDK_HOME": "/path/to/androidndk"

#### Windows 环境变量注意!!! 必须使用`/`或`\\`分割路径, 例如:
* `D:\vcpkg`: 错误! cmake不转义`\`, 会导致找不到路径
* `D:/vcpkg`: 正确!

#### 编译apk
* Release: `gradle -p android assembleRelease`
* Debug: `gradle -p android assembleDebug`

#### 单独编译so动态库
* Release: 
  * `cmake -GNinja -B cmake-build-android-release -DCMAKE_BUILD_TYPE=Release -DANDROID_ABI="arm64-v8a" -DANDROID_PLATFORM=29`
  * `cmake --build cmake-build-android-release`
* Debug: 
  * `cmake -GNinja -B cmake-build-android-debug -DCMAKE_BUILD_TYPE=Debug -DANDROID_ABI="arm64-v8a" -DANDROID_PLATFORM=29`
  * `cmake --build cmake-build-android-debug`

### 插件
* `https://github.com/orgs/wamsoft/repositories?type=all`