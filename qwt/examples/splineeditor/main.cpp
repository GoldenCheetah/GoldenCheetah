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
#include <QTabWidget>

namespace
{
    class ToolButton : public QToolButton
    {
      public:
        ToolButton( const char* text )
        {
            setText( text );
            setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
        }
    };

    class PlotTab : public QMainWindow
    {
      public:
        PlotTab( bool parametric,  QWidget* parent )
            : QMainWindow( parent )
        {
            void ( QComboBox::*comboActivated )( const QString& ) = 
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                &QComboBox::textActivated;
#else
                &QComboBox::activated;
#endif

            Plot* plot = new Plot( parametric, this );
            setCentralWidget( plot );

            QToolBar* toolBar = new QToolBar();

#ifndef QT_NO_PRINTER
            ToolButton* btnPrint = new ToolButton( "Print" );
            toolBar->addWidget( btnPrint );
            QObject::connect( btnPrint, &QToolButton::clicked,
                plot, &Plot::printPlot );
#endif

            ToolButton* btnOverlay = new ToolButton( "Overlay" );
            btnOverlay->setCheckable( true );
            toolBar->addWidget( btnOverlay );
            QObject::connect( btnOverlay, &QToolButton::toggled,
                plot, &Plot::setOverlaying );

            if ( parametric )
            {
                QComboBox* parameterBox = new QComboBox();

                parameterBox->addItem( "Uniform" );
                parameterBox->addItem( "Centripetral" );
                parameterBox->addItem( "Chordal" );
                parameterBox->addItem( "Manhattan" );
                toolBar->addWidget( parameterBox );

                connect( parameterBox, comboActivated,
                    plot, &Plot::setParametric );

                parameterBox->setCurrentIndex( 2 ); // chordal
                plot->setParametric( parameterBox->currentText() );

                ToolButton* btnClosed = new ToolButton( "Closed" );
                btnClosed->setCheckable( true );
                toolBar->addWidget( btnClosed );
                QObject::connect( btnClosed, &QToolButton::toggled,
                    plot, &Plot::setClosed );
            }

            QComboBox* boundaryBox = new QComboBox();

            boundaryBox->addItem( "Natural" );
            boundaryBox->addItem( "Linear Runout" );
            boundaryBox->addItem( "Parabolic Runout" );
            boundaryBox->addItem( "Cubic Runout" );
            boundaryBox->addItem( "Not a Knot" );

            toolBar->addWidget( boundaryBox );
            connect( boundaryBox, comboActivated,
                plot, &Plot::setBoundaryCondition );

            addToolBar( toolBar );
        }
    };
}

int main( int argc, char* argv[] )
{
    QApplication app( argc, argv );
    app.setStyle( "Windows" );

    QTabWidget tabWidget;
    tabWidget.addTab( new PlotTab( false, &tabWidget ), "Non Parametric" );
    tabWidget.addTab( new PlotTab( true, &tabWidget ), "Parametric" );

    tabWidget.resize( 800, 600 );
    tabWidget.show();

    return app.exec();
}
