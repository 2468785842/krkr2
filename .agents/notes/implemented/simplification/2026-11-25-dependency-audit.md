# 原生依赖审计与首轮清理

Status: implemented

## 范围与基线

用户授权进行依赖瘦身审计。基线 dev `b6cb9990a84e31eafc18ee137bef337d77d0be59`，
在 `chore/dependency-audit` 分支修改；以下本机验证对应该分支提交前的工作区。
不自动切换 macOS/Android 宿主，也不删除未验收的素材格式或游戏平台功能。
实际消费者、条件依赖与后续平台批次见[审计报告](../../../../docs/subsystems/dependency-audit.md)。

## 已实现

- 新增 SDK 清单审计工具，汇总实际 installed ports、feature、反向依赖、文件逻辑大小、缺失文件、
  编译单元/OpenMP 选项和应用哈希；拒绝空状态、缺失/歧义清单及越界路径，不用文本命中猜测“无用”。
  native 守卫同步按 installed 状态过滤：已 purge 的 GLM 记录仍可留在 status 中，不能据此误计已安装包。
- 移除无人消费的 vcpkg GLM；原生 WebP 只保留 SIMD、decoder 链接；音频去掉 Vorbis encoder 链接。
- base 去掉未消费的 archive/unrar/zstd 直接边，storage 保留实际 unrar/archive 消费。
- libarchive overlay 明确声明/传递 zlib、bzip2、liblzma、LZ4、zstd、LibXml2 及 Windows bcrypt；
  port-version=2，不再编译四个归档 CLI。没有通过关闭压缩或 XAR 来绕过静态链接问题。
- 新增只链接 Catch2/LibArchive 导入目标的实际压缩往返测试，以及 WebP/Vorbis 真实应用探针。
  静态消费者随 app 测试注册；Linux codec 运行测试仅在真实 SDL3 script-smoke 配置注册。
  Windows 原生 app 的注册数量由 67 增为 68；Linux full 由 119 增为 121。
- 编码探针的灰色图片、透明图、正弦波和 XAR XML TOC 均为自制素材，不使用商业游戏资源。

## 失败、定位与修正

第一次删除 base 冗余归档链接后，实际链接在 archive 的 XZ/ZIP reader 上报
`undefined reference to lzma_alone_decoder`。暴露原 overlay 仅声明 XML、其他 codec 靠 SDK
偶然可见和重复链接补齐的问题；修复为归档自身的完整依赖，不恢复无关目标的直接边。

尝试用 XAR 写入器产生测试包时返回 ARCHIVE_WARN；改用自制读取样本后，Linux SDK 仍返回不支持。
源码与配置证据表明：该平台 OpenSSL=OFF，缺少 XAR 必需的 MD5/SHA1 后端；XML 头可用不等于 XAR 可用。
测试因此核对平台的实际支持状态：Windows/Apple 要求支持并读取 XAR；其他平台支持时读取，不支持时核对明确错误。
XAR 样本另经带哈希后端的系统 libarchive 实际解包核对有效；没有把 SDK 中不支持的格式伪装成读取成功。
早期失败日志位于 `out/dependency-audit/base-build.log`、`xar-test.log`，最终结果单独记录如下。

## 本机证据

环境：Linux、GCC 14.2、CMake 3.31.1、Ninja、Release、SDL3-only，PSD ON、UI test driver OFF、tools OFF。
所有构建、临时样本、清单和测量原始输出都放 `out/dependency-audit/`，SDK 为原独立的 `out/native-sdk`。

| 项目 | 前后结果 |
|---|---|
| SDK port/架构组合 | 164 → 163；没有安装 GLM 或 Cocos |
| SDK 文件逻辑字节 | 1,610,123,432 → 1,600,154,469，减少 9,968,963（约 9.51 MiB）；缺失文件 0 |
| 应用字节 | 121,353,680 → 121,353,648，仅 32 字节链接布局差异，不宣称运行包明显缩小 |
| 编译单元 | 368 → 369，新项为隔离的 libarchive consumer 测试 |
| 构建及 CTest | 普通应用完整构建通过；121/121 项通过（Xvfb），实际注册与执行一致 |
| Python 审计测试 | `python -m unittest discover -s scripts/tests -v`，8/8 通过 |
| 依赖守卫 | foundation 11 目标、完整 native 编译/SDK/动态库/资源审计通过 |
| CLI | 原生真实程序 15 项 help/错误退出码通过 |
| 发布打包 | 审计通过后生成约 60 MiB 的 tar.gz；未据此宣称相对原基线包体收益 |
| 软件图像场景 | imageops_repro 通过，图像 SHA256 `395f5bb131813c513681734ed29859ea0682c6640e0fe80bebfc47e1375d711d` |
| WebP/Vorbis | 有损/无损/alpha 的尺寸/RGB/透明度通过；实际 PCM 440 Hz 振幅约 4038，正常退出，没有强停 |

最终发布包 SHA256：`0f54eff6e89ce1584d00f7627ec00a022997ff7d310f3fe4528c6c1805edde6b`。
最终应用 SHA256：`096e0a34c41eb14051542c7011669e465bdd9b6e8a1154374314d0a49eb30dc3`。
初批移除 GLM/WebP 编码边时前后应用均为
`500a64149b37455c11ab9e9046464c5ee8e5b48d61d005c185fa845ffc33eb3a`，之后归档依赖重新排序才改变布局。

单库构建微基准：同一 libarchive 3.7.7 源码与 SDK，Release/Ninja/3 并发、四个新构建目录，
CLI ON/OFF/OFF/ON；不计配置时间，四次 build 分别 22.093/21.081/20.084/20.505 秒。
样本区间重叠，不据此宣称稳定提速或整仓冷构建收益。配置/构建参数和原始日志为
`out/dependency-audit/build-times.json` 与 `bench-*.log`。

## 剩余边界

新改动未在本机执行 Windows/macOS/Android 或旧 Cocos 完整宿主。
Linux 当前 XAR 不支持属于既有哈希后端边界；Windows CNG 的传递链接由代码明确补齐，仍需 CI 实际验收。
原 Windows 商业游戏验收属于基线，不当成本次改动后又跑过商业游戏。
OpenMP 的真实目标选项归属另记[调查](../../proposed/architecture/2026-11-25-openmp-target-scope.md)，本次没有改变线程语义。
