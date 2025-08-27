@echo off
REM Standalone build script for docking test on Windows
REM This builds the test without modifying CMakeLists.txt

echo Building standalone docking test...

REM Set paths relative to this script
set SCRIPT_DIR=%~dp0
set WORKSPACE_DIR=%SCRIPT_DIR%..
set BUILD_DIR=%WORKSPACE_DIR%\build
set TEST_DIR=%WORKSPACE_DIR%\test

REM Check if main project is built
if not exist "%BUILD_DIR%" (
    echo Error: Main project not built. Please build the main project first:
    echo   cd %WORKSPACE_DIR%
    echo   mkdir build
    echo   cd build
    echo   cmake ..
    echo   cmake --build . --config Release
    exit /b 1
)

REM Check Visual Studio environment
if "%VCINSTALLDIR%"=="" (
    echo Error: Visual Studio environment not set.
    echo Please run this script from a Visual Studio Developer Command Prompt
    exit /b 1
)

REM Compile standalone test
echo Compiling standalone_docking_test.cpp...

cl /EHsc /std:c++17 ^
   /I"%WORKSPACE_DIR%\include" ^
   /I"%WXWIN%\include" ^
   /I"%WXWIN%\include\msvc" ^
   "%TEST_DIR%\standalone_docking_test.cpp" ^
   /link ^
   /LIBPATH:"%BUILD_DIR%\src\docking\Release" ^
   /LIBPATH:"%WXWIN%\lib\vc_x64_lib" ^
   docking.lib ^
   wxmsw32u_core.lib ^
   wxbase32u.lib ^
   kernel32.lib user32.lib gdi32.lib comdlg32.lib ^
   winspool.lib winmm.lib shell32.lib shlwapi.lib ^
   comctl32.lib ole32.lib oleaut32.lib uuid.lib ^
   rpcrt4.lib advapi32.lib version.lib wsock32.lib ^
   wininet.lib ^
   /OUT:"%TEST_DIR%\standalone_docking_test.exe"

if %ERRORLEVEL% EQU 0 (
    echo Build successful!
    echo Run with: %TEST_DIR%\standalone_docking_test.exe
) else (
    echo Build failed!
    exit /b 1
)