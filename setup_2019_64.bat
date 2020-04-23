CALL "c:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
set PATH=%PATH%;C:\Qt\Qt5.12.1\Tools\QtCreator\bin;C:\Qt\Qt5.12.1\5.12.1\msvc2017_64\bin\;c:\Users\esc\Documents\Bison\;C:\coding\vlc-2.2.5.1;C:\coding\gc-win-sdk-64bit\ReleaseLibs64-Qt5.8.0
C:\Qt\Qt5.12.1\5.12.1\msvc2017_64\bin\qmake.exe -o Makefile build.pro
qmake build.pro