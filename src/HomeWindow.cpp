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

#include "Athlete.h"
#include "Context.h"
#include "Tab.h"
#include "TabView.h"
#include "HomeWindow.h"
#include "LTMTool.h"
#include "LTMSettings.h"
#include "LTMWindow.h"
#include "Settings.h"
#include "ChartBar.h"

#include <QGraphicsDropShadowEffect>
#include <QStyle>
#include <QStyleFactory>
#include <QScrollBar>

static const int tileMargin = 20;
static const int tileSpacing = 10;

HomeWindow::HomeWindow(Context *context, QString name, QString /* windowtitle */) :
    GcWindow(context), context(context), name(name), active(false),
    clicked(NULL), dropPending(false), chartCursor(-2), loaded(false)
{
    // setup control area
    QWidget *cw = new QWidget(this);
    cw->setContentsMargins(0,0,0,0);

    QVBoxLayout *cl = new QVBoxLayout(cw);
    cl->setSpacing(0);
    cl->setContentsMargins(0,0,0,0);

    QLabel *titleLabel = new QLabel(tr("Title"), this);
    QHBoxLayout *hl = new QHBoxLayout;
    hl->setSpacing(5);
    hl->setContentsMargins(0,0,0,0);

    titleEdit = new QLineEdit(this);
    titleEdit->setEnabled(false);
    controlStack = new QStackedWidget(this);
    controlStack->setContentsMargins(0,0,0,0);
    hl->addWidget(titleLabel);
    hl->addWidget(titleEdit);
    cl->addLayout(hl);
    cl->addSpacing(15);
    cl->addWidget(controlStack);
    setControls(cw);

    setProperty("isManager", true);
    setAcceptDrops(true);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QFont bigandbold;
    bigandbold.setPointSize(bigandbold.pointSize() + 2);
    bigandbold.setWeight(QFont::Bold);

    style = new QStackedWidget(this);
    style->setAutoFillBackground(false);
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(style);

    QPalette palette;
    palette.setBrush(backgroundRole(), QColor("#B3B4B6"));
    setAutoFillBackground(false);

    // each style has its own container widget
    QWidget *tabArea = new QWidget(this);
    tabArea->setContentsMargins(0,0,0,0); // no spacing now, used to be 20px
    QVBoxLayout *tabLayout = new QVBoxLayout(tabArea);
    tabLayout->setContentsMargins(0,0,0,0);
    tabLayout->setSpacing(0);
    tabbed = new QStackedWidget(this);

    chartbar = new ChartBar(context);
    tabLayout->addWidget(chartbar);
#ifdef WANT_RIDECACHE
    tabLayout->addWidget(new ProgressLine(this, context));
#endif
    tabLayout->addWidget(tabbed);
    style->addWidget(tabArea);

    // tiled
    tileWidget = new QWidget(this);
    tileWidget->setAutoFillBackground(false);
    tileWidget->setPalette(palette);
    tileWidget->setContentsMargins(0,0,0,0);

    tileGrid = new QGridLayout(tileWidget);
    tileGrid->setSpacing(0);

    tileArea = new QScrollArea(this);
#ifdef Q_OS_WIN
    QStyle *cde = QStyleFactory::create(OS_STYLE);
    tileArea->setStyle(cde);
#endif
    tileArea->setAutoFillBackground(false);
    tileArea->setPalette(palette);
    tileArea->setWidgetResizable(true);
    tileArea->setWidget(tileWidget);
    tileArea->setFrameStyle(QFrame::NoFrame);
    tileArea->setContentsMargins(0,0,0,0);
    style->addWidget(tileArea);

    winWidget = new QWidget(this);
    winWidget->setAutoFillBackground(false);
    winWidget->setContentsMargins(0,0,0,0);
    winWidget->setPalette(palette);

    winFlow = new GcWindowLayout(winWidget, 0, tileSpacing, tileSpacing);
    winFlow->setContentsMargins(tileMargin,tileMargin,tileMargin,tileMargin);

    winArea = new QScrollArea(this);
    winArea->setAutoFillBackground(false);
    winArea->setPalette(palette);
    winArea->setWidgetResizable(true);
    winArea->setWidget(winWidget);
    winArea->setFrameStyle(QFrame::NoFrame);
    winArea->setContentsMargins(0,0,0,0);
    style->addWidget(winArea);
    winWidget->installEventFilter(this); // to draw cursor
    winWidget->setMouseTracking(true); // to draw cursor

    // enable right click to add a chart
    winArea->setContextMenuPolicy(Qt::CustomContextMenu);
    tabArea->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(winArea,SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(rightClick(const QPoint &)));
    connect(tabArea,SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(rightClick(const QPoint &)));

    currentStyle=-1;
    styleChanged(2);

    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(this, SIGNAL(dateRangeChanged(DateRange)), this, SLOT(dateRangeChanged(DateRange)));
    connect(context, SIGNAL(configChanged()), this, SLOT(configChanged()));
    //connect(tabbed, SIGNAL(currentChanged(int)), this, SLOT(tabSelected(int)));
    //connect(tabbed, SIGNAL(tabCloseRequested(int)), this, SLOT(removeChart(int)));
    //connect(tb, SIGNAL(tabMoved(int,int)), this, SLOT(tabMoved(int,int)));
    connect(chartbar, SIGNAL(currentIndexChanged(int)), this, SLOT(tabSelected(int)));
    connect(titleEdit, SIGNAL(textChanged(const QString&)), SLOT(titleChanged()));

    installEventFilter(this);
    qApp->installEventFilter(this);
}

