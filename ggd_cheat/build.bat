@echo off
setlocal
if not "%~1"=="" if /I not "%~1"=="internal" (
    echo Usage: build.bat [internal]
    exit /b 2
)
echo ========================================================
echo  Building 34Cheat - Standalone Single-EXE Internal Cheat
echo ========================================================

call "%~dp0setup_msvc.bat"
if errorlevel 1 (
    echo [ERROR] Failed to initialize MSVC x64.
    exit /b 1
)

cd /d "%~dp0"

echo [1/3] Compiling 34Cheat.dll (Internal + Static CRT)...
cl /nologo /std:c++17 /utf-8 /MT /O2 /DNDEBUG /Zi /Fd:InternalCompile.pdb /LD /EHsc /I . /I imgui /I minhook Internal.cpp imgui\imgui.cpp imgui\imgui_draw.cpp imgui\imgui_tables.cpp imgui\imgui_widgets.cpp imgui\imgui_impl_win32.cpp imgui\imgui_impl_dx11.cpp minhook\buffer.c minhook\hook.c minhook\trampoline.c minhook\hde\hde32.c minhook\hde\hde64.c /Fe:34Cheat.dll /link /DEBUG:FULL /INCREMENTAL:NO /OPT:REF /OPT:ICF /PDB:34Cheat.Internal.pdb /PDBALTPATH:34Cheat.Internal.pdb d3d11.lib dxgi.lib user32.lib gdi32.lib psapi.lib advapi32.lib
if errorlevel 1 (
    echo [ERROR] Failed to build 34Cheat.dll!
    exit /b 1
)
del *.obj *.exp *.lib 2>nul
echo [ OK ] 34Cheat.dll built successfully.

echo [2/3] Embedding 34Cheat.dll into C++ byte array for single-EXE bundle...
python embed.py
if errorlevel 1 (
    echo [ERROR] Failed to embed 34Cheat.dll!
    exit /b 1
)

echo [3/3] Compiling 34Cheat.exe (Single Standalone Executable /MT)...
rc /nologo Loader.rc
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /utf-8 /MT /O2 /DNDEBUG /Zi /Fd:LoaderCompile.pdb /EHsc /I imgui Injector.cpp Loader.res imgui\imgui.cpp imgui\imgui_draw.cpp imgui\imgui_tables.cpp imgui\imgui_widgets.cpp imgui\imgui_impl_win32.cpp imgui\imgui_impl_dx11.cpp /Fe:34Cheat.exe /link /SUBSYSTEM:WINDOWS /MANIFEST:NO /DEBUG:FULL /INCREMENTAL:NO /OPT:REF /OPT:ICF /PDB:34Cheat.Loader.pdb /PDBALTPATH:34Cheat.Loader.pdb d3d11.lib dxgi.lib dwmapi.lib user32.lib gdi32.lib advapi32.lib shell32.lib ole32.lib comdlg32.lib bcrypt.lib windowscodecs.lib
if errorlevel 1 (
    echo [ERROR] Failed to build 34Cheat.exe!
    exit /b 1
)
copy /y 34Cheat.exe 34Cheat_Injector.exe >nul
if errorlevel 1 exit /b 1
del *.obj 2>nul
echo [ OK ] 34Cheat.exe built successfully.

copy /y 34Cheat.dll ..\34Cheat.dll >nul
if errorlevel 1 exit /b 1
copy /y 34Cheat.exe ..\34Cheat.exe >nul
if errorlevel 1 exit /b 1
if not exist ..\release mkdir ..\release
if errorlevel 1 exit /b 1
copy /y 34Cheat.exe ..\release\34Cheat.exe >nul
if errorlevel 1 exit /b 1
del *.obj 2>nul

echo ========================================================
echo  BUILD COMPLETE!
echo  Single-File Standalone Executable:
echo    - 34Cheat.exe
echo  Additional artifacts:
echo    - 34Cheat.dll
echo    - 34Cheat_Injector.exe
echo ========================================================
