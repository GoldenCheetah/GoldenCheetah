# Windows After Build Script
$ErrorActionPreference = "Stop"

# Set PUBLISH_BINARIES according to commit message
Set-AppveyorBuildVariable -Name 'PUBLISH_BINARIES' -Value "false"
if ($env:APPVEYOR_REPO_COMMIT_MESSAGE_EXTENDED -Match "\[publish binaries\]") {
    Set-AppveyorBuildVariable -Name 'PUBLISH_BINARIES' -Value "true"
}

Set-Location src\release

# Copy dependencies
& windeployqt --release GoldenCheetah.exe
Copy-Item "c:\libs\10_Precompiled_DLL\usbexpress_3.5.1\USBXpress\USBXpress_API\Host\x64\SiUSBXp.dll" .
Copy-Item "c:\libs\10_Precompiled_DLL\libsamplerate64\lib\libsamplerate-0.dll" .
Copy-Item "c:\OpenSSL-Win64\bin\lib*.dll" .
Copy-Item "c:\OpenSSL-Win64\license.txt" "OpenSSL License.txt"
Copy-Item -Recurse "C:\Python" .
Copy-Item "C:\Python\LICENSE.txt" "PYTHON LICENSE.txt"
Copy-Item "c:\tools\vcpkg\installed\x64-windows\bin\gsl*.dll" .

# ReadMe, license and icon files
Copy-Item "..\Resources\win32\ReadMe.txt" .
"GoldenCheetah is licensed under the GNU General Public License v2" | Set-Content license.txt
Add-Content license.txt "" # echo. equivalent
Get-Content "..\..\COPYING" | Add-Content license.txt
Copy-Item "..\Resources\win32\gc.ico" .

# Installer script
Copy-Item "..\Resources\win32\GC3.7-Master-W64-QT6.nsi" .

# Build the installer
& makensis GC3.7-Master-W64-QT6.nsi
Move-Item "GoldenCheetah_v3.7.1_64bit_Windows.exe" "..\..\GoldenCheetah_v3.7_x64.exe"

Set-Location ..\..
