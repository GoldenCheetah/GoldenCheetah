/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#include "TVPlot.h"

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
        MainWindow()
        {
            TVPlot* plot = new TVPlot();
            setCentralWidget( plot );

            QComboBox* typeBox = new QComboBox();
            typeBox->addItem( "Outline" );
            typeBox->addItem( "Columns" );
            typeBox->addItem( "Lines" );
            typeBox->addItem( "Column Symbol" );
            typeBox->setCurrentIndex( typeBox->count() - 1 );
            typeBox->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

            QToolButton* btnExport = new QToolButton();
            btnExport->setText( "Export" );
            btnExport->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
            connect( btnExport, &QToolButton::clicked, plot, &TVPlot::exportPlot );

            QToolBar* toolBar = new QToolBar( this );
            toolBar->addWidget( typeBox );
            toolBar->addWidget( btnExport );
            addToolBar( toolBar );

            void (QComboBox::*comboValueChanged)(int) = &QComboBox::currentIndexChanged;

            plot->setMode( typeBox->currentIndex() );
            connect( typeBox, comboValueChanged, plot, &TVPlot::setMode );
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
