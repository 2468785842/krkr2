# ---------- 基础 ----------
FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

# ---------- 系统依赖 ----------
RUN apt-get update && apt-get install -y --no-install-recommends \
        wget curl unzip git python3 bison nasm \
        openjdk-17-jdk-headless \
    && rm -rf /var/lib/apt/lists/*

# ---------- Android SDK/NDK ----------
ARG ANDROID_SDK_ROOT=/opt/android-sdk
ARG ANDROID_NDK_VERSION=28.0.13004108
ENV ANDROID_SDK_ROOT=$ANDROID_SDK_ROOT
ENV ANDROID_NDK=$ANDROID_SDK_ROOT/ndk/$ANDROID_NDK_VERSION
ENV ANDROID_HOME=$ANDROID_SDK_ROOT
ENV PATH="$ANDROID_SDK_ROOT/cmdline-tools/latest/bin:$ANDROID_SDK_ROOT/platform-tools:$PATH"

# 1) 安装命令行工具
RUN mkdir -p $ANDROID_SDK_ROOT && cd $ANDROID_SDK_ROOT \
 && wget -q https://dl.google.com/android/repository/commandlinetools-linux-10406996_latest.zip \
 && unzip -q commandlinetools-linux-10406996_latest.zip \
 && mkdir -p cmdline-tools/latest \
 && mv cmdline-tools/* cmdline-tools/latest/ \
 && rm commandlinetools-linux-10406996_latest.zip

# 2) 接受许可证并安装组件
RUN yes | sdkmanager --licenses \
 && sdkmanager --install \
        "platform-tools" \
        "platforms;android-33" \
        "build-tools;34.0.0" \
        "cmake;3.31.1" \
        "ndk;$ANDROID_NDK_VERSION"

# ---------- vcpkg ----------
ENV VCPKG_ROOT=/opt/vcpkg
ENV VCPKG_INSTALL_OPTIONS="--debug"
RUN git clone https://github.com/microsoft/vcpkg.git $VCPKG_ROOT \
 && $VCPKG_ROOT/bootstrap-vcpkg.sh

# （可选）GitHub Packages 缓存；如不需要可删除
# ENV VCPKG_BINARY_SOURCES="clear;nuget,https://nuget.pkg.github.com/<OWNER>/index.json,readwrite"

# ---------- Gradle ----------
# 使用官方 Gradle Wrapper 即可，无需额外安装
WORKDIR /workspace

# ---------- 源码 ----------
COPY .. .

# ---------- 构建 ----------
# 默认任务：assembleDebug；可根据需要改为 assembleRelease
CMD ["./gradlew", "assembleDebug"]


# 添加元数据
LABEL description="Android build environment for Krkr2 project" \
      version="1.0"
#      maintainer="your-email@example.com" \
