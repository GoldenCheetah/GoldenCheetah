# Python version configuration - update this when upgrading Python
$PYTHON_EMBED_VERSION="3.11.9"

# Get Python
if (-not (Test-Path 'C:\Python')) {
  $pyurl = "https://www.python.org/ftp/python/$PYTHON_EMBED_VERSION/python-$PYTHON_EMBED_VERSION-embed-amd64.zip"
  Start-FileDownload $pyurl "python-embed.zip"
  Expand-Archive -Path python-embed.zip -DestinationPath C:\Python -Force
  # Enable site import
  mkdir C:\Python\lib\site-packages
  $py_ver = $($env:PYTHON_VERSION -replace '\.', '')
  (Get-Content C:\Python\python$py_ver._pth) -replace '#import site', 'import site' | Set-Content C:\Python\python$py_ver._pth
  # Enable pip in embedded Python
  Start-FileDownload "https://bootstrap.pypa.io/get-pip.py" "get-pip.py"
  C:\Python\python.exe get-pip.py --no-warn-script-location
  # Upgrade pip to ensure you have the latest version
  C:\Python\python -m pip install --upgrade pip
  # Install your project's dependencies from a requirements.txt file
  C:\Python\python -m pip install --upgrade --only-binary :all: -r src\Python\requirements.txt -t C:\Python\lib\site-packages
}

Set-Location src\release

# Copy dependencies
& windeployqt --release GoldenCheetah.exe
Copy-Item "c:\libs\10_Precompiled_DLL\usbexpress_3.5.1\USBXpress\USBXpress_API\Host\x64\SiUSBXp.dll" .
Copy-Item "c:\libs\10_Precompiled_DLL\libsamplerate64\lib\libsamplerate-0.dll" .
Copy-Item "c:\OpenSSL-Win64\bin\lib*.dll" .
Copy-Item "c:\OpenSSL-Win64\license.txt" "OpenSSL License.txt"
xcopy /s /i /e /q C:\Python .
Copy-Item "C:\Python\LICENSE.txt" "PYTHON LICENSE.txt"
Copy-Item "c:\tools\vcpkg\installed\x64-windows\bin\gsl*.dll" .

# ReadMe, license and icon files
Copy-Item "..\Resources\win32\ReadMe.txt" .
"GoldenCheetah is licensed under the GNU General Public License v2" | Set-Content license.txt
Add-Content license.txt "" # echo. equivalent
Get-Content "..\..\COPYING" | Add-Content license.txt
Copy-Item "..\Resources\win32\gc.ico" .

# Installer script
Copy-Item "..\Resources\win32\GC3.8-Master-W64-QT6.nsi" .

# Build the installer
& makensis .\GC3.8-Master-W64-QT6.nsi
Move-Item "GoldenCheetah_v3.8_64bit_Windows.exe" "..\..\GoldenCheetah_v3.8_x64.exe"

Set-Location ..\..
