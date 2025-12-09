# Windows Build Script
$ErrorActionPreference = "Stop"

# Initial QMake
& qmake.exe build.pro -r -spec win32-msvc

# Build QWT
if (-not (Test-Path "qwt\lib\qwt.lib")) {
    & c:\jom\jom.exe -j1 sub-qwt
    if ($LASTEXITCODE -ne 0) { exit 1 }
}

# Build Source
& c:\jom\jom.exe -j2 sub-src
