# ---------- 基础 ----------
FROM ubuntu:22.04 AS build-tools
ENV DEBIAN_FRONTEND=noninteractive

# ---------- 系统工具 ----------
RUN apt update && apt install -y --no-install-recommends \
        ca-certificates \
        wget curl git zip unzip tar \
        # 编译链 \
        build-essential gcc g++ ninja-build nasm yasm python3 python3-jinja2 bison \
        autoconf automake libtool pkg-config libltdl-dev \
        # X11 / OpenGL / 其它三方库头文件 \
        libxft-dev libxext-dev libxxf86vm-dev libx11-dev libxmu-dev \
        libglu1-mesa-dev libgl2ps-dev libxi-dev libzip-dev libpng-dev \
        libcurl4-gnutls-dev libfontconfig1-dev libsqlite3-dev libglew-dev \
        libssl-dev libgtk-3-dev binutils \
        # ccache \
        ccache \
    && update-ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# ---------- CMake 3.31 ----------
# 官方预编译 tarball
ARG CMAKE_VER=3.31.6
RUN set -eux; \
  mkdir -p /opt; \
  wget https://github.com/Kitware/CMake/releases/download/v${CMAKE_VER}/cmake-${CMAKE_VER}-linux-x86_64.tar.gz; \
  ls -lh cmake-${CMAKE_VER}-linux-x86_64.tar.gz; \
  tar -xf cmake-${CMAKE_VER}-linux-x86_64.tar.gz -C /opt; \
  ln -sf /opt/cmake-${CMAKE_VER}-linux-x86_64/bin/* /usr/local/bin; \
  rm cmake-${CMAKE_VER}-linux-x86_64.tar.gz

# ---------- vcpkg ----------
ENV VCPKG_ROOT=/opt/vcpkg
ENV VCPKG_INSTALL_OPTIONS="--debug"
RUN git clone https://github.com/microsoft/vcpkg.git ${VCPKG_ROOT} \
    && ${VCPKG_ROOT}/bootstrap-vcpkg.sh

FROM build-tools AS generate
WORKDIR /workspace
# ---------- 源码 ----------
COPY ../cmake ./cmake
COPY ../cpp ./cpp
COPY ../platforms/linux ./platforms/linux
COPY ../ui ./ui
COPY ../vcpkg ./vcpkg
COPY ../CMakeLists.txt ./CMakeLists.txt
COPY ../CMakePresets.json ./CMakePresets.json
COPY ../vcpkg.json ./vcpkg.json
COPY ../vcpkg-configuration.json ./vcpkg-configuration.json

# ---------- 构建 ----------
RUN --mount=type=cache,target=/opt/vcpkg/buildtrees \
    --mount=type=cache,target=/opt/vcpkg/downloads \
    --mount=type=cache,target=/opt/vcpkg/installed \
    --mount=type=cache,target=/opt/vcpkg/packages \
    --mount=type=cache,target=/workspace/out \
    cmake --preset="Linux Debug Config" -DENABLE_TESTS=OFF -DBUILD_TOOLS=OFF \
    && cmake --build --preset="Linux Debug Build"
# 添加元数据
LABEL description="Linux build environment for Krkr2 project" \
      version="1.0"
