# OpenCV ImageOps 构建配置

本目录基于 microsoft/vcpkg 的 OpenCV **4.7.0#6** port，未升级 OpenCV，也未修改算法源码。

- 固定 registry baseline：`aa40adda5352e87655b8583cfb2451d5e9e276fd`。
- 版本表 `versions/o-/opencv4.json` 中的 port git tree：`d435a87c7a3f4dd38d623503d5200abe65c249d1`。
- 原始 portfile blob：`ffd843d35d2268bf492660fb611c81d9beb64a42`。
- 来源：<https://github.com/microsoft/vcpkg/tree/aa40adda5352e87655b8583cfb2451d5e9e276fd>，
  以上述版本表中的旧 port tree 为准，不能用 baseline 下最新 ports/opencv4 替代。
- vcpkg port/补丁保留原文件，工具代码原 MIT 许可见 `LICENSE.vcpkg`；下载的 OpenCV 源码
  及分发许可仍由原 port 的 `vcpkg_install_copyright` 处理（OpenCV Apache-2.0）。

项目增量仅为 `imageops` feature、portfile 对 `imageops-profile.cmake` 的调用和该配置文件。
选择该 feature 时设置 `BUILD_LIST=core,imgproc`，仅允许 core/imageops/openmp，
冲突功能明确拒绝。未选择 imageops 时保留原 port 的功能选择。
OpenMP 的平台规则、CPU baseline/dispatch、优化及所有原补丁均沿用固定版本。

本仓库 engine 与独立 imageops feature 都选择该配置。图像算法、线程调度及 ARM Linear
回退规则仍见 [ImageOps](../../../docs/subsystems/imageops.md)。
