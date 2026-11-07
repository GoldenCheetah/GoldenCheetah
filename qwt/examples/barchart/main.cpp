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
    BarChart* chart = new BarChart();
    setCentralWidget( chart );

    QToolBar* toolBar = new QToolBar();

    QComboBox* typeBox = new QComboBox( toolBar );
    typeBox->addItem( "Grouped" );
    typeBox->addItem( "Stacked" );
    typeBox->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

    QComboBox* orientationBox = new QComboBox( toolBar );
    orientationBox->addItem( "Vertical" );
    orientationBox->addItem( "Horizontal" );
    orientationBox->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

    QToolButton* btnExport = new QToolButton( toolBar );
    btnExport->setText( "Export" );
    btnExport->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
    connect( btnExport, &QToolButton::clicked, chart, &BarChart::exportChart );

    toolBar->addWidget( typeBox );
    toolBar->addWidget( orientationBox );
    toolBar->addWidget( btnExport );
    addToolBar( toolBar );

    void (QComboBox::*comboValueChanged)(int) = &QComboBox::currentIndexChanged;

    chart->setMode( typeBox->currentIndex() );
    chart->setOrientation( orientationBox->currentIndex() );

    connect( typeBox, comboValueChanged, chart, &BarChart::setMode );
    connect( orientationBox, comboValueChanged, chart, &BarChart::setOrientation );
}

int main( int argc, char* argv[] )
{
    QApplication app( argc, argv );

    MainWindow window;

    window.resize( 600, 400 );
    window.show();

    return app.exec();
}
