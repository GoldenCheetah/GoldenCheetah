/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#include "Plot.h"

#include <QApplication>
#include <QMainWindow>
#include <QToolBar>
#include <QToolButton>
#include <QComboBox>
#include <QLabel>

namespace
{
    class MainWindow : public QMainWindow
    {
      public:
        MainWindow( QWidget* parent = nullptr )
            : QMainWindow( parent )
        {
            Plot* plot = new Plot( this );
            setCentralWidget( plot );

            QToolBar* toolBar = new QToolBar( this );

            QComboBox* rasterBox = new QComboBox( toolBar );
            rasterBox->addItem( "Wikipedia" );

            toolBar->addWidget( new QLabel( "Data ", toolBar ) );
            toolBar->addWidget( rasterBox );
            toolBar->addSeparator();

            QComboBox* modeBox = new QComboBox( toolBar );
            modeBox->addItem( "Nearest Neighbour" );
            modeBox->addItem( "Bilinear Interpolation" );
            modeBox->addItem( "Bicubic Interpolation" );

            toolBar->addWidget( new QLabel( "Resampling ", toolBar ) );
            toolBar->addWidget( modeBox );

            toolBar->addSeparator();

            QToolButton* btnExport = new QToolButton( toolBar );
            btnExport->setText( "Export" );
            btnExport->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
            toolBar->addWidget( btnExport );

            addToolBar( toolBar );

            void (QComboBox::*comboValueChanged)(int) = &QComboBox::currentIndexChanged;

            connect( modeBox, comboValueChanged, plot, &Plot::setResampleMode );
            connect( btnExport, &QToolButton::clicked, plot, &Plot::exportPlot );
        }
    };
}

int main( int argc, char* argv[] )
{
    QApplication app( argc, argv );

    MainWindow window;
    window.resize( 600, 400 );
    window.show();

    return app.exec();
}