HomeWindow::~HomeWindow()
{
    qApp->removeEventFilter(this);
}

void
HomeWindow::rightClick(const QPoint & /*pos*/)
{
    return; //deprecated right click on homewindow -- bad UX
}

void
HomeWindow::addChartFromMenu(QAction*action)
{
    GcWinID id = GcWindowTypes::None;
    for (int i=0; GcWindows[i].relevance; i++) {
        if (GcWindows[i].name == action->text()) {
            id = GcWindows[i].id;
            break;
        }
    }

    if (id != GcWindowTypes::None) {
        appendChart(id); // called from Context to inset chart
    }
}

void
HomeWindow::configChanged()
{
    // update scroll bar
//#ifndef Q_OS_MAC
    tileArea->verticalScrollBar()->setStyleSheet(TabView::ourStyleSheet());
//#endif
}

void
HomeWindow::selected()
{
    setUpdatesEnabled(false);

    if (loaded == false) {
        restoreState();
        loaded = true;
        if (!currentStyle && charts.count()) tabSelected(0);
    }

    resizeEvent(NULL); // force a relayout
    rideSelected();
    dateRangeChanged(DateRange());

    setUpdatesEnabled(true);
    update();
}

void
HomeWindow::titleChanged()
{
    if (charts.count() == 0) return;

    if (titleEdit->text() != charts[controlStack->currentIndex()]->property("title").toString()) {

        // set the title property
        charts[controlStack->currentIndex()]->setProperty("title", titleEdit->text());

        // rename the tab
        if (!currentStyle) {
            chartbar->setText(controlStack->currentIndex(), titleEdit->text());
        }

        // repaint to reflect
        charts[controlStack->currentIndex()]->repaint();
    }
}

void
HomeWindow::rideSelected()
{
    // we need to notify of null rides immediately
    if (!myRideItem || isVisible()) {
        for (int i=0; i < charts.count(); i++) {

            // show if its not a tab
            if (currentStyle) charts[i]->show(); // keep tabs hidden, show the rest

            // if we are tabbed AND its the current tab then mimic tabselected
            // to force the tabwidget to refresh AND set its ride property (see below)
            //otherwise just go ahead and notify it of a new ride
            if (!currentStyle && charts.count() && i==tabbed->currentIndex())
                tabSelected(tabbed->currentIndex(), true); // for ride change
            else
                charts[i]->setProperty("ride", property("ride"));
        }
    }
}

void
HomeWindow::dateRangeChanged(DateRange dr)
{ Q_UNUSED( dr )
    if (isVisible()) {

        for (int i=0; i < charts.count(); i++) {

            // show if its not a tab
            if (currentStyle) charts[i]->show(); // keep tabs hidden, show the rest

            // if we are tabbed AND its the current tab then mimic tabselected
            // to force the tabwidget to refresh AND set its ride property (see below)
            //otherwise just go ahead and notify it of a new ride
            if (!currentStyle && charts.count() && i==tabbed->currentIndex())
                tabSelected(tabbed->currentIndex(), false); // for date range
            else
                charts[i]->setProperty("dateRange", property("dateRange"));
        }
    }
}

void
HomeWindow::tabSelected(int index)
{
    // user switched tabs -- tell tabe to redraw!
    if (active || currentStyle != 0) return;

    active = true;

    if (index >= 0) {
        charts[index]->show();
        charts[index]->setProperty("ride", property("ride"));
        charts[index]->setProperty("dateRange", property("dateRange"));
        controlStack->setCurrentIndex(index);
        titleEdit->setText(charts[index]->property("title").toString());
        tabbed->setCurrentIndex(index);
        chartbar->setCurrentIndex(index);
    }

    active = false;
}

void
HomeWindow::tabSelected(int index, bool forride)
{
    if (active || currentStyle != 0) return;

    active = true;

    if (index >= 0) {
        charts[index]->show();
        if (forride) charts[index]->setProperty("ride", property("ride"));
        else charts[index]->setProperty("dateRange", property("dateRange"));
        controlStack->setCurrentIndex(index);
        titleEdit->setText(charts[index]->property("title").toString());
    }

    active = false;
}

void
HomeWindow::tabMoved(int to, int from)
{
    // re-order the tabs
    GcWindow *orig = charts[to];
    charts[to] = charts[from];
    charts[from] = orig;

    // re-order the controls
    controlStack->blockSignals(true);
    QWidget *w = controlStack->widget(from);
    controlStack->removeWidget(w);
    controlStack->insertWidget(to, w);
    controlStack->setCurrentIndex(to);
    controlStack->blockSignals(false);
}

