# PowerShell build script for Release configuration

# Find Visual Studio installation
$vswhere = "${env:ProgramFiles}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
}

if (-not (Test-Path $vswhere)) {
    Write-Host "Error: Visual Studio not found. Please install Visual Studio with C++ support." -ForegroundColor Red
    exit 1
}

$vsPath = & $vswhere -latest -property installationPath

if (-not $vsPath) {
    Write-Host "Error: Visual Studio installation path not found." -ForegroundColor Red
    exit 1
}

# Setup MSVC environment
$vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcvars)) {
    Write-Host "Error: vcvars64.bat not found at: $vcvars" -ForegroundColor Red
    exit 1
}

# Create output directory
$outputDir = Join-Path $PSScriptRoot "x64\Release"
if (-not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
}

# Get script directory
$scriptDir = $PSScriptRoot

# Call vcvars64.bat and then compile/link in the same environment
$buildScript = @"
@echo off
call "$vcvars"
cd /d "$scriptDir"
if not exist "x64\Release" mkdir x64\Release
cl.exe /EHsc /std:c++17 /W3 /O2 /MD /I"$scriptDir\include" /I"$scriptDir\GameOfLife" /DNDEBUG /D_CONSOLE /c "$scriptDir\GameOfLife\main.cpp" /Fo"$scriptDir\x64\Release\main.obj"
if errorlevel 1 exit /b 1
link.exe /OUT:"$scriptDir\x64\Release\GameOfLife.exe" /LIBPATH:"$scriptDir\lib" sfml-system.lib sfml-audio.lib sfml-window.lib sfml-graphics.lib "$scriptDir\x64\Release\main.obj" /SUBSYSTEM:CONSOLE /OPT:REF /OPT:ICF
if errorlevel 1 exit /b 1
echo Build successful!
"@

# Write temporary batch file
$tempBat = Join-Path $env:TEMP "build-release-temp.bat"
$buildScript | Out-File -FilePath $tempBat -Encoding ASCII

# Execute the batch file
& cmd.exe /c $tempBat

$buildResult = $LASTEXITCODE

# Clean up
Remove-Item $tempBat -ErrorAction SilentlyContinue

if ($buildResult -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    exit $buildResult
}

Write-Host "Build successful!" -ForegroundColor Green

