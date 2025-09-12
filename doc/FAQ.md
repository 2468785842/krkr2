## 安装glib失败
### 在Wndows为Android进行跨平台编译时遇到vcpkg无法安装glib时
#### 原因
- 是因为 meson 生成的 install.dat 的路径不正常如
```
D:/source/vcpkg/packages/glib_arm64-android/debug/share/gdb/auto-load/./D:/source/vcpkg/packages/glib_arm64-android/debug/lib
```
- 需要修改`meson`的`minstall.py`

#### 具体步骤
1. 根据`vcpkg的dbg err 信息`锁定是哪一个'meson'如果没有更改`glib`版本那么应该是`1.6.1`
2. 进入`meson`目录,在`VCPKG_ROOT/downloads/tools/meson-1.6.1-哈希值/mesonbuild`,我的路径是`VCPKG_ROOT\vcpkg\downloads\tools\meson-1.6.1-6779de\mesonbuild` 
3. 打开`minstall.py`
4. 替换`install_data`
```py
def install_data(self, d: InstallData, dm: DirMaker, destdir: str, fullprefix: str) -> None:
        

        for i in d.data:
            if not self.should_install(i):
                continue
            if "/./" in i.install_path:
                i.install_path = i.install_path.split('.')[0]+i.install_path.split('\\')[-1]
            fullfilename = i.path
            outfilename = get_destdir_path(destdir, fullprefix, i.install_path)
            
            outdir = os.path.dirname(outfilename)
            try:
                if self.do_copyfile(fullfilename, outfilename, makedirs=(dm, outdir), follow_symlinks=i.follow_symlinks):
                    self.did_install_something = True
            except Exception as e:
                print(f"Error installing {fullfilename} to {outfilename}: {e}")
            self.set_mode(outfilename, i.install_mode, d.install_umask)
```