void
HomeWindow::styleChanged(int id)
{
    // ignore if out of bounds or we're already using that style
    if (id > 2 || id < 0 || id == currentStyle) return;
    active = true;

    // block updates as it is butt ugly
    setUpdatesEnabled(false);
    tabbed->hide(); //This is a QTabWidget setUpdatesEnabled bug (?)
    chartbar->hide();
    chartbar->clear();

    // move the windows from there current
    // position to there new position
    for (int i=0; i<charts.count(); i++) {
        // remove from old style
        switch (currentStyle) {
        case 0 : // they are tabs in a TabWidget
            {
            int tabnum = tabbed->indexOf(charts[i]);
            tabbed->removeWidget(tabbed->widget(tabnum));
            }
            break;
        case 1 : // they are lists in a GridLayout
            {
            tileGrid->removeWidget(charts[i]);
            }
            break;
        case 2 : // they are in a FlowLayout
            {
            winFlow->removeWidget(charts[i]);
            }
        default:
            break;
        }

        // now put into new style
        switch (id) {
        case 0 : // they are tabs in a TabWidget
            tabbed->addWidget(charts[i]);
            chartbar->addWidget(charts[i]->property("title").toString());
            charts[i]->setContentsMargins(0,25,0,0);
            charts[i]->setResizable(false); // we need to show on tab selection!
            charts[i]->setProperty("dateRange", property("dateRange"));
            charts[i]->hide(); // we need to show on tab selection!
            break;
        case 1 : // they are lists in a GridLayout
            tileGrid->addWidget(charts[i], i,0);
            charts[i]->setContentsMargins(0,25,0,0);
            charts[i]->setResizable(false); // we need to show on tab selection!
            charts[i]->show();
            charts[i]->setProperty("dateRange", property("dateRange"));
            charts[i]->setProperty("ride", property("ride"));
            break;
        case 2 : // thet are in a FlowLayout
            winFlow->addWidget(charts[i]);
            charts[i]->setContentsMargins(0,15,0,0);
            charts[i]->setResizable(true); // we need to show on tab selection!
            charts[i]->show();
            charts[i]->setProperty("dateRange", property("dateRange"));
            charts[i]->setProperty("ride", property("ride"));
        default:
            break;
        }

    }

    currentStyle = id;
    style->setCurrentIndex(currentStyle);

    active = false;

    if (currentStyle == 0 && charts.count()) tabSelected(0);
    resizeEvent(NULL); // watch out in case resize event uses this!!

    // now refresh as we are done
    setUpdatesEnabled(true);
    tabbed->show(); // QTabWidget setUpdatesEnabled bug
    chartbar->show(); // and resize artefact too.. tread carefully.
    update();
}

void
HomeWindow::dragEnterEvent(QDragEnterEvent *)
{
#if 0 // draw and drop chart no longer part of the UX
    if (event->mimeData()->formats().contains("application/x-qabstractitemmodeldatalist")) {
        event->accept();
        dropPending = true;
    }
#endif
}

void
HomeWindow::appendChart(GcWinID id)
{
    GcWindow *newone = NULL;

    // GcWindowDialog is delete on close, so no need to delete
    GcWindowDialog *f = new GcWindowDialog(id, context, &newone);
    f->exec();

    // returns null if cancelled or closed
    if (newone) {
        addChart(newone);
        newone->show(); //Crash on QT5
    }

    // now wipe it
    f->hide();
    f->deleteLater();

    // before we return lets turn the cursor off
    chartCursor = -2;
    winWidget->repaint();
}

void
HomeWindow::dropEvent(QDropEvent *)
{
#if 0 // drah and drop chart no longer part of the UX
    QStandardItemModel model;
    model.dropMimeData(event->mimeData(), Qt::CopyAction, -1,-1, QModelIndex());
    QString chart = model.data(model.index(0,0), Qt::DisplayRole).toString();

    dropPending = false;

    // which one am i?
    for (int i = 0; GcWindows[i].id != GcWindowTypes::None; i++) {
        if (GcWindows[i].name == chart) {

            event->accept();
            appendChart(GcWindows[i].id);
            return;
        }
    }

    // nope not one of ours
    event->ignore();

    // turn off cursor
    chartCursor = -2;
    winWidget->repaint();

    return;
#endif
}

void
HomeWindow::showControls()
{
    context->tab->chartsettings()->adjustSize();
    context->tab->chartsettings()->show();
}

void
HomeWindow::addChart(GcWindow* newone)
{
    int chartnum = charts.count();

    active = true;

    if (newone) {

        // add the controls
        QWidget *x = dynamic_cast<GcWindow*>(newone)->controls();
        QWidget *c = (x != NULL) ? x : new QWidget(this);

        // link settings button to show controls
        connect(newone, SIGNAL(showControls()), this, SLOT(showControls()));
        connect(newone, SIGNAL(closeWindow(GcWindow*)), this, SLOT(closeWindow(GcWindow*)));

        if (currentStyle == 2 && chartCursor >= 0)
            controlStack->insertWidget(chartCursor, c);
        else
            controlStack->addWidget(c);

        // watch for enter events!
        newone->installEventFilter(this);

        RideItem *notconst = (RideItem*)context->currentRideItem();
        newone->setProperty("ride", QVariant::fromValue<RideItem*>(notconst));
        newone->setProperty("dateRange", property("dateRange"));

        // add to tabs
        switch (currentStyle) {

        case 0 :
            newone->setContentsMargins(0,25,0,0);
            newone->setResizable(false); // we need to show on tab selection!
            //tabbed->addTab(newone, newone->property("title").toString());
            tabbed->addWidget(newone);
            chartbar->addWidget(newone->property("title").toString());
            break;
        case 1 :
            {
            // add to tiles
            // set geometry
            newone->setFixedWidth((tileArea->width()-50));
            newone->setFixedHeight(newone->width() * 0.7);
            newone->setResizable(false); // we need to show on tab selection!
            int row = chartnum; // / 2;
            int column = 0; //chartnum % 2;
            newone->setContentsMargins(0,25,0,0);
            tileGrid->addWidget(newone, row, column);
            }
            break;
        case 2 :
            {
                double heightFactor = newone->property("heightFactor").toDouble();
                double widthFactor = newone->property("widthFactor").toDouble();

                // width of area minus content margins and spacing around each item
                // divided by the number of items

                double newwidth = (winArea->width() - 20 /* scrollbar */
                               - (tileMargin*2) /* left and right marings */
                               - ((widthFactor-1) * tileSpacing) /* internal spacing */
                               ) / widthFactor;

                double newheight = (winArea->height()
                               - (tileMargin*2) /* top and bottom marings */
                               - ((heightFactor-1) * tileSpacing) /* internal spacing */
                               ) / heightFactor;

                int minWidth = 10;
                int minHeight = 10;
                if (newwidth < minWidth) newwidth = minWidth;
                newone->setFixedWidth(newwidth);
                if (newheight < minHeight) newheight = minHeight;
                else newone->setFixedHeight(newheight);
                newone->setContentsMargins(0,15,0,0);
                newone->setResizable(true); // we need to show on tab selection!

                if (currentStyle == 2 && chartCursor >= 0) winFlow->insert(chartCursor, newone);
                else winFlow->addWidget(newone);
            }
            break;
        }
        // add to the list
        if (currentStyle == 2 && chartCursor >= 0) charts.insert(chartCursor, newone);
        else charts.append(newone);
        newone->hide();

        // watch for moves etc
        connect(newone, SIGNAL(resizing(GcWindow*)), this, SLOT(windowResizing(GcWindow*)));
        connect(newone, SIGNAL(moving(GcWindow*)), this, SLOT(windowMoving(GcWindow*)));
        connect(newone, SIGNAL(resized(GcWindow*)), this, SLOT(windowResized(GcWindow*)));
        connect(newone, SIGNAL(moved(GcWindow*)), this, SLOT(windowMoved(GcWindow*)));
    }

    // enable disable
    if (charts.count()) titleEdit->setEnabled(true);
    else {
        titleEdit->setEnabled(false);
        titleEdit->setText("");
    }

    // select the last tab just added
    active = false;
}

