@echo off
setlocal
cd /d "%~dp0"
call "%~dp0setup_msvc.bat"
if errorlevel 1 exit /b 1
if not exist tests\out mkdir tests\out
pushd tests\out
cl /nologo /std:c++17 /utf-8 /MT /Od /EHsc /I ..\.. /I ..\..\imgui /I ..\..\minhook ..\InternalTests.cpp ..\..\imgui\imgui.cpp ..\..\imgui\imgui_draw.cpp ..\..\imgui\imgui_tables.cpp ..\..\imgui\imgui_widgets.cpp ..\..\imgui\imgui_impl_win32.cpp ..\..\imgui\imgui_impl_dx11.cpp ..\..\minhook\buffer.c ..\..\minhook\hook.c ..\..\minhook\trampoline.c ..\..\minhook\hde\hde32.c ..\..\minhook\hde\hde64.c /Fe:InternalTests.exe /link d3d11.lib dxgi.lib user32.lib gdi32.lib psapi.lib advapi32.lib dwmapi.lib ole32.lib windowscodecs.lib
if errorlevel 1 exit /b 1
InternalTests.exe
set test_result=%errorlevel%
popd
exit /b %test_result%
