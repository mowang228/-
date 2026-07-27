# 一键构建并运行
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$build = Join-Path $root "build"

# 首次使用或 build/ 不存在时自动配置
if (-not (Test-Path $build)) {
    Write-Host "首次构建，配置 CMake..."
    cmake -B $build -G "MinGW Makefiles"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

# 编译并运行
cmake --build $build --target run
