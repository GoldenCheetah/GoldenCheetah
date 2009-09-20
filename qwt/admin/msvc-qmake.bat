REM Batch file to make all Makefiles or all Visual Studio project files
REM (*.dsp for MSVC-6.0 or *.vcproj for MSVC-7.0) for Qwt with qmake.
REM
REM BUG: the designer plugin *.dsp file may not work; the Makefile does.
REM
REM To make Makefiles, type: msvc-qmake
REM To make project files type: msvc-qmake vc

REM For the Qwt library:
cd src
qmake -t %1lib% src.pro
cd ..

REM For the designer plugin:
cd textengines\mathml
qmake -t %1lib mathml.pro
cd ..\..

REM For the designer plugin:
cd designer
qmake -t %1lib designer.pro
cd ..

REM For the examples:
cd examples
cd bode
qmake -t %1app bode.pro
cd ..\cpuplot
qmake -t %1app cpuplot.pro
cd ..\curvdemo1
qmake -t %1app curvdemo1.pro
cd ..\curvdemo2
qmake -t %1app curvdemo2.pro
cd ..\data_plot
qmake -t %1app data_plot.pro
cd ..\dials
qmake -t %1app dials.pro
cd ..\event_filter
qmake -t %1app event_filter.pro
cd ..\histogram
qmake -t %1app histogram.pro
cd ..\radio
qmake -t %1app radio.pro
cd ..\realtime_plot
qmake -t %1app realtime_plot.pro
cd ..\simple_plot
qmake -t %1app simple_plot.pro
cd ..\sliders
qmake -t %1app sliders.pro
cd ..\spectrogram
qmake -t %1app spectrogram.pro
cd ..\svgmap
qmake -t %1app svgmap.pro
cd ..\sysinfo
qmake -t %1app sysinfo.pro
cd ..\..

REM EOF
