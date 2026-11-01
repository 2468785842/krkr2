## xp3viewer工具

### 解包`xp3`文件

```
./xp3 --help                                    
Usage: xp3 [--help] [--version] [--output VAR] files...

Positional arguments:
files          input files path [nargs: 1 or more]

Optional arguments:
-h, --help     shows help message and exits
-v, --version  prints version information and exits
-o, --output   output dir path
```
### 独立构建与验证

见 [仅构建 XP3 解包工具](../../docs/building.md#仅构建-xp3-解包工具)。关闭应用和引擎测试后，无需安装图形、视频和通用归档依赖。

`python tools/xp3/smoke_test.py <xp3可执行文件>` 会生成自制 XP3 并验证原始、zlib 解包内容及无效签名处理。
