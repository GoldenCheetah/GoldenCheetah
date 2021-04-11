CALL "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
qmake build.pro -spec win32-msvc
jom -j 7 qmake_all && jom -j 7
