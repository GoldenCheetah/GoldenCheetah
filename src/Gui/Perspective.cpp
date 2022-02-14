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
#include "AthleteTab.h"
#include "AbstractView.h"
#include "Perspective.h"
#include "LTMTool.h"
#include "LTMSettings.h"
#include "LTMWindow.h"
#include "Settings.h"
#include "GcUpgrade.h" // for VERSION_CONFIG_PREFIX url to -layout.xml
#include "LTMSettings.h" // for special case of edit LTM settings
#include "Overview.h" // for special case of Overview defaults
#include "ChartBar.h"
#include "Utils.h"
#include "SearchBox.h"

#define PERSPECTIVE_DEBUG false

#ifndef PERSPECTIVE_DEBUG
#define PERSPECTIVE_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (PERSPECTIVE_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (PERSPECTIVE_DEBUG) {                                       \
            fprintf(stderr, "[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stderr);                                             \
        }                                                               \
    } while(0)
#endif

#define SSS printd("\n")

// When ESC pressed during R processing we cancel it
#ifdef GC_WANT_R
#include "RTool.h"
#endif

#include <QDesktopWidget>
#include <QStyle>
#include <QStyleFactory>
#include <QScrollBar>

// getting config via URL
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
static const int tileMargin = 20;
static const int tileSpacing = 10;

Perspective::Perspective(Context *context, QString title, int type) :
    GcWindow(context), context(context), active(false),  resizing(false), clicked(NULL), dropPending(false),
    type_(type), title_(title), chartCursor(-2), df(NULL), expression_(""), trainswitch(None)
{
    SSS;
    // setup control area
    QWidget *cw = new QWidget(this);
    cw->setContentsMargins(0,0,0,0);

    QVBoxLayout *cl = new QVBoxLayout(cw);
    cl->setSpacing(0);
    cl->setContentsMargins(0,0,0,0);

    QLabel *titleLabel = new QLabel(tr("Title"), this);
    QHBoxLayout *hl = new QHBoxLayout;
    hl->setSpacing(5 *dpiXFactor);
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

    switch(this->type_) {
    case VIEW_ANALYSIS: view="analysis"; break;
    case VIEW_DIARY: view="diary"; break;
    case VIEW_TRENDS: view="home"; break;
    case VIEW_TRAIN: view="train"; break;
    }
    setProperty("isManager", true);
    nomenu=true;
    setAcceptDrops(true);

    QVBoxLayout *layout = new QVBoxLayout(this);

    style = new QStackedWidget(this);
    style->setAutoFillBackground(false);
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(style);

    QPalette palette;
    //palette.setBrush(backgroundRole(), QColor("#B3B4B6"));
    palette.setBrush(backgroundRole(), type == VIEW_TRAIN ? GColor(CTRAINPLOTBACKGROUND) : GColor(CPLOTBACKGROUND));
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
    tabLayout->addWidget(new ProgressLine(this, context));
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
    styleChanged(2, true);

    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(this, SIGNAL(dateRangeChanged(DateRange)), this, SLOT(dateRangeChanged(DateRange)));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    //connect(tabbed, SIGNAL(currentChanged(int)), this, SLOT(tabSelected(int)));
    //connect(tabbed, SIGNAL(tabCloseRequested(int)), this, SLOT(removeChart(int)));
    connect(chartbar, SIGNAL(itemMoved(int,int)), this, SLOT(tabMoved(int,int)));
    connect(chartbar, SIGNAL(currentIndexChanged(int)), this, SLOT(tabSelected(int)));
    connect(chartbar, SIGNAL(contextMenu(int,int)), this, SLOT(tabMenu(int,int)));
    connect(titleEdit, SIGNAL(textChanged(const QString&)), SLOT(titleChanged()));

    // trends view we should select a library chart when a chart is selected.
    if (type == VIEW_TRENDS) connect(context, SIGNAL(presetSelected(int)), this, SLOT(presetSelected(int)));

    // Allow realtime controllers to scroll train view with steering movements
    if (type == VIEW_TRAIN) connect(context, SIGNAL(steerScroll(int)), this, SLOT(steerScroll(int)));

    installEventFilter(this);
    qApp->installEventFilter(this);
}

Perspective::~Perspective()
{
    SSS;
    qApp->removeEventFilter(this);
}

void
Perspective::rightClick(const QPoint & /*pos*/)
{
    SSS;
    return; //deprecated right click on homewindow -- bad UX
}

void
Perspective::addChartFromMenu(QAction*action)
{
    SSS;
    // & removed to avoid issues with kde AutoCheckAccelerators
    QString actionText = QString(action->text()).replace("&", "");
    GcWinID id = GcWindowTypes::None;
    for (int i=0; GcWindows[i].relevance; i++) {
        if (GcWindows[i].name == actionText) {
            id = GcWindows[i].id;
            break;
        }
    }

    if (id != GcWindowTypes::None) {
        appendChart(id); // called from Context to inset chart
    }
}

void
Perspective::importChart(QMap<QString,QString>properties, bool select)
{
    SSS;
    // turn off updates whilst we do this...
    setUpdatesEnabled(false);

    // what type?
    GcWinID type = static_cast<GcWinID>(properties.value("TYPE","1").toInt());

    GcChartWindow *chart = GcWindowRegistry::newGcWindow(type, context);

    // bad chart file !
    if (chart == NULL) {
        setUpdatesEnabled(true);

        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(tr("Unable to process chart file"));
        msgBox.setInformativeText(QString(tr("Bad chart type (%1).")).arg(static_cast<int>(type)));
        msgBox.exec();
        return;
    }

    // hide it before doing anything!
    chart->hide();

    // metaobject describes properties
    const QMetaObject *m = chart->metaObject();

    // set all the properties
    chart->setProperty("view", view);

    // each of the user properties
    QMapIterator<QString,QString> prop(properties);
    prop.toFront();
    while(prop.hasNext()) {
        prop.next();
        if (prop.key() == "VIEW" || prop.key()=="TYPE") continue;

        // ok, we have a property
        for (int i=0; i<m->propertyCount(); i++) {
            QMetaProperty p = m->property(i);
            if (p.name() == prop.key()) {

                // ok we have a winner, how to format it?
                if (QString(p.typeName()) == "int") chart->setProperty(prop.key().toLatin1(), prop.value().toInt());
                if (QString(p.typeName()) == "double") chart->setProperty(prop.key().toLatin1(), prop.value().toDouble());
                if (QString(p.typeName()) == "QDate") chart->setProperty(prop.key().toLatin1(), QDate::fromString(prop.value()));
                if (QString(p.typeName()) == "QString") chart->setProperty(prop.key().toLatin1(), Utils::jsonunprotect(prop.value()));
                if (QString(p.typeName()) == "bool") chart->setProperty(prop.key().toLatin1(), prop.value().toInt());
                if (QString(p.typeName()) == "LTMSettings") {
                    QByteArray base64(prop.value().toLatin1());
                    QByteArray unmarshall = QByteArray::fromBase64(base64);
                    QDataStream s(&unmarshall, QIODevice::ReadOnly);
                    LTMSettings x;
                    s >> x;
                    chart->setProperty(prop.key().toLatin1(), QVariant().fromValue<LTMSettings>(x));
                }
            }
        }
    }

    // now set managed properties etc
    addChart(chart);

    // set to whatever we have selected
    RideItem *notconst = (RideItem*)context->currentRideItem();
    chart->setProperty("ride", QVariant::fromValue<RideItem*>(notconst));
    chart->setProperty("dateRange", property("dateRange"));

    if (select && charts.count())  tabSelected(charts.count()-1);

    setUpdatesEnabled(true);
}

void
Perspective::configChanged(qint32)
{
    SSS;
    // update scroll bar
//#ifndef Q_OS_MAC
    tileArea->verticalScrollBar()->setStyleSheet(AbstractView::ourStyleSheet());
//#endif
    QPalette palette;
    palette.setBrush(backgroundRole(), type() == VIEW_TRAIN ? GColor(CTRAINPLOTBACKGROUND) : GColor(CPLOTBACKGROUND));
    setPalette(palette);
    tileWidget->setPalette(palette);
    tileArea->setPalette(palette);
    winWidget->setPalette(palette);
    winArea->setPalette(palette);

    // basic chart setup
    for (int i=0; i<charts.count(); i++) {
        if (currentStyle == 0) {
            if (charts[i]->type() == GcWindowTypes::Overview || charts[i]->type() == GcWindowTypes::OverviewTrends) chartbar->setColor(i, GColor(COVERVIEWBACKGROUND));
            else if (charts[i]->type() == GcWindowTypes::UserAnalysis || charts[i]->type() == GcWindowTypes::UserTrends) chartbar->setColor(i, RGBColor(QColor(charts[i]->property("color").toString())));
            else {
                if (type() == VIEW_TRAIN)chartbar->setColor(i, GColor(CTRAINPLOTBACKGROUND));
                else chartbar->setColor(i, GColor(CPLOTBACKGROUND));
            }
        }

        // set top margin
        charts[i]->setContentsMargins(0,
                                      (currentStyle==0 ? 0 : 15) * dpiXFactor,
                                      0, 0);

    }
}

void
Perspective::selected()
{
    SSS;
    setUpdatesEnabled(false);

    resize();
    rideSelected();
    dateRangeChanged(DateRange());
    setUpdatesEnabled(true);
    //update();
}

void
Perspective::titleChanged()
{
    SSS;
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

// we have a user chart and its just changed its config
void
Perspective::userChartConfigChanged(UserChartWindow *chart)
{
    if (!currentStyle) {
        // let chartbar know...
        for(int index=0; index < charts.count(); index++) {
            if (charts[index] == (GcChartWindow*)(chart)) {
                chartbar->setColor(index, RGBColor(QColor(charts[index]->property("color").toString())));
                return;
            }
        }
    }
}

void
Perspective::rideSelected()
{
    SSS;
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
Perspective::tabMenu(int index, int x)
{
    SSS;
    // activate this tab's menu
    QPoint pos = QPoint(x, mapToGlobal(chartbar->geometry().bottomLeft()).y()+(2*dpiXFactor));
    charts[index]->menu->exec(pos);
}

void
Perspective::dateRangeChanged(DateRange dr)
{ Q_UNUSED( dr )
    SSS;
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
Perspective::tabSelected(int index)
{
    SSS;
    // user switched tabs -- tell tabe to redraw!
    if (active || currentStyle != 0) return;

    active = true;

    if (index >= 0) {

        if (currentStyle == 0) charts[index]->setContentsMargins(0,0,0,0);

        // show
        charts[index]->show();
        controlStack->setCurrentIndex(index);
        titleEdit->setText(charts[index]->property("title").toString());
        tabbed->setCurrentIndex(index);
        chartbar->setCurrentIndex(index);

        // set
        charts[index]->setProperty("ride", property("ride"));
        charts[index]->setProperty("dateRange", property("dateRange"));
    }

    active = false;
}

void
Perspective::tabSelected(int index, bool forride)
{
    SSS;
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
Perspective::tabMoved(int to, int from)
{
    SSS;
     GcChartWindow *me = charts.takeAt(from);
     charts.insert(to, me);

    // re-order the controls - to reflect new indexes
    controlStack->blockSignals(true);
    QWidget *w = controlStack->widget(from);
    controlStack->removeWidget(w);
    controlStack->insertWidget(to, w);
    controlStack->setCurrentIndex(to);
    controlStack->blockSignals(false);

    // re-order the stack - to reflect new indexes
    tabbed->blockSignals(true);
    w = tabbed->widget(from);
    tabbed->removeWidget(w);
    tabbed->insertWidget(to, w);
    tabbed->setCurrentIndex(to);
    tabbed->blockSignals(false);
}

void
Perspective::styleChanged(int id, bool force)
{
    SSS;
    // ignore if out of bounds or we're already using that style
    if (id > 2 || id < 0 || (id == currentStyle && !force)) return;
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
            charts[i]->setResizable(false); // we need to show on tab selection!
            charts[i]->setProperty("dateRange", property("dateRange"));
            charts[i]->showMore(false);
            charts[i]->menuButton->hide(); // we use tab button
            charts[i]->hide(); // we need to show on tab selection!
            charts[i]->setContentsMargins(0,0,0,0);
            break;
        case 1 : // they are lists in a GridLayout
            tileGrid->addWidget(charts[i], i,0);
            charts[i]->setContentsMargins(0,25*dpiYFactor,0,0);
            charts[i]->setResizable(false); // we need to show on tab selection!
            charts[i]->show();
            charts[i]->showMore(true);
            charts[i]->setProperty("dateRange", property("dateRange"));
            charts[i]->setProperty("ride", property("ride"));
            break;
        case 2 : // thet are in a FlowLayout
            winFlow->addWidget(charts[i]);
            charts[i]->setContentsMargins(0,15*dpiYFactor,0,0);
            charts[i]->setResizable(true); // we need to show on tab selection!
            charts[i]->show();
            charts[i]->showMore(true);
            charts[i]->setProperty("dateRange", property("dateRange"));
            charts[i]->setProperty("ride", property("ride"));
        default:
            break;
        }
        charts[i]->setProperty("style", id);

    }

    currentStyle = id;
    style->setCurrentIndex(currentStyle);

    if (currentStyle == 0 && charts.count()) tabSelected(0);

    resize();

    if (currentStyle == 0) {
        tabbed->show(); // QTabWidget setUpdatesEnabled bug
        chartbar->show(); // and resize artefact too.. tread carefully.
    }

    active = false;

    // now refresh as we are done
    setUpdatesEnabled(true);
    //update();

}

void
Perspective::dragEnterEvent(QDragEnterEvent *)
{
    SSS;
#if 0 // draw and drop chart no longer part of the UX
    if (event->mimeData()->formats().contains("application/x-qabstractitemmodeldatalist")) {
        event->accept();
        dropPending = true;
    }
#endif
}

void
Perspective::appendChart(GcWinID id)
{
    SSS;
    GcChartWindow *newone = NULL;

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
Perspective::dropEvent(QDropEvent *)
{
    SSS;
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
Perspective::showControls()
{
    SSS;
    context->tab->chartsettings()->adjustSize();
    context->tab->chartsettings()->show();
}

void
Perspective::addChart(GcChartWindow* newone)
{
    SSS;
    int chartnum = charts.count();

    active = true;

    if (newone) {

        // add the controls
        QWidget *x = dynamic_cast<GcChartWindow*>(newone)->controls();
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
        newone->setProperty("view", view);
        newone->setProperty("ride", QVariant::fromValue<RideItem*>(notconst));
        newone->setProperty("dateRange", property("dateRange"));
        newone->setProperty("style", currentStyle);
        newone->setProperty("perspective", QVariant::fromValue<Perspective*>(this));

        // add to tabs
        switch (currentStyle) {

        case 0 :
            newone->setResizable(false); // we need to show on tab selection!
            newone->showMore(false);
            //tabbed->addTab(newone, newone->property("title").toString());
            tabbed->addWidget(newone);
            chartbar->addWidget(newone->property("title").toString());

            // tab colors
            if (newone->type() == GcWindowTypes::Overview || newone->type() == GcWindowTypes::OverviewTrends) chartbar->setColor(chartnum, GColor(COVERVIEWBACKGROUND));
            else {
                if (type() == VIEW_TRAIN)chartbar->setColor(chartnum, GColor(CTRAINPLOTBACKGROUND));
                else chartbar->setColor(chartnum, GColor(CPLOTBACKGROUND));
            }

            // lets not bother with a title in tab view- its in the name of the tab already!
            newone->setContentsMargins(0,0,0,0);
            break;
        case 1 :
            {
            // add to tiles
            // set geometry
            newone->setFixedWidth((tileArea->width()-50));
            newone->setFixedHeight(newone->width() * 0.7);
            newone->setResizable(false); // we need to show on tab selection!
            newone->showMore(true);
            int row = chartnum; // / 2;
            int column = 0; //chartnum % 2;
            newone->setContentsMargins(0,25*dpiYFactor,0,0);
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
                newone->setFixedHeight(newheight);
                newone->setContentsMargins(0,15*dpiYFactor,0,0);
                newone->setResizable(true); // we need to show on tab selection!
                newone->showMore(true);

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

GcChartWindow *
Perspective::takeChart(GcChartWindow *chart)
{
    SSS;
    // lets find it...
    int index = charts.indexOf(chart);

    if (index >= 0) {
        removeChart(index, false, true);
        return chart;
    }

    // failed to find it
    return NULL;
}

bool
Perspective::removeChart(int num, bool confirm, bool keep)
{
    SSS;
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

    ((GcChartWindow*)(charts[num]))->close(); // disconnect chart
    ((GcChartWindow*)(charts[num]))->removeEventFilter(this); // stop watching events
    if (!keep) ((GcChartWindow*)(charts[num]))->deleteLater();
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
Perspective::showEvent(QShowEvent *)
{
    SSS;

    // just before we show lets make sure it all looks good
    // bear in mind the perspective may have been loaded but
    // the formatting of contents doesn't happen till we have
    // a style and we are shown.
    for(int index=0; index<charts.count(); index++) {
        if (currentStyle == 0) charts[index]->setContentsMargins(0,0,0,0);
        else charts[index]->setContentsMargins(0,15*dpiXFactor,0,0);
    }

    resize();
}

void
Perspective::resizeEvent(QResizeEvent * /* e */)
{
    SSS;
    resize();
}

void Perspective::resize()
{
    SSS;
    if (resizing) return;

    resizing=true;

    foreach (GcChartWindow *x, charts) {

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
            x->setFixedHeight(newheight);
            }
            break;

        default:
            break;
        }
    }

    resizing=false;
}

bool
Perspective::eventFilter(QObject *object, QEvent *e)
{
    if (active || resizing || !isVisible()) return false; // ignore when we aren't visible or busy doing shit

#ifdef GC_WANT_R
    if (e->type() == QEvent::KeyPress && static_cast<QKeyEvent*>(e)->key()==Qt::Key_Escape) {
        // if we're running a script stop it
        if (rtool && rtool->canvas) rtool->cancel();
    }
#endif

    // mouse moved and tabbed -- should we show/hide chart popup controls?
    if (e->type() == QEvent::MouseMove && currentStyle == 0 && tabbed->currentIndex() >= 0) {

        if (tabbed->currentIndex() >= charts.count()) return false;

        QPoint pos = tabbed->widget(tabbed->currentIndex())->mapFromGlobal(QCursor::pos());
        GcChartWindow *us = charts[tabbed->currentIndex()];

        // lots of nested if statements to breakout as quickly as possible
        // this code gets called A LOT, since mouse events are from the 
        // application
        if (us->hasReveal()) { // does  this chart have reveal controls?

            if (us->underMouse()) { // is it even under the cursor?

                // mouse towards top so reveal
                if (us->revealed == false && pos.y() < (40*dpiYFactor)) {
                    us->reveal();
                    us->revealed = true;
                }

                // hide as mouse moves away
                if (us->revealed == true && pos.y() > (80*dpiYFactor)) {
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
Perspective::pointTile(QPoint pos)
{
    SSS;
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
Perspective::windowMoved(GcWindow*w)
{
    SSS;
    if (active || resizing) return;

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
                    charts.insert(chartCursor, dynamic_cast<GcChartWindow*>(l));
                } else {
                    controlStack->addWidget(c);
                    winFlow->addWidget(m);
                    charts.append(dynamic_cast<GcChartWindow*>(l));
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
Perspective::windowResized(GcWindow* /*w*/)
{
    SSS;
    if (active || resizing) return;
}

void
Perspective::windowMoving(GcWindow* /*w*/)
{
    SSS;
    if (active || resizing) return;

    // ensure the mouse pointer is visible, scrolls
    // as we get near to the margins...
    QPoint pos = winWidget->mapFromGlobal(QCursor::pos());
    winArea->ensureVisible(pos.x(), pos.y(), 20, 20);

    chartCursor = pointTile(pos);
    winWidget->repaint(); // show cursor

}

void
Perspective::windowResizing(GcWindow* /*w*/)
{
    SSS;
    if (active || resizing) return;

    QPoint pos = winWidget->mapFromGlobal(QCursor::pos());
    winArea->ensureVisible(pos.x(), pos.y(), 20, 20);
}

void
Perspective::drawCursor()
{
    SSS;
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

GcWindowDialog::GcWindowDialog(GcWinID type, Context *context, GcChartWindow **here, bool sidebar, LTMSettings *use) : context(context), type(type), here(here), sidebar(sidebar)
{
    SSS;
    //setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags());
    setWindowTitle(tr("Chart Setup"));

    QRect size= desktop->availableGeometry();
    setMinimumHeight(500);

    // chart and settings side by side need to be big!
    if (size.width() >= 1300) setMinimumWidth(1200); 
    else setMinimumWidth(800); // otherwise the old default
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

    // before we do anything, we need to set the perspective, in case
    // the chart uses it to decide something - apologies for the convoluted
    // method to determine the perspective, but its rare to use this outside
    // the context of a chart or a view
    win->setProperty("perspective", QVariant::fromValue<Perspective*>(context->mainWindow->athleteTab()->view(context->mainWindow->athleteTab()->currentView())->page()));
    chartLayout->addWidget(win);
    //win->setFrameStyle(QFrame::Box);

    // lets not have space for controls if there aren't any
    layout->setStretch(0, 100);
    if (win->controls()) {
        layout->addWidget(win->controls());
        layout->setStretch(1, 50);
    }

    // settings passed as we're editing LTM chart sidebar
    // bit of a hack, but good enough for now.
    if (type == GcWindowTypes::LTM && use) {
        static_cast<LTMWindow*>(win)->applySettings(*use);
        win->setProperty("title", use->name);
        title->setText(use->name);
    }

    // special case
    if (type == GcWindowTypes::Overview || type == GcWindowTypes::OverviewTrends) {
        static_cast<OverviewWindow*>(win)->setConfiguration("");
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

    // no basic tab for library charts - remove once all in place.
    if (sidebar) {
        // hide the basic tab
        static_cast<LTMWindow*>(win)->hideBasic();
    }

    connect(ok, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));

    hide();
}

void GcWindowDialog::okClicked()
{
    SSS;
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
    SSS;
    if (QDialog::exec()) {
        *here = win;

    } else {
        *here = NULL;
    }
    return 0;
}

/*----------------------------------------------------------------------
 * Scroll train window                                                 *
 * Used by Tacx trainers with steering sensors                         *
 * scrollAmount is:                                                    *
 * -1 = single step upwards scroll                                     *
 * -2 = page step upwards scroll                                       *
 * +1 = single step downwards scroll                                   *
 * +2 = page step downwards scroll                                     *
 *--------------------------------------------------------------------*/
void Perspective::steerScroll(int scrollAmount) {
    QScrollBar *vertScroll;
    switch (currentStyle) {
    case 1 : { vertScroll = tileArea->verticalScrollBar(); }
        break;
    case 2 : { vertScroll = winArea->verticalScrollBar(); }
        break;
    default:
        return;
    }

    int scrollPix;
    switch (scrollAmount) {
    case 1: { scrollPix = vertScroll->singleStep(); }
        break;
    case 2: { scrollPix = vertScroll->pageStep(); }
        break;
    case -1: { scrollPix = 0 - vertScroll->singleStep(); }
        break;
    case -2: { scrollPix = 0 - vertScroll->pageStep(); }
        break;
    default:
        return;
    }

    vertScroll->setValue(vertScroll->value() + scrollPix);
}

void Perspective::closeWindow(GcWindow*thisone)
{
    SSS;
    if (charts.contains(static_cast<GcChartWindow*>(thisone)))
        removeChart(charts.indexOf(static_cast<GcChartWindow*>(thisone)));
}

void Perspective::translateChartTitles(QList<GcChartWindow*> charts)
{
    SSS;
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
    titleMap.insert("W' In Zone", tr("W' In Zone"));
    titleMap.insert("Sustained In Zone", tr("Sustained In Zone"));
    titleMap.insert("Training Mix", tr("Training Mix"));
    titleMap.insert("Navigator", tr("Navigator"));
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
    titleMap.insert("CV", tr("CV"));

    foreach(GcChartWindow *chart, charts) {
        QString chartTitle = chart->property("title").toString();
        chart->setProperty("title", titleMap.value(chartTitle, chartTitle));
    }
}

void
Perspective::presetSelected(int n)
{
    SSS;
    if (n > 0) {

        // if we are in tabbed mode and we are not on a 'library' LTM chart
        // then we need to select a library chart to show the selection

        // tabbed is an LTM?
        if (!currentStyle && tabbed->currentIndex() >= 0 && charts.count() > 0) {

            int index = tabbed->currentIndex();
            GcWinID type = charts[index]->property("type").value<GcWinID>();

            // not on a 'library' chart
            if (type != GcWindowTypes::LTM || static_cast<LTMWindow*>(charts[index])->preset() == false) {

                // find a 'library' chart
                for(int n=0; n<charts.count(); n++) {
                    GcWinID type = charts[n]->property("type").value<GcWinID>();
                    if (type == GcWindowTypes::LTM) {
                        if (static_cast<LTMWindow*>(charts[n])->preset() == true) {
                            chartbar->setCurrentIndex(n);
                            break;
                        }
                    }
                }
            }
        }
    }
}

/*--------------------------------------------------------------------------------
 *  Import and Export the Perspective to xml
 * -----------------------------------------------------------------------------*/
Perspective *Perspective::fromFile(Context *context, QString filename, int type)
{
    SSS;
    Perspective *returning = NULL;

    QFileInfo finfo(filename);
    QString content = "";

    // no such file
    if (!finfo.exists()) return returning;

    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
        content = file.readAll();
        file.close();
    } else return returning;

    // parse the content
    QXmlInputSource source;
    source.setData(content);
    QXmlSimpleReader xmlReader;
    ViewParser handler(context, type, false);
    xmlReader.setContentHandler(&handler);
    xmlReader.setErrorHandler(&handler);

    // parse and instantiate the charts
    xmlReader.parse(source);

    // none loaded ?
    if (handler.perspectives.count() == 0) return returning;

    // return the first one with the right type (if there are multiple)
    for(int i=0; i<handler.perspectives.count(); i++)
        if (returning == NULL && handler.perspectives[i]->type_ == type)
            returning = handler.perspectives[i];

    // delete any further perspectives
    for(int i=0; i<handler.perspectives.count(); i++)
        if (handler.perspectives[i] != returning)
            delete (handler.perspectives[i]);

    // return it, but bear in mind it hasn't been initialised (current ride, date range etc)
    return returning;
}

bool
Perspective::toFile(QString filename)
{
    SSS;
    QFile file(filename);
    if (!file.open(QFile::WriteOnly)) return false;

    // truncate and use 8bit encoding
    file.resize(0);
    QTextStream out(&file);
    out.setCodec("UTF-8");

    // write to output stream
    toXml(out);

    // all done
    file.close();

    return true;
}

void
Perspective::toXml(QTextStream &out)
{
    SSS;
    out<<"<layout name=\""<< title_
       <<"\" style=\"" << currentStyle
       <<"\" type=\"" << type_
       <<"\" expression=\"" << Utils::xmlprotect(expression_)
       <<"\" trainswitch=\"" << (int)trainswitch
       << "\">\n";

    // iterate over charts
    foreach (GcChartWindow *chart, charts) {
        GcWinID type = chart->property("type").value<GcWinID>();

        out<<"\t<chart id=\""<<static_cast<int>(type)<<"\" "
           <<"name=\""<<Utils::xmlprotect(chart->property("instanceName").toString())<<"\" "
           <<"title=\""<<Utils::xmlprotect(chart->property("title").toString())<<"\" >\n";

        // iterate over chart properties
        const QMetaObject *m = chart->metaObject();
        for (int i=0; i<m->propertyCount(); i++) {
            QMetaProperty p = m->property(i);
            if (p.isUser(chart)) {
               out<<"\t\t<property name=\""<<Utils::xmlprotect(p.name())<<"\" "
                  <<"type=\""<<p.typeName()<<"\" "
                  <<"value=\"";

                if (QString(p.typeName()) == "int") out<<p.read(chart).toInt();
                if (QString(p.typeName()) == "double") out<<p.read(chart).toDouble();
                if (QString(p.typeName()) == "QDate") out<<p.read(chart).toDate().toString();
                if (QString(p.typeName()) == "QString") out<<Utils::xmlprotect(p.read(chart).toString());
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
}

/*--------------------------------------------------------------------------------
 *  Using an expression to switch/filter content
 * -----------------------------------------------------------------------------*/

QString
Perspective::expression() const
{
    SSS;
    return expression_;
}

void
Perspective::setExpression(QString expr)
{
    SSS;
    if (expression_ == expr) return;

    if (df) {
        delete df;
        df = NULL;
    }
    expression_ = expr;

    // non-blanks
    if (expression_ != "")
        df = new DataFilter(this, context, expression_);

    // notify charts that the filter changed
    // but only for trends views where it matters
    if (type_ == VIEW_TRENDS)
        foreach(GcWindow *chart, charts)
            chart->notifyPerspectiveFilterChanged(expression_);
}

bool
Perspective::relevant(RideItem *item)
{
    SSS;
    if (type_ != VIEW_ANALYSIS) return true;
    else if (df == NULL) return false;
    else if (df == NULL || item == NULL) return false;

    // validate
    Result ret = df->evaluate(item, NULL);
    return ret.number();

}

QStringList
Perspective::filterlist(DateRange dr)
{
    SSS;
    QStringList returning;

    Specification spec;
    spec.setDateRange(dr);

    foreach(RideItem *item, context->athlete->rideCache->rides()) {
        if (!spec.pass(item)) continue;

        // if no filter, or the filter passes add to count
        if (!isFiltered() || df->evaluate(item, NULL).number() != 0)
            returning << item->fileName;
    }

    return returning;
}

/*--------------------------------------------------------------------------------
 *  Import Chart Dialog - select/deselect charts before importing them
 * -----------------------------------------------------------------------------*/
ImportChartDialog::ImportChartDialog(Context *context, QList<QMap<QString,QString> >list, QWidget *parent) : QDialog(parent), context(context), list(list)
{
    SSS;
    setWindowFlags(windowFlags());
    setWindowTitle(tr("Import Charts"));
    setWindowModality(Qt::ApplicationModal);
    setMinimumWidth(450 * dpiXFactor);
    QVBoxLayout *layout = new QVBoxLayout(this);

    table = new QTableWidget(this);
    cancel = new QPushButton(tr("Cancel"), this);
    import = new QPushButton(tr("Import"), this);

    // set table
#ifdef Q_OS_MAC
    table->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    table->setRowCount(list.count());
    table->setColumnCount(3);
    QStringList headings;
    headings<<"";
    headings<<tr("View");
    headings<<tr("Title");
    table->setHorizontalHeaderLabels(headings);
    table->setSortingEnabled(false);
    table->verticalHeader()->hide();
    table->setShowGrid(false);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    // Populate the list of named searches
    for(int i=0; i<list.count(); i++) {

        // select XXX fix widget...
        QCheckBox *c = new QCheckBox(this);
        c->setChecked(true);
        table->setCellWidget(i, 0, c);

        QString view = list[i].value("VIEW");

        // convert to user name for view from here, since
        // they won't recognise the names used internally
        // as they need to be translated too
        if (view == "diary") view = tr("Diary");
        if (view == "home") view = tr("Trends");
        if (view == "analysis") view = tr("Activities");
        if (view == "train") view = tr("Train");

        QTableWidgetItem *t;
#ifndef GC_HAVE_ICAL
        // diary not available!
        if (view == tr("Diary"))  view = tr("Trends");
        // View
        t = new QTableWidgetItem;
        t->setText(view);
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        table->setItem(i, 1, t);

#else
        // we should be able to select trend/diary
        if (view == tr("Diary") || view == tr("Trends")) {

            QComboBox *com = new QComboBox(this);
            com->addItem(tr("Diary"));
            com->addItem(tr("Trends"));
            table->setCellWidget(i,1,com);
            if (view == tr("Diary")) com->setCurrentIndex(0);
            else com->setCurrentIndex(1);

        } else {

            // View
            t = new QTableWidgetItem;
            t->setText(view);
            t->setFlags(t->flags() & (~Qt::ItemIsEditable));
            table->setItem(i, 1, t);

        }
#endif

        // title
        t = new QTableWidgetItem;
        t->setText(list[i].value("title"));
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        table->setItem(i, 2, t);

    }

    layout->addWidget(table);

    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(import);
    buttons->addWidget(cancel);
    layout->addLayout(buttons);

    connect(import, SIGNAL(clicked(bool)), this, SLOT(importClicked()));
    connect(cancel, SIGNAL(clicked(bool)), this, SLOT(cancelClicked()));
}

void
ImportChartDialog::importClicked()
{
    SSS;
    // do stuff
    for(int i=0; i<list.count(); i++) {

        // is it checked?
        if (static_cast<QCheckBox*>(table->cellWidget(i,0))->isChecked()) {

            // where we putting it?
            QString view;

            // is there a combo box?
            QComboBox *com = static_cast<QComboBox*>(table->cellWidget(i,1));
            if (com) {
                switch(com->currentIndex()) {

                    case 0 : view = tr("Diary"); break;

                    default:
                    case 1 : view = tr("Trends"); break;
                }
            } else {
                view = table->item(i,1)->text();
            }

            int x=0;
            if (view == tr("Trends"))      { x=0; context->mainWindow->selectTrends(); }
            if (view == tr("Activities"))  { x=1; context->mainWindow->selectAnalysis(); }
            if (view == tr("Diary"))       { x=2; context->mainWindow->selectDiary(); }
            if (view == tr("Train"))       { x=3; context->mainWindow->selectTrain(); }

            // add to the currently selected tab and select if only adding one chart
            context->mainWindow->athleteTab()->view(x)->importChart(list[i], (list.count()==1));
        }
    }
    accept();
}

void
ImportChartDialog::cancelClicked()
{
    SSS;
    accept();
}

AddPerspectiveDialog::AddPerspectiveDialog(QWidget *parent, Context *context, QString &name, QString &expression, int type, Perspective::switchenum &trainswitch, bool edit) :
    QDialog(parent), context(context), name(name), expression(expression), trainswitch(trainswitch), type(type)
{
    SSS;
    setWindowFlags(windowFlags());
    if (edit) setWindowTitle(tr("Edit Perspective"));
    else setWindowTitle(tr("Add Perspective"));
    setWindowModality(Qt::ApplicationModal);
    setMinimumWidth(200 * dpiXFactor);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QFormLayout *form = new QFormLayout();
    nameEdit = new QLineEdit(this);
    nameEdit->setText(name);
    form->addRow(new QLabel(tr("Perspective Name")), nameEdit);
    layout->addLayout(form);

    if (type == VIEW_ANALYSIS || type == VIEW_TRENDS) {
        filterEdit = new SearchBox(context, this);
        filterEdit->setFixedMode(true);
        filterEdit->setMode(SearchBox::Filter);
        filterEdit->setText(expression);
        if (type == VIEW_ANALYSIS) form->addRow(new QLabel(tr("Switch expression")), filterEdit);
        if (type == VIEW_TRENDS) form->addRow(new QLabel(tr("Activities filter")), filterEdit);
    }

    if (type == VIEW_TRAIN) {
        trainSwitch = new QComboBox(this);
        trainSwitch->addItem(tr("Don't switch"), Perspective::None);
        trainSwitch->addItem(tr("Erg Workout"), Perspective::Erg);
        trainSwitch->addItem(tr("Slope Workout"), Perspective::Slope);
        trainSwitch->addItem(tr("Video Workout"), Perspective::Video);
        trainSwitch->setCurrentIndex(trainswitch);
        form->addRow(new QLabel(tr("Switch for")), trainSwitch);
    }

    QHBoxLayout *buttons = new QHBoxLayout();
    if (edit) add = new QPushButton(tr("OK"), this);
    else add = new QPushButton(tr("Add"), this);
    cancel = new QPushButton(tr("Cancel"), this);
    buttons->addStretch();
    buttons->addWidget(cancel);
    buttons->addWidget(add);
    layout->addLayout(buttons);

    connect(add, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
}

void
AddPerspectiveDialog::addClicked()
{
    SSS;
    name = nameEdit->text();
    if (type == VIEW_ANALYSIS || type == VIEW_TRENDS) expression = filterEdit->text();
    if (type == VIEW_TRAIN) trainswitch=(Perspective::switchenum)trainSwitch->itemData(trainSwitch->currentIndex(), Qt::UserRole).toInt();
    accept();
}

void
AddPerspectiveDialog::cancelClicked()
{
    SSS;
    reject();
}
