# Python version configuration - update this when upgrading Python
$PYTHON_EMBED_VERSION="3.11.9"

# Get the libraries
if (-not (Test-Path 'C:\LIBS')) {
  Start-FileDownload "https://github.com/GoldenCheetah/WindowsSDK/releases/download/v0.1.1/gc-ci-libs.zip"
  7z x -y gc-ci-libs.zip -oC:\LIBS
}

# Get jom
if (-not (Test-Path 'C:\JOM')) {
  Start-FileDownload "https://download.qt.io/official_releases/jom/jom_1_1_3.zip"
  7z x -y jom_1_1_3.zip -oC:\JOM\
}

# GSL
vcpkg install gsl:x64-windows

# Get R
if (-not (Test-Path 'C:\R')) {
  # Lets use 4.1 until 4.2 issues are fixed
  #$rurl = $(ConvertFrom-JSON $(Invoke-WebRequest https://rversions.r-pkg.org/r-release-win).Content).URL
  $rurl = "https://cran.r-project.org/bin/windows/base/old/4.1.3/R-4.1.3-win.exe"
  Start-FileDownload $rurl "R-win.exe"
  Start-Process -FilePath .\R-win.exe -ArgumentList "/VERYSILENT /DIR=C:\R" -NoNewWindow -Wait
}
C:\R\bin\R --version

# Get Python
if (-not (Test-Path 'C:\Python')) {
  $pyurl = "https://www.python.org/ftp/python/$PYTHON_EMBED_VERSION/python-$PYTHON_EMBED_VERSION-embed-amd64.zip"
  Start-FileDownload $pyurl "python-embed.zip"
  Expand-Archive -Path python-embed.zip -DestinationPath C:\Python -Force
  mkdir C:\Python\lib\site-packages
  # Enable pip in embedded Python
  $py_ver = $env:PYTHON_VERSION -replace '.', ''
  (Get-Content C:\Python\python$py_ver._pth) -replace '#import site', 'import site' | Set-Content C:\Python\python$py_ver._pth
  Start-FileDownload "https://bootstrap.pypa.io/get-pip.py" "get-pip.py"
  C:\Python\python.exe get-pip.py --no-warn-script-location
  # Upgrade pip to ensure you have the latest version
  python -m pip install --upgrade pip
  # Install your project's dependencies from a requirements.txt file
  python -m pip install --only-binary:all: -r src\Python\requirements.txt -t C:\Python\lib\site-packages
}