bool
HomeWindow::removeChart(int num, bool confirm)
{
    if (num >= charts.count()) return false; // out of bounds (!)

    // better let the user confirm since this
    // is undoable etc - code swiped from delete
    // ride in Context, seems to work ok ;)
    if(confirm == true)
    {
        QMessageBox msgBox;
        msgBox.setText(tr("Are you sure you want to remove the chart?"));
        QPushButton *deleteButton = msgBox.addButton(tr("Remove"),QMessageBox::YesRole);
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        if(msgBox.clickedButton() != deleteButton) return false;
    }
    charts[num]->hide();

    // just in case its currently selected
    if (clicked == charts[num]) clicked=NULL;

    // remove the controls
    QWidget *m = controlStack->widget(num);
    if (m) controlStack->removeWidget(m);

    switch(currentStyle) {
        case 0 : // delete tab and widget
            chartbar->removeWidget(num);
            tabbed->removeWidget(tabbed->widget(num));
            break;
        case 1 : // scrolled
            tileGrid->removeWidget(charts[num]);
            break;
        case 2 : // tiled
            winFlow->removeWidget(charts[num]);
            break;
        default:
            break; // never reached
    }
    ((GcWindow*)(charts[num]))->close(); // disconnect
    ((GcWindow*)(charts[num]))->deleteLater();
    charts.removeAt(num);

    update();

    if (charts.count()) titleEdit->setEnabled(true);
    else {
        titleEdit->setEnabled(false);
        titleEdit->setText("");
    }

    return true;
}

void
HomeWindow::resetLayout()
{
    setUpdatesEnabled(false);
    int numCharts = charts.count();
    for(int i = numCharts - 1; i >= 0; i--) {
        removeChart(i,false);
    }
    restoreState(true);
    for(int i = 0; i < charts.count(); i++) {
        RideItem *notconst = (RideItem*)context->currentRideItem();
        charts[i]->setProperty("ride", QVariant::fromValue<RideItem*>(notconst));
        charts[i]->setProperty("dateRange", property("dateRange"));
        if (currentStyle != 0) charts[i]->show();
        
    }
    setUpdatesEnabled(true);
    if (currentStyle == 0 && charts.count()) tabSelected(0);
}

void
HomeWindow::showEvent(QShowEvent *)
{
    resizeEvent(NULL);
}

void
HomeWindow::resizeEvent(QResizeEvent * /* e */)
{
    foreach (GcWindow *x, charts) {

        switch (currentStyle) {

        case 0 : // tabbed
            x->setMinimumSize(0,0);
            x->setMaximumSize(9999,9999);
            break;

        case 1 : // tiled
            x->setFixedWidth((tileArea->width()-50));
            x->setFixedHeight(x->width() * 0.7);
            break;

        case 2 : // flow
            {
            double heightFactor = x->property("heightFactor").toDouble();
            double widthFactor = x->property("widthFactor").toDouble();


            double newwidth = (winArea->width() - 20 /* scrollbar */
                           - (tileMargin*2) /* left and right marings */
                           - ((widthFactor-1) * tileSpacing) /* internal spacing */
                           ) / widthFactor;

            double newheight = (winArea->height()
                           - (tileMargin*2) /* top and bottom marings */
                           - ((heightFactor-1) * tileSpacing) /* internal spacing */
                           ) / heightFactor;

            int minWidth = 10;
            int minHeight = 10;
            if (newwidth < minWidth) newwidth = minWidth;
            x->setFixedWidth(newwidth);
            if (newheight < minHeight) newheight = minHeight;
            else x->setFixedHeight(newheight);
            }
            break;

        default:
            break;
        }
    }
}

