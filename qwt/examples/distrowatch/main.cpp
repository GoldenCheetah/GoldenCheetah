/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#include "BarChart.h"

#include <QApplication>
#include <QMainWindow>
#include <QToolBar>
#include <QToolButton>
#include <QComboBox>

namespace
{
    class MainWindow : public QMainWindow
    {
      public:
        MainWindow( QWidget* = nullptr );

    };
}

MainWindow::MainWindow( QWidget* parent )
    : QMainWindow( parent )
{
    BarChart* chart = new BarChart( this );
    setCentralWidget( chart );

    QToolBar* toolBar = new QToolBar( this );

    QComboBox* orientationBox = new QComboBox( toolBar );
    orientationBox->addItem( "Vertical" );
    orientationBox->addItem( "Horizontal" );
    orientationBox->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

    QToolButton* btnExport = new QToolButton( toolBar );
    btnExport->setText( "Export" );
    btnExport->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
    connect( btnExport, &QToolButton::clicked,
        chart, &BarChart::exportChart );

    QToolButton* btnScreenshot = new QToolButton( toolBar );
    btnScreenshot->setText( "Screenshot" );
    btnScreenshot->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
    connect( btnScreenshot, &QToolButton::clicked,
        chart, &BarChart::doScreenShot );

    toolBar->addWidget( orientationBox );
    toolBar->addWidget( btnExport );
    toolBar->addWidget( btnScreenshot );
    addToolBar( toolBar );

    void (QComboBox::*comboValueChanged)(int) = &QComboBox::currentIndexChanged;

    chart->setOrientation( orientationBox->currentIndex() );
    connect( orientationBox, comboValueChanged,
        chart, &BarChart::setOrientation );
}

int main( int argc, char* argv[] )
{
    QApplication app( argc, argv );

    MainWindow window;

    window.resize( 600, 400 );
    window.show();

    return app.exec();
}
