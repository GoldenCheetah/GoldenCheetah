### Generate installer from release folder updating Qt dependencies
qmake --version
windeployqt --force .\GoldenCheetah.exe
copy ..\Resources\win32\GC3.8-Master-W64-QT6.nsi .\GC3.8-Master-W64-QT6.nsi
C:\Program` Files` `(x86`)\NSIS\makensis.exe .\GC3.8-Master-W64-QT6.nsi
move -force .\GoldenCheetah_v3.8_64bit_Windows.exe .\GoldenCheetah_v3.8_x64Qt6.exe

### Generate version file with SHA
.\GoldenCheetah --version 2>.\GCversionWindowsQt6.txt
git log -1 >>.\GCversionWindowsQt6.txt
CertUtil -hashfile .\GoldenCheetah_v3.8_x64Qt6.exe sha256 | Select-Object -First 2 | Add-Content .\GCversionWindowsQt6.txt
type .\GCversionWindowsQt6.txt
