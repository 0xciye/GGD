@echo off
if /I "%VSCMD_ARG_TGT_ARCH%"=="x64" (
    where cl >nul 2>&1
    if not errorlevel 1 exit /b 0
)
set "task_vswhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "task_vsroot="
if not exist "%task_vswhere%" (
    echo [ERROR] Install Visual Studio C++ build tools with the Windows SDK.
    exit /b 1
)
for /f "usebackq delims=" %%I in (`"%task_vswhere%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "task_vsroot=%%I"
if not defined task_vsroot (
    echo [ERROR] No compatible MSVC x64 toolchain found.
    exit /b 1
)
call "%task_vsroot%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
exit /b %errorlevel%
