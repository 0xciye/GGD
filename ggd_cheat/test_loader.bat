@echo off
setlocal
cd /d "%~dp0"
call "%~dp0setup_msvc.bat"
if errorlevel 1 exit /b 1
if not exist tests\out mkdir tests\out
pushd tests\out
cl /nologo /std:c++17 /utf-8 /MT /Od /EHsc ..\LoaderTarget.cpp /Fe:LoaderTarget.exe /link user32.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /utf-8 /MT /Od /EHsc /LD ..\LoaderRuntime.cpp /Fe:LoaderRuntime.dll /link user32.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /utf-8 /MT /Od /EHsc ..\LoaderTests.cpp /Fe:LoaderTests.exe /link user32.lib advapi32.lib shell32.lib ole32.lib bcrypt.lib
if errorlevel 1 exit /b 1
LoaderTests.exe
set test_result=%errorlevel%
popd
exit /b %test_result%
