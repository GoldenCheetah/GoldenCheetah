/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _GC_HomeWindow_h
#define _GC_HomeWindow_h 1
#include "GoldenCheetah.h"
#include "GcWindowRegistry.h"
#include "GcWindowLayout.h"

#include <QtGui>
#include <QXmlDefaultHandler>
#include "MainWindow.h"

#ifdef Q_OS_MAC
#include "QtMacSegmentedButton.h"
#endif

class HomeWindow : public GcWindow
{
    Q_OBJECT
    G_OBJECT

    //Q_PROPERTY(int view READ view WRITE setView)

    public:

        HomeWindow(MainWindow *, QString name, QString title);
        ~HomeWindow();

        //int view() const { return viewMode->currentIndex(); }
        //void setView(int x) { viewMode->setCurrentIndex(x); }

    public slots:

        // GC signals
        void rideSelected();
        void configChanged();

        // QT Widget events and signals
        void tabSelected(int id);
        virtual void dragEnterEvent(QDragEnterEvent *);
        virtual void dropEvent(QDropEvent *);
        void resizeEvent(QResizeEvent *);
        bool eventFilter(QObject *object, QEvent *e);

        // My widget signals and events
        void styleChanged(int);
        void addChart(GcWindow* newone);
        bool removeChart(int);

        // save / restore window state
        void saveState();
        void restoreState();

        //notifiction that been made visible
        void selected();

        // moved or resized
        void windowMoving(GcWindow*);
        void windowResizing(GcWindow*);
        void windowMoved(GcWindow*);
        void windowResized(GcWindow*);

        // when moving tiles
        int pointTile(QPoint pos);
        void drawCursor();

    protected:
        MainWindow *mainWindow;
        QString name;
        bool active; // ignore gui signals when changing views
        GcWindow *clicked; // keep track of selected charts
        bool dropPending;

        // top bar
        QLabel *title;
#ifdef Q_OS_MAC
        QtMacSegmentedButton *styleSelector;
#else
        QComboBox *styleSelector;
#endif
        int currentStyle;

        QStackedWidget *style; // tab, freeform, tiled

        // each style has its own container widget
        QTabWidget *tabbed; 

        QScrollArea *tileArea;
        QWidget *tileWidget;
        QGridLayout *tileGrid;

        QScrollArea *winArea;
        QWidget *winWidget;
        GcWindowLayout *winFlow;

        // the charts!
        QList<GcWindow*> charts;
        int chartCursor;

        bool loaded;

};

// setup the chart
class GcWindowDialog : public QDialog
{
    Q_OBJECT

    public:
        GcWindowDialog(GcWinID, MainWindow*);
        GcWindow *exec();               // return pointer to window, or NULL if cancelled

    public slots:
        void okClicked();
        void cancelClicked();

    protected:
        MainWindow *mainWindow;
        GcWinID type;

        // we remove from the layout at the end
        QHBoxLayout *layout;
        QVBoxLayout *mainLayout;
        QVBoxLayout *chartLayout;
        QFormLayout *controlLayout;

        QPushButton *ok, *cancel;
        GcWindow *win;
        QLineEdit *title;
        QDoubleSpinBox *height, *width;
};

class ViewParser : public QXmlDefaultHandler
{

public:
    ViewParser(MainWindow *mainWindow) : mainWindow(mainWindow) {}

    // the results!
    QList<GcWindow*> charts;

    // unmarshall
    bool startDocument();
    bool endDocument();
    bool endElement( const QString&, const QString&, const QString &qName );
    bool startElement( const QString&, const QString&, const QString &name, const QXmlAttributes &attrs );
    bool characters( const QString& str );

protected:
    MainWindow *mainWindow;
    GcWindow *chart;

};
#endif // _GC_HomeWindow_h
