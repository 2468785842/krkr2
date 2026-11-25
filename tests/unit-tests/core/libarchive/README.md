# 静态归档依赖测试

此目标只链接 Catch2 与 `LibArchive::LibArchive`，避免由引擎其他模块偶然补齐 codec 链接。
测试 gzip、bzip2、xz、LZ4、zstd 的真实 TAR 往返，并检查 XAR 的平台能力。

`fixture.xar` 是自制容器：大端 `>IHHQQI` 头（magic=0x78617221、size=28、version=1、checksum=0），
随后是 zlib 压缩的 XML TOC 和原始 UTF-8 数据 `archive dependency contract 中文\n`。
TOC 只有一个 `fixture.txt` 常规文件，data offset=0、size/length 为 payload 字节数、encoding 为
`application/octet-stream`，不使用第三方素材。样本另经系统 libarchive 的 XAR 读取器验证。
Windows CNG/Apple CommonCrypto 支持哈希时要求解包成功；其他平台返回不支持时检查实际错误，
不能将 XML 库存在等同于 XAR 格式已启用。
