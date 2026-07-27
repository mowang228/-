$module = [System.IO.Path]::GetFileName((Get-Location).Path)
$workspace = $PSScriptRoot
$buildDir = "$workspace\build"
$cmakeCache = "$buildDir\CMakeCache.txt"

function Ensure-CmakeConfig {
    param([string]$Reason)
    Write-Host "=== build/ 未配置${Reason}，运行 cmake 配置 ==="
    if (-not (Test-Path $buildDir)) { $null = New-Item -ItemType Directory -Path $buildDir }
    cmake -S $workspace -B $buildDir -G "MinGW Makefiles"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

if (-not (Test-Path $cmakeCache)) {
    Ensure-CmakeConfig ""
}

if ($module -like 'model*') {
    Write-Host "=== 检测到模块: $module ==="

    # 构建测试目标，如果目标不存在则重新 cmake 配置再试
    cmake --build $buildDir --target "test_$module"
    if ($LASTEXITCODE -ne 0) {
        Ensure-CmakeConfig "或目标不存在"
        cmake --build $buildDir --target "test_$module"
    }

    if ($LASTEXITCODE -eq 0) {
        $env:Path = "D:\Qt\6.11.1\mingw_64\bin;$env:Path"
        & "$buildDir\bin\test_$module.exe"
    }
} else {
    cmake --build $buildDir --target run
}