bool
HomeWindow::eventFilter(QObject *object, QEvent *e)
{
    if (!isVisible()) return false; // ignore when we aren't visible

    // mouse moved and tabbed -- should we show/hide chart popup controls?
    if (e->type() == QEvent::MouseMove && currentStyle == 0 && tabbed->currentIndex() >= 0) {

        if (tabbed->currentIndex() >= charts.count()) return false;

        QPoint pos = tabbed->widget(tabbed->currentIndex())->mapFromGlobal(QCursor::pos());
        GcWindow *us = charts[tabbed->currentIndex()];

        // lots of nested if statements to breakout as quickly as possible
        // this code gets called A LOT, since mouse events are from the 
        // application
        if (us->hasReveal()) { // does  this chart have reveal controls?

            if (us->underMouse()) { // is it even under the cursor?

                // mouse towards top so reveal
                if (us->revealed == false && pos.y() < 40) {
                    us->reveal();
                    us->revealed = true;
                }

                // hide as mouse moves away
                if (us->revealed == true && pos.y() > 80) {
                    us->unreveal();
                    us->revealed = false;
                }

            } else if (us->revealed) { // not under cursor but revealed
                us->unreveal();
                us->revealed = false;
            }
        }
    }

    // useful for tracking events across the charts
    //qDebug()<<QTime::currentTime()<<name<<"filter event"<<object<<e->type();

    // we watch the mouse when
    // dropping charts, to update
    // the cursor position
    // dropping and mouse move?
    if (object == this) {
        if (dropPending == true && e->type() == QEvent::DragMove) {
            QPoint pos = winWidget->mapFromGlobal(QCursor::pos());
            winArea->ensureVisible(pos.x(), pos.y(), 20, 20);
            chartCursor = pointTile(pos);
            winWidget->repaint(); // show cursor
        }
        return false;
    }

    // draw a cursor when winWidget paint event kicks off
    if (object == winWidget) {
        if (e->type() == QEvent::Paint) drawCursor();
        return false;
    }

    // From here on, we're looking at events
    // from the charts...

    // is it a mouse enter event?
    if (e->type() == QEvent::Enter) {

        // ok which one?
        for(int i=0; i<charts.count(); i++) {
            if (charts[i] == object) {

                // if we havent clicked on a chart lets set the controls
                // on a mouse over
                if (!clicked) {
                    controlStack->setCurrentIndex(i);
                    titleEdit->setText(charts[i]->property("title").toString());
                }

                // paint the border!
                charts[i]->repaint();
                return false;
            }
        }

    } else if (e->type() == QEvent::Leave) {

        // unpaint the border
        for(int i=0; i<charts.count(); i++) {
            if (charts[i] == object) {
                charts[i]->repaint();
                return false;
            }
        }
    } else if (e->type() == QEvent::MouseButtonPress) {

        int y = ((QMouseEvent*)(e))->pos().y();

        if (y<=15) {

            // on the title bar! -- which one?
            for(int i=0; i<charts.count(); i++) {
                if (charts[i] == object) {

                        if (charts[i] != clicked) { // we aren't clicking to toggle

                            // clear the chart that is currently clicked
                            // if at all (ie. null means no clicked chart
                            if (clicked) {
                                clicked->setProperty("active", false);
                                clicked->repaint();
                            }
                            charts[i]->setProperty("active", true);
                            charts[i]->repaint();
                            clicked = charts[i];
                            controlStack->setCurrentIndex(i);
                            titleEdit->setText(charts[i]->property("title").toString());

                        } else { // already clicked so unclick it (toggle)
                            charts[i]->setProperty("active", false);
                            charts[i]->repaint();
                            clicked = NULL;
                        }
                }
            }
        }
        return false;
    }
    return false;
}

// which tile to place before, -1 means append
int
HomeWindow::pointTile(QPoint pos)
{
    // find the window that is to the right of
    // the drop point

    // is the cursor above a widget?
    int i;
    int here = -1; // chart number to the right of cursor...
    for (i=0; i<charts.count(); i++) {
        if (charts[i]->geometry().contains(pos)) {
            here = i;
            break;
        }
    }

    //  is it to the right?
    if (here == -1) {

        int toRight = pos.x();
        while (here == -1 && toRight < winArea->width()) {
            toRight += 20;
            QPoint right = QPoint(toRight, pos.y());
            for (i=0; i<charts.count(); i++) {
                if (charts[i]->geometry().contains(right)) {
                    here = i;
                    break;
                }
            }
        }
    }

    //  is it to the left?
    if (here == -1) {

        int toLeft = pos.x();
        while (here == -1 && toLeft > 0) {
            toLeft -= 20;
            QPoint left = QPoint(toLeft, pos.y());
            for (i=0; i<charts.count(); i++) {
                if (charts[i]->geometry().contains(left)) {
                    here = i+1;
                    break;
                }
            }
        }
    }

    // well it must be below, so if there is
    // anything at y+20, use that, otherwise
    // we can assume we are at the end and
    // append to the list...
    if (here == -1) {
        QPoint below(20, pos.y()+20);
        for (i=0; i<charts.count(); i++) {
            if (charts[i]->geometry().contains(below)) {
                here = i;
                break;
            }
        }
    }

    // all done
    if (here == -1 || here >= charts.count()) return -1;
    return here;
}

