/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#include "MainWindow.h"
#include "SamplingThread.h"

#include <QApplication>

int main( int argc, char* argv[] )
{
    QApplication app( argc, argv );
    app.setPalette( Qt::darkGray );

    MainWindow window;
    window.resize( 800, 400 );

    SamplingThread samplingThread;
    samplingThread.setFrequency( window.frequency() );
    samplingThread.setAmplitude( window.amplitude() );
    samplingThread.setInterval( window.signalInterval() );

    window.connect( &window, &MainWindow::frequencyChanged,
        &samplingThread, &SamplingThread::setFrequency );

    window.connect( &window, &MainWindow::amplitudeChanged,
        &samplingThread, &SamplingThread::setAmplitude );

    window.connect( &window, &MainWindow::signalIntervalChanged,
        &samplingThread, &SamplingThread::setInterval );

    window.show();

    samplingThread.start();
    window.start();

    const bool ok = app.exec();

    samplingThread.stop();
    samplingThread.wait( 1000 );

    return ok;
}
