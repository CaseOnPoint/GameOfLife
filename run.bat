@echo off
echo Building...
call build-debug.bat
if %errorlevel% equ 0 (
    echo.
    echo Starting game...
    x64\Debug\GameOfLife.exe
) else (
    echo Build failed! Fix errors and try again.
)