void
HomeWindow::windowMoved(GcWindow*w)
{
    // fix single click error
    if (chartCursor == -2) return; // no it didn't!

    // now drop at the cursor
    if (currentStyle == 2) {
        // go find...
        int before=0;
        for (;before < charts.count(); before++) {
            if (charts[before] == w) {
                QWidget *m = winFlow->takeAt(before)->widget();
                QWidget *c = controlStack->widget(before);
                controlStack->removeWidget(c);
                QWidget *l = charts.takeAt(before);
                if (chartCursor > before) chartCursor--;
                if (chartCursor >= 0) {
                    controlStack->insertWidget(chartCursor, c);
                    winFlow->insert(chartCursor, m);
                    charts.insert(chartCursor, dynamic_cast<GcWindow*>(l));
                } else {
                    controlStack->addWidget(c);
                    winFlow->addWidget(m);
                    charts.append(dynamic_cast<GcWindow*>(l));
                }
                break;
            }
        }
        winFlow->update();
        chartCursor = -2;
        winWidget->repaint();
    }

    // remove the cursor
    chartCursor = -2; // -2 means don't show
}

void
HomeWindow::windowResized(GcWindow* /*w*/)
{
}

void
HomeWindow::windowMoving(GcWindow* /*w*/)
{
    // ensure the mouse pointer is visible, scrolls
    // as we get near to the margins...
    QPoint pos = winWidget->mapFromGlobal(QCursor::pos());
    winArea->ensureVisible(pos.x(), pos.y(), 20, 20);

    chartCursor = pointTile(pos);
    winWidget->repaint(); // show cursor

}

void
HomeWindow::windowResizing(GcWindow* /*w*/)
{
    QPoint pos = winWidget->mapFromGlobal(QCursor::pos());
    winArea->ensureVisible(pos.x(), pos.y(), 20, 20);
}

void
HomeWindow::drawCursor()
{
    if (chartCursor == -2) return;

    QPainter painter(winWidget);

    // lets draw to the left of the chart...
    if (chartCursor > -1) {

        // background light gray for now?
        QRect line(charts[chartCursor]->geometry().x()-12,
                   charts[chartCursor]->geometry().y(),
                   4, 
                   charts[chartCursor]->geometry().height());

        painter.setPen(Qt::NoPen);
        painter.setBrush(QBrush(Qt::white));
        painter.drawRect(line);

    }

    // lets draw at to the right of the
    // last chart...
    if (chartCursor == -1 && charts.count()) {

        // background light gray for now?
        QRect line(charts[charts.count()-1]->geometry().x() + charts[charts.count()-1]->width() + 8,
                   charts[charts.count()-1]->geometry().y(),
                   4, 
                   charts[charts.count()-1]->geometry().height());

        painter.setPen(Qt::NoPen);
        painter.setBrush(QBrush(Qt::white));
        painter.drawRect(line);

    }
}

GcWindowDialog::GcWindowDialog(GcWinID type, Context *context, GcWindow **here) : context(context), type(type), here(here)
{
    //setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags());
    setWindowTitle(tr("Chart Setup"));
    setMinimumHeight(500);
    setMinimumWidth(800);
    setWindowModality(Qt::ApplicationModal);

    mainLayout = new QVBoxLayout(this);
    layout = new QHBoxLayout();
    mainLayout->addLayout(layout);
    chartLayout = new QVBoxLayout;
    layout->addLayout(chartLayout);

    title = new QLineEdit(this);
    chartLayout->addWidget(title);
    title->setText(GcWindowRegistry::title(type));

    win = GcWindowRegistry::newGcWindow(type, context);
    chartLayout->addWidget(win);
    //win->setFrameStyle(QFrame::Box);

    // lets not have space for controls if there aren't any
    layout->setStretch(0, 100);
    if (win->controls()) {
        controlLayout = new QFormLayout;
        controlLayout->addRow(win->controls());
        layout->addLayout(controlLayout);
        layout->setStretch(1, 50);
    }

    RideItem *notconst = (RideItem*)context->currentRideItem();
    win->setProperty("ride", QVariant::fromValue<RideItem*>(notconst));
    DateRange dr = context->currentDateRange();
    win->setProperty("dateRange", QVariant::fromValue<DateRange>(dr));


    QHBoxLayout *buttons = new QHBoxLayout;
    mainLayout->addLayout(buttons);

    buttons->addStretch();
    buttons->addWidget((cancel=new QPushButton(tr("Cancel"), this)));
    buttons->addWidget((ok=new QPushButton(tr("OK"), this)));

    connect(ok, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));

    hide();
}

void GcWindowDialog::okClicked()
{
    // give back to owner so we can re-use
    // note that in reject they are not and will
    // get deleted (this has been verified with
    // some debug statements in ~GcWindow).

    // set its title property and geometry factors
    win->setProperty("title", title->text());
    win->setProperty("widthFactor", 2.00);
    win->setProperty("heightFactor", 2.00);
    win->setProperty("GcWinID", type);
    accept();
}

void GcWindowDialog::cancelClicked() { reject(); }

int
GcWindowDialog::exec()
{
    if (QDialog::exec()) {

        *here = win;
    } else {
        *here = NULL;
    } 
    return 0;
}

/*----------------------------------------------------------------------
 * Save and restore state (xxxx-layout.xml)
 *--------------------------------------------------------------------*/
// static helper to protect special xml characters
// ideally we would use XMLwriter to do this but
// the file format is trivial and this implementation
// is easier to follow and modify... for now.
static QString xmlprotect(QString string)
{
    QTextEdit trademark("&#8482;"); // process html encoding of(TM)
    QString tm = trademark.toPlainText();

    QString s = string;
    s.replace( tm, "&#8482;" );
    s.replace( "&", "&amp;" );
    s.replace( ">", "&gt;" );
    s.replace( "<", "&lt;" );
    s.replace( "\"", "&quot;" );
    s.replace( "\'", "&apos;" );
    return s;
}

