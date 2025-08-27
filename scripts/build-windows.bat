@echo off
setlocal

REM 获取脚本所在目录
set "SCRIPT_DIR=%~dp0"
REM 去掉末尾的反斜杠
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

REM 获取项目根目录
for %%i in ("%SCRIPT_DIR%") do set "PROJECT_ROOT=%%~dpi"
REM 去掉末尾的反斜杠
set "PROJECT_ROOT=%PROJECT_ROOT:~0,-1%"

REM 切换到项目根目录并执行构建
pushd "%PROJECT_ROOT%/.."
cmake --preset="Windows Debug Config" -DDISABLE_TEST=ON
cmake --build --preset="Windows Debug Build"
popd

endlocal