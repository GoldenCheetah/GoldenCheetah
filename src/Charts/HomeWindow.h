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
#include "ChartSettings.h"
#include "GcWindowRegistry.h"
#include "GcWindowLayout.h"

#include <QtGui>
#include <QScrollArea>
#include <QFormLayout>
#include <QXmlDefaultHandler>
#include <QMessageBox>
#include <QLabel>
#include <QDialog>
#include <QTreeWidget>
#include <QTableWidget>
#include <QCheckBox>
#include <QStackedWidget>

#include "Context.h"
#include "RideItem.h"

class ChartBar;
class LTMSettings;

class HomeWindow : public GcWindow
{
    Q_OBJECT
    G_OBJECT

    public:

        HomeWindow(Context *, QString name, QString title);
        ~HomeWindow();

        void resetLayout();
        void importChart(QMap<QString,QString> properties, bool select);

        void setStyle(int style) { styleChanged(style); }
        int currentStyle;

        int currentTab() { return currentStyle ? -1 : controlStack->currentIndex(); }
        GcChartWindow *currentChart() {
            return currentTab() >= 0 ? charts[currentTab()] : NULL;
        }

    public slots:

        // GC signals
        void rideSelected();
        void dateRangeChanged(DateRange);
        void configChanged(qint32);
        void presetSelected(int n);

        // QT Widget events and signals
        void tabSelected(int id);
        void tabSelected(int id, bool forride);
        void tabMoved(int from, int to);
        void tabMenu(int index, int x);
        virtual void dragEnterEvent(QDragEnterEvent *);
        virtual void dropEvent(QDropEvent *);
        void resizeEvent(QResizeEvent *);
        void showEvent(QShowEvent *);
        bool eventFilter(QObject *object, QEvent *e);

        // My widget signals and events
        void styleChanged(int);
        void addChart(GcChartWindow* newone);
        void addChartFromMenu(QAction*action); // called with an action
        void appendChart(GcWinID id); // called from Context *to inset chart
        bool removeChart(int, bool confirm = true);
        void titleChanged();

        // window wants to close...
        void closeWindow(GcWindow*);
        void showControls();

        // save / restore window state
        void saveState();
        void restoreState(bool useDefault = false);

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
        void rightClick(const QPoint &pos);

        // Realtime steering control of train window scrolling
        void steerScroll(int scrollAmount);

    protected:
        Context *context;
        QString name;
        bool active; // ignore gui signals when changing views
        GcChartWindow *clicked; // keep track of selected charts
        bool dropPending;

        // top bar
        QLabel *title;
        QLineEdit *titleEdit;

        QComboBox *styleSelector;
        QStackedWidget *style; // tab, freeform, tiled
        QStackedWidget *controlStack; // window controls

        // each style has its own container widget
        ChartBar *chartbar;
        QStackedWidget *tabbed; 

        QScrollArea *tileArea;
        QWidget *tileWidget;
        QGridLayout *tileGrid;

        QScrollArea *winArea;
        QWidget *winWidget;
        GcWindowLayout *winFlow;

        // the charts!
        QList<GcChartWindow*> charts;
        int chartCursor;

        bool loaded;

        void translateChartTitles(QList<GcChartWindow*> charts);
};

// setup the chart
class GcWindowDialog : public QDialog
{
    Q_OBJECT

    public:
        GcWindowDialog(GcWinID, Context *, GcChartWindow **, bool sidebar=false, LTMSettings *use=NULL);
        int exec();               // return pointer to window, or NULL if cancelled

    public slots:
        void okClicked();
        void cancelClicked();

    protected:
        Context *context;
        GcWinID type;
        GcChartWindow **here;
        bool sidebar;

        // we remove from the layout at the end
        QHBoxLayout *layout;
        QVBoxLayout *mainLayout;
        QVBoxLayout *chartLayout;
        QFormLayout *controlLayout;

        QPushButton *ok, *cancel;
        GcChartWindow *win;
        QLineEdit *title;
        QDoubleSpinBox *height, *width;
};

class ViewParser : public QXmlDefaultHandler
{

public:
    ViewParser(Context *context) : style(2), context(context) {}

    // the results!
    QList<GcChartWindow*> charts;
    int style;

    // unmarshall
    bool startDocument();
    bool endDocument();
    bool endElement( const QString&, const QString&, const QString &qName );
    bool startElement( const QString&, const QString&, const QString &name, const QXmlAttributes &attrs );
    bool characters( const QString& str );

protected:
    Context *context;
    GcChartWindow *chart;

};

class ImportChartDialog : public QDialog
{
    Q_OBJECT

    public:
        ImportChartDialog(Context *context, QList<QMap<QString,QString> >list, QWidget *parent);

    protected:
        QTableWidget *table;
        QPushButton *import, *cancel;

    public slots:
        void importClicked();
        void cancelClicked();

    private:
        Context *context;
        QList<QMap<QString,QString> >list;

};

#endif // _GC_HomeWindow_h
