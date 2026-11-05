# PSBFile 插件

内建 `cpp/plugins/psbfile` 提供 `Plugins.link("psbfile.dll")`、`PSBFile` 和只读 `psb://` 存储介质。
这是对已有解析器和接口的补全，不表示与闭源 Windows 插件的全部功能等价。

## 脚本接口

```javascript
Plugins.link("psbfile.dll");
var file = new PSBFile();
if (!file.load("ui.pimg")) throw "PSB 加载失败";
var layers = file.root.layers;
// root 内的资源值是 Octet；以文件名加载时也注册 psb://ui.pimg/<键路径>。
var memoryCopy = new PSBFile(file.root.embeddedPsb);
```

`new PSBFile()` 创建空实例。构造器也接受一个文件名或 Octet；构造加载失败时保留空实例，
需要判断成功的调用方应使用返回布尔值的 `load(source)`。
`load` 接受一个字符串或 Octet，参数数量/类型错误仍按 TJS 错误码报告。
重复成功加载替换实例数据；加载失败返回 false，并保留上次成功的 root。
`root` 中字典、数组、标量和资源转换为 TJS 值，资源 Octet 拥有自己的数据。

文件加载通过共享 `PSBMediaRegistry` 注册资源，构造加载和 load 的行为一致。
递归字典/数组资源使用键名/数字索引组成路径；资源注册后的数据独立于 PSBFile 实例。
Octet 没有文件名，因此只通过 root 访问资源，不自动创建 URI。
同名资源的后续注册覆盖之前的内容；目前未自动注销已注册的旧路径。
缺失资源和写入请求抛出存储错误，不创建空资源。GetListAt 仅列出该目录下的直接文件。

## 格式与边界

现有解析路径支持 PSB v1–v3、MDF 和 LZ4 包装；v4 尚未支持。
本次合成测试覆盖 v1–v3 和 MDF，实际 PIMG 路径另做游戏启动验证；并未据此宣称全部格式兼容。
新增数组宽度、索引、资源范围、名称循环及对象嵌套深度检查；过深对象（超过 256 层）拒绝加载。
这些检查不是完整模糊测试或不可信文件安全审计，旧的 LZ4/解密路径仍需单独验证。

## 验证

`tools/psbfile/smoke.py` 只生成自制数据，不需要游戏或第三方 DLL：

```sh
LIBGL_ALWAYS_SOFTWARE=1 ALSOFT_DRIVERS=null xvfb-run -a \
  python3 tools/psbfile/smoke.py --engine /absolute/path/to/krkr2
```

覆盖文件/Octet 构造和加载、嵌套资源读取、自动路径目录枚举、缺失资源不污染注册表、
三种 PSB 版本、MDF、失败保留旧数据、
成功替换数据、无 id/非字符串 id 的类型推断及十种无效输入。
必须正常退出并输出 `PSBFILE_SMOKE_OK` 才判定成功。
可配置 `-DKRKR2_ENABLE_PSBFILE_SMOKE=ON` 注册 `psbfile-runtime`，构建应用后执行：

```sh
ctest --test-dir <build> -N -R '^psbfile-runtime$'
LIBGL_ALWAYS_SOFTWARE=1 ALSOFT_DRIVERS=null xvfb-run -a \
  ctest --test-dir <build> -R '^psbfile-runtime$' --output-on-failure --no-tests=error
```

Linux 游戏启动还依赖 TJS 空字符串检查、正确的目录创建返回值和 fstat 缺失文件处理。
这些启动修复与 PSB 格式支持分别验证，未加载全部游戏场景不能称为全程兼容。
