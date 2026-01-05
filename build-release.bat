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
if not exist "x64\Release" mkdir x64\Release

REM Compile
cl.exe /EHsc /std:c++17 /W3 /O2 /MD /I"%CD%\include" /I"%CD%\GameOfLife" /DNDEBUG /D_CONSOLE /c "%CD%\GameOfLife\main.cpp" /Fo"%CD%\x64\Release\main.obj"

if errorlevel 1 exit /b 1

REM Link
link.exe /OUT:"%CD%\x64\Release\GameOfLife.exe" /LIBPATH:"%CD%\lib" sfml-system.lib sfml-audio.lib sfml-window.lib sfml-graphics.lib "%CD%\x64\Release\main.obj" /SUBSYSTEM:CONSOLE /OPT:REF /OPT:ICF

if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

echo Build successful!