static QString unprotect(QString buffer)
{
    // get local TM character code
    QTextEdit trademark("&#8482;"); // process html encoding of(TM)
    QString tm = trademark.toPlainText();

    // remove quotes
    QString s = buffer.trimmed();

    // replace html (TM) with local TM character
    s.replace( "&#8482;", tm );

    // html special chars are automatically handled
    // NOTE: other special characters will not work
    // cross-platform but will work locally, so not a biggie
    // i.e. if the default charts.xml has a special character
    // in it it should be added here
    return s;
}

void
HomeWindow::saveState()
{
    // run through each chart and save its USER properties
    // we do not save all the other Qt properties since
    // we're not interested in them
    // NOTE: currently we support QString, int, double and bool types - beware custom types!!
    if (charts.count() == 0) return; // don't save empty, use default instead

    QString filename = context->athlete->home->config().canonicalPath() + "/" + name + "-layout.xml";
    QFile file(filename);
    file.open(QFile::WriteOnly);
    file.resize(0);
    QTextStream out(&file);
    out.setCodec("UTF-8");

    out<<"<layout name=\""<< name <<"\" style=\"" << currentStyle <<"\">\n";

    // iterate over charts
    foreach (GcWindow *chart, charts) {
        GcWinID type = chart->property("type").value<GcWinID>();

        out<<"\t<chart id=\""<<static_cast<int>(type)<<"\" "
           <<"name=\""<<xmlprotect(chart->property("instanceName").toString())<<"\" "
           <<"title=\""<<xmlprotect(chart->property("title").toString())<<"\" >\n";

        // iterate over chart properties
        const QMetaObject *m = chart->metaObject();
        for (int i=0; i<m->propertyCount(); i++) {
            QMetaProperty p = m->property(i);
            if (p.isUser(chart)) {
               out<<"\t\t<property name=\""<<xmlprotect(p.name())<<"\" "
                  <<"type=\""<<p.typeName()<<"\" "
                  <<"value=\"";

                if (QString(p.typeName()) == "int") out<<p.read(chart).toInt();
                if (QString(p.typeName()) == "double") out<<p.read(chart).toDouble();
                if (QString(p.typeName()) == "QDate") out<<p.read(chart).toDate().toString();
                if (QString(p.typeName()) == "QString") out<<xmlprotect(p.read(chart).toString());
                if (QString(p.typeName()) == "bool") out<<p.read(chart).toBool();
                if (QString(p.typeName()) == "LTMSettings") {
                    QByteArray marshall;
                    QDataStream s(&marshall, QIODevice::WriteOnly);
                    LTMSettings x = p.read(chart).value<LTMSettings>();
                    s << x;
                    out<<marshall.toBase64();
                }

                out<<"\" />\n";
            }
        }
        out<<"\t</chart>\n";
    }
    out<<"</layout>\n";
    file.close();
}

void
HomeWindow::restoreState(bool useDefault)
{
    bool defaultUsed = false;

    // restore window state
    QString filename = context->athlete->home->config().canonicalPath() + "/" + name + "-layout.xml";
    QFileInfo finfo(filename);

    if (useDefault) QFile::remove(filename);

    // use a default if not there
    if (!finfo.exists()) {
        filename = QString(":xml/%1-layout.xml").arg(name);
        defaultUsed = true;
    }

    // now go read...
    QFile file(filename);

    // setup the handler
    QXmlInputSource source(&file);
    QXmlSimpleReader xmlReader;
    ViewParser handler(context);
    xmlReader.setContentHandler(&handler);
    xmlReader.setErrorHandler(&handler);

    // parse and instantiate the charts
    xmlReader.parse(source);
    // translate the titles
    translateChartTitles(handler.charts);

    // translate the metrics, but only if the built-in "default.XML"s are read (and only for LTM charts)
    if (defaultUsed) {
        // define and fill translation maps
        QMap<QString, QString> nMap;  // names
        QMap<QString, QString> uMap;  // unit of measurement
        LTMTool::getMetricsTranslationMap(nMap, uMap, context->athlete->useMetricUnits);

        // check all charts for LTMWindow(s)
        for (int i=0; i<handler.charts.count(); i++) {
            // find out if it's an LTMWindow via dynamic_cast
            LTMWindow* ltmW = dynamic_cast<LTMWindow*> (handler.charts[i]);
            if (ltmW) {
                // the current chart is an LTMWindow, let's translate

                // now get the LTMMetrics
                LTMSettings workSettings = ltmW->getSettings();
                for (int j=0; j<workSettings.metrics.count(); j++){
                    // now map and substitute
                    QString n  = nMap.value(workSettings.metrics[j].symbol, workSettings.metrics[j].uname);
                    QString u  = uMap.value(workSettings.metrics[j].symbol, workSettings.metrics[j].uunits);
                    // set name, units only if there was a description before
                    if (workSettings.metrics[j].name != "") workSettings.metrics[j].name = n;
                    workSettings.metrics[j].uname = n;
                    if (workSettings.metrics[j].units != "") workSettings.metrics[j].units = u;
                    workSettings.metrics[j].uunits = u;
                }
                ltmW->applySettings(workSettings);
            }
        }
    }

    // layout the results
    styleChanged(handler.style);
    foreach(GcWindow *chart, handler.charts) addChart(chart);
}

