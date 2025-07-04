@echo off
echo Building FreeCADNavigation project...

REM 检查是否存在CMakePresets.json
if not exist CMakePresets.json (
    echo Error: CMakePresets.json not found!
    echo Please make sure you are in the project root directory.
    pause
    exit /b 1
)

REM 使用预设配置项目
echo Configuring project with preset...
cmake --preset windows-vcpkg-x64
if %errorlevel% neq 0 (
    echo Configuration failed!
    pause
    exit /b 1
)

REM 构建项目
echo Building project...
cmake --build --preset windows-vcpkg-x64-debug
if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build completed successfully!
pause 