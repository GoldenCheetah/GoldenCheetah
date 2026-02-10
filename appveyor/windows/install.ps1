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
