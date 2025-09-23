# ---------- 基础 ----------
FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

# ---------- 系统依赖 ----------
RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates dos2unix \
        wget curl zip unzip tar git python3 bison nasm \
        gcc g++ build-essential pkgconf autoconf automake libtool openjdk-17-jdk-headless \
    && rm -rf /var/lib/apt/lists/*

# ---------- Android SDK/NDK ----------
ARG ANDROID_NDK_VERSION=28.0.13004108
ENV ANDROID_SDK_ROOT=/opt/android-sdk
ENV ANDROID_NDK=$ANDROID_SDK_ROOT/ndk/$ANDROID_NDK_VERSION
ENV ANDROID_HOME=$ANDROID_SDK_ROOT

# 1) 安装命令行工具
RUN mkdir -p $ANDROID_SDK_ROOT \
    && wget -q https://dl.google.com/android/repository/commandlinetools-linux-10406996_latest.zip \
    && unzip -q ./commandlinetools-linux-10406996_latest.zip \
    && mkdir -p $ANDROID_SDK_ROOT/cmdline-tools/latest \
    && mv ./cmdline-tools/* $ANDROID_SDK_ROOT/cmdline-tools/latest/ \
    && rm ./commandlinetools-linux-10406996_latest.zip

# 2) 接受许可证并安装组件
RUN yes | $ANDROID_SDK_ROOT/cmdline-tools/latest/bin/sdkmanager --sdk_root=$ANDROID_SDK_ROOT --licenses
RUN $ANDROID_SDK_ROOT/cmdline-tools/latest/bin/sdkmanager --sdk_root=$ANDROID_SDK_ROOT \
        "platform-tools" \
        "platforms;android-34" \
        "build-tools;34.0.0" \
        "cmake;3.31.1" \
        "ndk;$ANDROID_NDK_VERSION"

# ---------- vcpkg ----------
ENV VCPKG_ROOT=/opt/vcpkg
ENV VCPKG_INSTALL_OPTIONS="--debug"
RUN git clone https://github.com/microsoft/vcpkg.git $VCPKG_ROOT \
 && $VCPKG_ROOT/bootstrap-vcpkg.sh

# ---------- 构建 ----------
WORKDIR /workspace

# ---------- 源码 ----------
COPY ../cmake ./cmake
COPY ../cpp ./cpp
COPY ../platforms/android ./platforms/android
COPY ../ui ./ui
COPY ../vcpkg ./vcpkg
COPY ../CMakeLists.txt ./CMakeLists.txt
COPY ../CMakePresets.json ./CMakePresets.json
COPY ../vcpkg.json ./vcpkg.json
COPY ../vcpkg-configuration.json ./vcpkg-configuration.json

RUN --mount=type=cache,target=/opt/vcpkg/buildtrees \
    --mount=type=cache,target=/opt/vcpkg/downloads \
    --mount=type=cache,target=/opt/vcpkg/installed \
    --mount=type=cache,target=/opt/vcpkg/packages \
    --mount=type=cache,target=/workspace/platforms/android/out \
    --mount=type=cache,target=/root/.gradle \
    dos2unix ./platforms/android/gradlew \
    && chmod +x ./platforms/android/gradlew \
    && ./platforms/android/gradlew -p ./platforms/android assembleDebug

# 添加元数据
LABEL description="Android build environment for Krkr2 project" \
      version="1.0"
