@echo off
chcp 65001 >nul
cd /d "%~dp0"
if not exist build\ (
    echo 首次构建，配置 CMake...
    cmake -B build -G "MinGW Makefiles"
    if errorlevel 1 pause & exit /b
)
cmake --build build --target run
if errorlevel 1 pause
