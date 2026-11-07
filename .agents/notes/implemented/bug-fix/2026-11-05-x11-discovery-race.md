# X11 窗口枚举的销毁竞态

Status: implemented

## 问题与复现

文档提交 `a2393ba` 的 Headless [41937321097](https://github.com/2468785842/krkr2/actions/runs/41937321097)
仅 SDL3 job `126948953418` 失败，其他 12 个 job 通过。真实 X11 验证时 XGetWindowProperty
访问已销毁的 `0x400031`，Xlib 默认处理器报告 BadWindow、major opcode 20，直接退出 1。
这不是新文本模块或游戏运行器的失败；随后 `380e4ef` 的同一 SDL3 job 又通过，符合时序竞态。

`XQueryTree` 的子窗口快照不保证窗口在后续 PID 属性或可见性查询时仍存活。
仅检查函数返回值无效，因为默认 Xlib 处理器会先终止进程。
新增真实 X server 回归在 GetProperty 前、GetProperty 后/GetWindowAttributes 前分别销毁候选，
不伪造 Xlib 返回值；还检查实际 BadAtom 不得被忽略，并保留未映射窗口就绪检查。
修复前先运行新测试，首项属性后销毁用例以 BadWindow/opcode 3 直接退出 1，
复现日志 `out/x11-discovery-race/baseline-tests.log`。

## 修改及本地验证

`verify_x11.py::window_query` 用有生命周期的 ctypes 回调临时接管单个候选查询，
前后 XSync 排空请求，finally 恢复原全局处理器。
只忽略本连接、当前候选、GetProperty/GetWindowAttributes 的 BadWindow；其他错误抛出 RuntimeError。
普通输入、焦点、调整大小及关闭请求不处于该容错范围，原有事件和像素断言未放宽。
该工具为单线程独立进程，不将此全局 Xlib 处理器方案推广到游戏后端或多线程宿主。

本机没有 Xvfb，使用已安装的 Xorg dummy 驱动在私有 `:92` 上启动独立 X server，
配置和日志位于 `out/x11-discovery-race/`，未修改系统配置或安装包。

```sh
DISPLAY=:92 python -m unittest discover -s tools/sdl3-prototype -p 'test_*.py' -v
python -m py_compile tools/sdl3-prototype/verify_x11.py tools/sdl3-prototype/test_verify_x11.py
git diff --check
```

4 项实际 X server 用例全部通过，Python 编译与 diff 检查通过。
Windows 默认模式子批次另行验证，不将此探针修复计作生产 SDL3/IME 能力。

## 远端补充

源提交 `8332194` 的 Headless [41941969717](https://github.com/2468785842/krkr2/actions/runs/41941969717)，
SDL3 job `126964760491` 成功：Xvfb 下实际发现并通过上述 4 项，
队列键盘/鼠标、原生 X11 键盘/鼠标、关闭否决及故意错误绘制的负向验证全部保留并通过。
4 份正向报告均 passed=true、exit_code=0、forced_stop=false。
