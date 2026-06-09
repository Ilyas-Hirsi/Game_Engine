# Build on Windows (PowerShell)
#
# Usage:
#   .\scripts\build.ps1 -Run
#   .\scripts\build.ps1 -Clean -Run

param(
    [string]$Preset = "debug",
    [switch]$Run,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$VsMsVc = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC"

$env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" +
            [System.Environment]::GetEnvironmentVariable("Path", "User")

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Error @"
'cmake' not found. Install it, then restart PowerShell:

  winget install Kitware.CMake
"@
}

if (-not (Test-Path $VsMsVc)) {
    Write-Error @"
Visual Studio 2022 is installed but the C++ compiler is missing.

1. Open Start -> 'Visual Studio Installer'
2. Click Modify on 'Visual Studio Community 2022'
3. Check 'Desktop development with C++'
4. Click Install / Update
5. Restart PowerShell, then run:

  .\scripts\build.ps1 -Run
"@
}

Write-Host "Using preset: $Preset"

if ($Clean -and (Test-Path "$Root\build")) {
    Remove-Item -Recurse -Force "$Root\build"
    Write-Host "Removed build/"
}

cmake --preset $Preset -S $Root
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

cmake --build --preset $Preset
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($Run) {
    $exe = "$Root\build\Debug\Game_Engine.exe"
    if (-not (Test-Path $exe)) {
        $exe = "$Root\build\Game_Engine.exe"
    }
    if (-not (Test-Path $exe)) {
        Write-Error "Executable not found under build/."
    }
    Write-Host "Running $exe"
    & $exe
}
