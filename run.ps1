# Quick run script - builds and runs the game
Write-Host "Building..." -ForegroundColor Yellow
& "$PSScriptRoot\build-debug.ps1"

if ($LASTEXITCODE -eq 0) {
    Write-Host "`nStarting game..." -ForegroundColor Green
    & "$PSScriptRoot\x64\Debug\GameOfLife.exe"
} else {
    Write-Host "Build failed! Fix errors and try again." -ForegroundColor Red
}

