# Windows Install Script
$ErrorActionPreference = "Stop"

# Get the libraries
if (-not (Test-Path "gc-ci-libs.zip")) {
    appveyor DownloadFile "https://github.com/GoldenCheetah/WindowsSDK/releases/download/v0.1.1/gc-ci-libs.zip"
}
& 7z x -y gc-ci-libs.zip -oC:\libs

# GSL
& vcpkg install gsl:x64-windows

# Get config
Copy-Item "qwt\qwtconfig.pri.in" "qwt\qwtconfig.pri"
Copy-Item "c:\libs\gcconfig64Qt6-Release.appveyor.pri" "src\gcconfig.pri"

# Get jom
if (-not (Test-Path "jom_1_1_3.zip")) {
    appveyor DownloadFile "https://download.qt.io/official_releases/jom/jom_1_1_3.zip"
}
& 7z x -y jom_1_1_3.zip -oc:\jom\

# Get R and add to config
if (-not (Test-Path 'C:\R')) {
    $rurl = "https://cran.r-project.org/bin/windows/base/old/4.1.3/R-4.1.3-win.exe"
    Start-FileDownload $rurl "R-win.exe"
    Start-Process -FilePath .\R-win.exe -ArgumentList "/VERYSILENT /DIR=C:\R" -NoNewWindow -Wait
}
& "C:\R\bin\R" --version
Add-Content src\gcconfig.pri "DEFINES+=GC_WANT_R"

# Get Python embeddable and install packages
if (-not (Test-Path 'C:\Python')) {
    Start-FileDownload "https://www.python.org/ftp/python/3.7.9/python-3.7.9-embed-amd64.zip" "Python.zip"
    & 7z x Python.zip -oC:\Python\
    "python37.zip . '' 'import site'" | Out-File C:\Python\python37._pth -Encoding ascii
    New-Item -ItemType Directory -Force -Path C:\Python\lib\site-packages
    & c:\python37-x64\python -m pip install --upgrade pip
    & c:\python37-x64\python -m pip install -r src\Python\requirements.txt -t C:\Python\lib\site-packages
}

# Get SIP and install on Python (using system python for build)
if (-not (Test-Path 'sip-4.19.8')) {
    Start-FileDownload "https://sourceforge.net/projects/pyqt/files/sip/sip-4.19.8/sip-4.19.8.zip"
    & 7z x sip-4.19.8.zip
    Push-Location sip-4.19.8
    & c:\python37-x64\python --version
    & c:\python37-x64\python configure.py
    & c:\jom\jom.exe -j2
    Pop-Location
}
# Install SIP
Push-Location sip-4.19.8
cmd /c "nmake install"
Pop-Location

# Add Python (avoiding collision between GC Context.h and Python context.h)
Add-Content src\gcconfig.pri "DEFINES+=GC_WANT_PYTHON"
Add-Content src\gcconfig.pri "PYTHONINCLUDES=-ICore -I`"c:\python37-x64\include`""
Add-Content src\gcconfig.pri "PYTHONLIBS=-L`"c:\python37-x64\libs`" -lpython37"

# GSL Config
Add-Content src\gcconfig.pri "GSL_INCLUDES=c:\tools\vcpkg\installed\x64-windows\include"
Add-Content src\gcconfig.pri "GSL_LIBS=-Lc:\tools\vcpkg\installed\x64-windows\lib -lgsl -lgslcblas"