//
// view layout parser - reads in athletehome/xxx-layout.xml
//
bool ViewParser::startDocument()
{
    return true;
}

bool ViewParser::endElement( const QString&, const QString&, const QString &qName )
{
    if (qName == "chart") { // add to the list
        charts.append(chart);
    }
    return true;
}

bool ViewParser::startElement( const QString&, const QString&, const QString &name, const QXmlAttributes &attrs )
{
    if (name == "layout") {
        for(int i=0; i<attrs.count(); i++) {
            if (attrs.qName(i) == "style") {
                style = unprotect(attrs.value(i)).toInt();
            }
        }
    }
    else if (name == "chart") {

        QString name="", title="", typeStr="";
        GcWinID type;

        // get attributes
        for(int i=0; i<attrs.count(); i++) {
            if (attrs.qName(i) == "name") name = unprotect(attrs.value(i));
            if (attrs.qName(i) == "title") title = unprotect(attrs.value(i));
            if (attrs.qName(i) == "id")  typeStr = unprotect(attrs.value(i));
        }

        // new chart
        type = static_cast<GcWinID>(typeStr.toInt());
        chart = GcWindowRegistry::newGcWindow(type, context);
        chart->hide();
        chart->setProperty("title", QVariant(title));

    }
    else if (name == "property") {

        QString name, type, value;

        // get attributes
        for(int i=0; i<attrs.count(); i++) {
            if (attrs.qName(i) == "name") name = unprotect(attrs.value(i));
            if (attrs.qName(i) == "value") value = unprotect(attrs.value(i));
            if (attrs.qName(i) == "type")  type = unprotect(attrs.value(i));
        }

        // set the chart property
        if (type == "int") chart->setProperty(name.toLatin1(), QVariant(value.toInt()));
        if (type == "double") chart->setProperty(name.toLatin1(), QVariant(value.toDouble()));

        // deprecate dateRange asa chart property THAT IS DSAVED IN STATE
        if (type == "QString" && name != "dateRange") chart->setProperty(name.toLatin1(), QVariant(QString(value)));
        if (type == "QDate") chart->setProperty(name.toLatin1(), QVariant(QDate::fromString(value)));
        if (type == "bool") chart->setProperty(name.toLatin1(), QVariant(value.toInt() ? true : false));
        if (type == "LTMSettings") {
            QByteArray base64(value.toLatin1());
            QByteArray unmarshall = QByteArray::fromBase64(base64);
            QDataStream s(&unmarshall, QIODevice::ReadOnly);
            LTMSettings x;
            s >> x;
            chart->setProperty(name.toLatin1(), QVariant().fromValue<LTMSettings>(x));
        }

    }
    return true;
}

bool ViewParser::characters( const QString&)
{
    return true;
}


bool ViewParser::endDocument()
{
    return true;
}

void HomeWindow::closeWindow(GcWindow*thisone)
{
    if (charts.contains(thisone)) removeChart(charts.indexOf(thisone));
}

void HomeWindow::translateChartTitles(QList<GcWindow*> charts)
{
    // Map default (english) title to external (Localized) name, new default
    // charts in *layout.xml need to be added to this list to be translated
    QMap<QString, QString> titleMap;
    titleMap.insert("Ride Log", tr("Ride Log"));
    titleMap.insert("Aerobic Power", tr("Aerobic Power"));
    titleMap.insert("Anaerobic Power", tr("Anaerobic Power"));
    titleMap.insert("Cadence", tr("Cadence"));
    titleMap.insert("Calendar", tr("Calendar"));
    titleMap.insert("CP", tr("CP"));
    titleMap.insert("Details", tr("Details"));
    titleMap.insert("Distance", tr("Distance"));
    titleMap.insert("Edit", tr("Edit"));
    titleMap.insert("Elapsed Time", tr("Elapsed Time"));
    titleMap.insert("Heartrate", tr("Heartrate"));
    titleMap.insert("Lap", tr("Lap"));
    titleMap.insert("Map", tr("Map"));
    titleMap.insert("Performance", tr("Performance"));
    titleMap.insert("PMC", tr("PMC"));
    titleMap.insert("Power", tr("Power"));
    titleMap.insert("QA", tr("QA"));
    titleMap.insert("Ride", tr("Ride"));
    titleMap.insert("Speed", tr("Speed"));
    titleMap.insert("Summary", tr("Summary"));
    titleMap.insert("Target Power", tr("Target Power"));
    titleMap.insert("Time and Distance", tr("Time and Distance"));
    titleMap.insert("Time In Zone", tr("Time In Zone"));
    titleMap.insert("Training Mix", tr("Training Mix"));
    titleMap.insert("W/kg", tr("W/kg"));
    titleMap.insert("Workout", tr("Workout"));
    titleMap.insert("Stress", tr("Stress"));
    titleMap.insert("Scatter", tr("Scatter"));
    titleMap.insert("HrPw", tr("HrPw"));
    titleMap.insert("Activity Log", tr("Activity Log"));
    titleMap.insert("HrPw", tr("HrPw"));
    titleMap.insert("Tracker", tr("Tracker"));
    titleMap.insert("CP History", tr("CP History"));
    titleMap.insert("Library", tr("Library"));

    foreach(GcWindow *chart, charts) {
        QString chartTitle = chart->property("title").toString();
        chart->setProperty("title", titleMap.value(chartTitle, chartTitle));
    }
}
