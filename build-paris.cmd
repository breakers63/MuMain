@echo off
setlocal
cd /d "%~dp0"
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 exit /b 1
chcp 65001 >nul
set "VSLANG=3082"
set "VCPKG_ROOT=C:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg"
set "CMAKE=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "BUILD_TARGET=%~1"
if "%BUILD_TARGET%"=="" set "BUILD_TARGET=Main"
"%CMAKE%" --preset windows-x64 -DCMAKE_DEFAULT_BUILD_TYPE=Release -DVCPKG_INSTALLED_DIR=C:/Users/WOoS/SrcLadoClienteMuMain-SDL3/out/build/windows-x64/vcpkg_installed -DRMLUI_THIRDPARTY_CONTAINERS=OFF > configure-paris.log 2>&1
if errorlevel 1 exit /b 1
"%CMAKE%" --build out/build/windows-x64 --config Release --target %BUILD_TARGET% --parallel 4 > build-paris.log 2>&1
exit /b %errorlevel%
