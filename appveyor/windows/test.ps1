# Windows Test Script
$ErrorActionPreference = "Stop"

# Version check
cmd /c "src\release\GoldenCheetah --version 2>GCversionWindows.txt"
cmd /c "git log -1 >> GCversionWindows.txt"

# Hash
CertUtil -hashfile GoldenCheetah_v3.7_x64.exe sha256 | Select-Object -First 2 | Add-Content GCversionWindows.txt

# Display
Get-Content GCversionWindows.txt
