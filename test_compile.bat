@echo off
echo Testing compilation fixes...

cd /d D:\source\repos\wxcoin\build

echo Building docking library...
msbuild src\docking\docking.vcxproj /p:Configuration=Debug /p:Platform=x64 /v:minimal

if %errorlevel% neq 0 (
    echo Build failed!
    exit /b 1
)

echo Build succeeded!
exit /b 0