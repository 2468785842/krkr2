<#
.SYNOPSIS
    KRKR2 Windows 构建脚本
.DESCRIPTION
    自动化设置构建环境并编译项目
.PARAMETER Clean
    清理所有构建产物和临时文件
#>
param(
    [switch]$Clean = $false
)

$ErrorActionPreference = "Stop"

# 环境变量和路径设置
$env:VCPKG_ROOT = "$PSScriptRoot\.vcpkg"
if($env:GITHUB_REPOSITORY_OWNER){
    $env:VCPKG_BINARY_SOURCES = "clear;nuget,https://nuget.pkg.github.com/$env:GITHUB_REPOSITORY_OWNER/index.json,readwrite"
}
$env:VCPKG_INSTALL_OPTIONS = "--debug"

function Install-Dependencies {
    Write-Host "正在安装系统依赖..." -ForegroundColor Cyan
    
    # 1. 安装 Chocolatey (如果尚未安装)
    if (-not (Get-Command choco -ErrorAction SilentlyContinue)) {
        Set-ExecutionPolicy Bypass -Scope Process -Force
        [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
        Invoke-Expression ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
    }
    
    # 2. 安装必要工具
    choco install -y git cmake python3 nasm visualstudio2022-workload-vctools --no-progress
    
    # 3. 安装 winflexbison
    $flexBisonUrl = "https://github.com/lexxmark/winflexbison/releases/download/v2.5.25/win_flex_bison-2.5.25.zip"
    $flexBisonPath = "C:\winflexbison"
    
    if (-not (Test-Path $flexBisonPath)) {
        $tempFile = "$env:TEMP\winflexbison.zip"
        Invoke-WebRequest -Uri $flexBisonUrl -OutFile $tempFile
        Expand-Archive -Path $tempFile -DestinationPath $flexBisonPath -Force
        [Environment]::SetEnvironmentVariable("PATH", "$flexBisonPath;$env:PATH", "Machine")
        $env:PATH = "$flexBisonPath;$env:PATH"
    }
}

function Setup-Vcpkg {
    Write-Host "正在设置 vcpkg..." -ForegroundColor Cyan
    
    # 1. 克隆 vcpkg (如果不存在)
    if (-not (Test-Path $env:VCPKG_ROOT)) {
        git clone https://github.com/microsoft/vcpkg.git $env:VCPKG_ROOT
    }
    
    # 2. 引导 vcpkg
    & "$env:VCPKG_ROOT\bootstrap-vcpkg.bat"
    
    # 3. 集成到系统
    & "$env:VCPKG_ROOT\vcpkg" integrate install
    
    # 4. 添加 NuGet 源 (如果提供了 API 密钥)
    if ($env:NUGET_API_KEY) {
        $nuget = & "$env:VCPKG_ROOT\vcpkg" fetch nuget | Select-Object -First 1
        & $nuget sources add `
            -Source "https://nuget.pkg.github.com/$env:GITHUB_REPOSITORY_OWNER/index.json" `
            -StorePasswordInClearText `
            -Name GitHubPackages `
            -UserName "$env:GITHUB_ACTOR" `
            -Password "$env:NUGET_API_KEY"
        
        & $nuget setapikey "$env:NUGET_API_KEY" `
            -Source "https://nuget.pkg.github.com/$env:GITHUB_REPOSITORY_OWNER/index.json"
    }
}

function Invoke-Build {
    Write-Host "开始构建项目..." -ForegroundColor Green
    
    # 1. 创建构建目录
    $buildDir = "$PSScriptRoot\build"
    if (-not (Test-Path $buildDir)) {
        New-Item -ItemType Directory -Path $buildDir | Out-Null
    }
    
    # 2. 配置 CMake
    Push-Location $buildDir
    try {
        cmake .. --preset="Windows Debug Config" -D DISABLE_TEST=ON
        if ($LASTEXITCODE -ne 0) { throw "CMake 配置失败" }
        
        # 3. 执行构建
        cmake --build . --config Debug
        if ($LASTEXITCODE -ne 0) { throw "构建失败" }
    }
    finally {
        Pop-Location
    }
}

function Clean-Build {
    Write-Host "正在清理构建产物..." -ForegroundColor Yellow
    
    $pathsToClean = @(
        "$PSScriptRoot\build",
        "$PSScriptRoot\out",
        "$PSScriptRoot\.vcpkg",
        "$env:TEMP\winflexbison.zip"
    )
    
    foreach ($path in $pathsToClean) {
        if (Test-Path $path) {
            Remove-Item -Recurse -Force $path
        }
    }
}

# 主执行逻辑
try {
    if ($Clean) {
        Clean-Build
        exit 0
    }
    
    # 安装依赖
    Install-Dependencies
    
    # 设置 vcpkg
    Setup-Vcpkg
    
    # 执行构建
    Invoke-Build
    
    Write-Host "构建成功完成！" -ForegroundColor Green
    exit 0
}
catch {
    Write-Host "构建失败: $_" -ForegroundColor Red
    exit 1
}