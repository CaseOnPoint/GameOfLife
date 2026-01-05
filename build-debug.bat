@echo off
REM Find Visual Studio installation
set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -property installationPath`) do set "VSINSTALLDIR=%%i"

if "%VSINSTALLDIR%"=="" (
    echo Error: Visual Studio not found. Please install Visual Studio with C++ support.
    exit /b 1
)

REM Setup MSVC environment
call "%VSINSTALLDIR%\VC\Auxiliary\Build\vcvars64.bat"

REM Create output directory
if not exist "x64\Debug" mkdir x64\Debug

REM Compile
cl.exe /EHsc /std:c++17 /W3 /Zi /Od /MDd /I"%CD%\include" /I"%CD%\GameOfLife" /D_DEBUG /D_CONSOLE /c "%CD%\GameOfLife\main.cpp" /Fo"%CD%\x64\Debug\main.obj"

if errorlevel 1 exit /b 1

REM Link
link.exe /OUT:"%CD%\x64\Debug\GameOfLife.exe" /LIBPATH:"%CD%\lib" sfml-system-d.lib sfml-audio-d.lib sfml-window-d.lib sfml-graphics-d.lib "%CD%\x64\Debug\main.obj" /SUBSYSTEM:CONSOLE /DEBUG

if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

echo Build successful!

