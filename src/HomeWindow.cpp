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


#include "HomeWindow.h"
#include "LTMSettings.h"
#include "Settings.h"

#include <QGraphicsDropShadowEffect>

HomeWindow::HomeWindow(MainWindow *mainWindow, QString name, QString /* windowtitle */) :
    GcWindow(mainWindow), mainWindow(mainWindow), name(name), active(false),
    clicked(NULL), dropPending(false), chartCursor(-2), loaded(false)
{
    setInstanceName("Home Window");

    // setup control area
    QWidget *cw = new QWidget(this);
    QVBoxLayout *cl = new QVBoxLayout(cw);
    QLabel *titleLabel = new QLabel("Title", this);
    QHBoxLayout *hl = new QHBoxLayout;

    titleEdit = new QLineEdit(this);
    titleEdit->setEnabled(false);
    controlStack = new QStackedWidget(this);
    hl->addWidget(titleLabel);
    hl->addWidget(titleEdit);
    cl->addLayout(hl);
    cl->addWidget(controlStack);
    setControls(cw);

    setProperty("isManager", true);
    setAcceptDrops(true);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QFont bigandbold;
    bigandbold.setPointSize(bigandbold.pointSize() + 2);
    bigandbold.setWeight(QFont::Bold);

    QHBoxLayout *titleBar = new QHBoxLayout;
#if 0
    title = new QLabel(windowtitle, this);
    title->setFont(bigandbold);
    title->setStyleSheet("QLabel {"
                           "background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
                           "stop: 0 #FFFFFF, stop: 0.4 #DDDDDD,"
                           "stop: 0.5 #D8D8D8, stop: 1.0 #CCCCCC);"
                           "color: #535353;"
                           "font-weight: bold; }");

    QPalette mypalette;
    mypalette.setColor(title->foregroundRole(), Qt::white);
    title->setPalette(mypalette);
#endif

#if 0
    static CocoaInitializer cocoaInitializer; // we only need one
    styleSelector = new QtMacSegmentedButton (3, this);
    styleSelector->setTitle(0, "Tab");
    styleSelector->setTitle(1, "Scroll");
    styleSelector->setTitle(2, "Tile");
#else
    styleSelector = new QComboBox(this);
    styleSelector->addItem("Tab");
    styleSelector->addItem("Scroll");
    styleSelector->addItem("Tile");
    QFont small;
    small.setPointSize(8);
    styleSelector->setFont(small);
    styleSelector->setFixedHeight(20);

#endif

    style = new QStackedWidget(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);
    titleBar->addStretch();
    titleBar->addWidget(styleSelector);
    layout->addLayout(titleBar);
    layout->addWidget(style);

    QPalette palette;
    //palette.setBrush(backgroundRole(), QBrush(QImage(":/images/carbon.jpg")));
    palette.setBrush(backgroundRole(), QColor("#A8A8A8"));

    // each style has its own container widget
    QWidget *tabArea = new QWidget(this);
    tabArea->setContentsMargins(20,0,20,20);
    tabArea->setAutoFillBackground(false);
    //tabArea->setFrameStyle(QFrame::NoFrame);
    QVBoxLayout *tabLayout = new QVBoxLayout(tabArea);
    tabLayout->setContentsMargins(0,0,0,0);
    tabLayout->setSpacing(0);
    tabbed = new QTabWidget(this);
    tabbed->setContentsMargins(0,0,0,0);
    tabbed->setTabsClosable(false);
    tabbed->setPalette(palette);
    tabbed->setDocumentMode(false);
    tabbed->setMovable(true);

    QTabBar *tb = tabbed->findChild<QTabBar*>(QLatin1String("qt_tabwidget_tabbar"));
    tb->setShape(QTabBar::RoundedSouth);
    tb->setDrawBase(false);
    tabbed->setStyleSheet("QTabWidget::tab-bar { alignment: center; }"
                          "QTabWidget::pane { top: 20px; }"
#if 0
                          "QTabWidget::tab-bar::tab:selected { background: #A8A8A8; font: bold; border: 1px; border-color: black; }"
                          "QTabWidget::tab-bar::tab:!selected { background: #A8A8A8;font: normal; }" 
#endif
                         );

    // styling commented out whilst work out the differences
    // between mac and linux and windows
#if 0
    tabbed->setStyleSheet("QTabBar::tab {"
                           "background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
                           "stop: 0 #CFCFCF, stop: 1.0 #A8A8A8);"
                           "border: 1px solid rgba(255, 255, 255, 32);"
                           "color: #535353;}"
                           "QTabBar::tab:selected {"
                           "min-height: 30px;"
                           "background: rgba(255,255,255,255); }");
#endif

    tabLayout->addWidget(tabbed);
    if (appsettings->value(this, GC_DROPSHADOW, true).toBool()) {
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(20);
        shadow->setXOffset(10);
        shadow->setYOffset(10);
        tabbed->setGraphicsEffect(shadow);
    }
    style->addWidget(tabArea);

    // tiled
    tileWidget = new QWidget(this);
    tileWidget->setPalette(palette);
    tileWidget->setContentsMargins(0,0,0,0);
    //tileWidget->setMouseTracking(true);
    //tileWidget->installEventFilter(this);

    tileGrid = new QGridLayout(tileWidget);
    tileGrid->setSpacing(0);

    tileArea = new QScrollArea(this);
    tileArea->setWidgetResizable(true);
    tileArea->setWidget(tileWidget);
    tileArea->setFrameStyle(QFrame::NoFrame);
    tileArea->setContentsMargins(0,0,0,0);
    style->addWidget(tileArea);

    winWidget = new QWidget(this);
    winWidget->setContentsMargins(0,0,0,0);
    winWidget->setPalette(palette);

    winFlow = new GcWindowLayout(winWidget, 0, 20, 20);
    winFlow->setContentsMargins(20,20,20,20);

    winArea = new QScrollArea(this);
    winArea->setWidgetResizable(true);
    winArea->setWidget(winWidget);
    winArea->setFrameStyle(QFrame::NoFrame);
    winArea->setContentsMargins(0,0,0,0);
    style->addWidget(winArea);
    winWidget->installEventFilter(this); // to draw cursor
    winWidget->setMouseTracking(true); // to draw cursor

    currentStyle=2;
#if 0
    styleSelector->setSelected(2);
#else
    styleSelector->setCurrentIndex(2);
#endif
    style->setCurrentIndex(2); // tile area

    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(mainWindow, SIGNAL(configChanged()), this, SLOT(configChanged()));
    connect(tabbed, SIGNAL(currentChanged(int)), this, SLOT(tabSelected(int)));
    connect(tabbed, SIGNAL(tabCloseRequested(int)), this, SLOT(removeChart(int)));
    connect(tb, SIGNAL(tabMoved(int,int)), this, SLOT(tabMoved(int,int)));
#if 0
    connect(styleSelector, SIGNAL(clicked(int,bool)), SLOT(styleChanged(int)));
#else
    connect(styleSelector, SIGNAL(currentIndexChanged(int)), SLOT(styleChanged(int)));
#endif
    connect(titleEdit, SIGNAL(textChanged(const QString&)), SLOT(titleChanged()));

    // watch drop operations
    //setMouseTracking(true);
    installEventFilter(this);
}

HomeWindow::~HomeWindow()
{
#if 0
disconnect(this);
qDebug()<<"removing from layouts!";
    // remove from our layouts -- they'reowned by mainwindow
    for (int i=0; i<charts.count(); i++) {
        // remove from old style
        switch (currentStyle) {
        case 0 : // they are tabs in a TabWidget
            {
            int tabnum = tabbed->indexOf(charts[i]);
            tabbed->removeTab(tabnum);
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
    }
#endif
}

void
HomeWindow::configChanged()
{
    // get config
}

void
HomeWindow::selected()
{
    if (loaded == false) {
        restoreState();
        loaded = true;
    }

    resizeEvent(NULL); // force a relayout
    rideSelected();
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
            tabbed->setTabText(controlStack->currentIndex(), titleEdit->text());
        }

        // repaint to reflect
        charts[controlStack->currentIndex()]->repaint();
    }
}

void
HomeWindow::rideSelected()
{
    if (amVisible()) {
        for (int i=0; i < charts.count(); i++) {

            // show if its not a tab
            if (currentStyle) charts[i]->show(); // keep tabs hidden, show the rest

            // if we are tabbed AND its the current tab then mimic tabselected
            // to force the tabwidget to refresh AND set its ride property (see below)
            //otherwise just go ahead and notify it of a new ride
            if (!currentStyle && charts.count() && i==tabbed->currentIndex())
                tabSelected(tabbed->currentIndex());
            else
                charts[i]->setProperty("ride", property("ride"));
        }
    }
}

void
HomeWindow::tabSelected(int index)
{
    if (active || currentStyle != 0) return;

    if (index >= 0) {
        charts[index]->show();
        charts[index]->setProperty("ride", property("ride"));
        controlStack->setCurrentIndex(index);
        titleEdit->setText(charts[index]->property("title").toString());
    }
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

    // move the windows from there current
    // position to there new position
    for (int i=0; i<charts.count(); i++) {
        // remove from old style
        switch (currentStyle) {
        case 0 : // they are tabs in a TabWidget
            {
            int tabnum = tabbed->indexOf(charts[i]);
            tabbed->removeTab(tabnum);
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
            tabbed->addTab(charts[i], charts[i]->property("title").toString());
            charts[i]->setContentsMargins(0,25,0,0);
            charts[i]->setResizable(false); // we need to show on tab selection!
            charts[i]->hide(); // we need to show on tab selection!
            break;
        case 1 : // they are lists in a GridLayout
            tileGrid->addWidget(charts[i], i,0);
            charts[i]->setContentsMargins(0,25,0,0);
            charts[i]->setResizable(false); // we need to show on tab selection!
            charts[i]->show();
            charts[i]->setProperty("ride", property("ride"));
            break;
        case 2 : // thet are in a FlowLayout
            winFlow->addWidget(charts[i]);
            charts[i]->setContentsMargins(0,15,0,0);
            charts[i]->setResizable(true); // we need to show on tab selection!
            charts[i]->show();
            charts[i]->setProperty("ride", property("ride"));
        default:
            break;
        }

    }
    currentStyle = id;
    style->setCurrentIndex(currentStyle);

    active = false;

    if (currentStyle == 0 && charts.count()) tabSelected(0);
    resizeEvent(NULL); // XXX watch out in case resize event uses this!!

    // now refresh as we are done
    setUpdatesEnabled(true);
    update();
}

void
HomeWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->formats().contains("application/x-qabstractitemmodeldatalist")) {
        event->accept();
        dropPending = true;
    }
}

void
HomeWindow::dropEvent(QDropEvent *event)
{
    QStandardItemModel model;
    model.dropMimeData(event->mimeData(), Qt::CopyAction, -1,-1, QModelIndex());
    QString chart = model.data(model.index(0,0), Qt::DisplayRole).toString();

    dropPending = false;

    // which one am i?
    for (int i = 0; GcWindows[i].id != GcWindowTypes::None; i++) {
        if (GcWindows[i].name == chart) {

            event->accept();

            GcWindow *newone = GcWindowRegistry::newGcWindow(GcWindows[i].id, mainWindow);
            newone->setProperty("title", GcWindows[i].name);
            newone->setProperty("widthFactor", (double)2.0);
            newone->setProperty("heightFactor", (double)2.0);
            newone->setProperty("GcWinID", GcWindows[i].id);
            if (currentStyle == 2) newone->setResizable(true);
            addChart(newone);
            newone->show();
            newone->setProperty("ride", property("ride"));
            resizeEvent(NULL);
#if 0
            // GcWindowDialog is delete on close, so no need to delete
            GcWindowDialog *f = new GcWindowDialog(GcWindows[i].id, mainWindow);
            GcWindow *newone = f->exec();

            // returns null if cancelled or closed
            if (newone) {
                addChart(newone);
                newone->show();
            }

            // now wipe it
            delete f;
#endif

            // before we return lets turn the cursor off
            chartCursor = -2;
            winWidget->repaint();
            return;
        }
    }

    // nope not one of ours
    event->ignore();

    // turn off cursor
    chartCursor = -2;
    winWidget->repaint();

    return;
}

void
HomeWindow::addChart(GcWindow* newone)
{
    int chartnum = charts.count();

    active = true;

    if (newone) {
        //GcWindow *newone = GcWindowRegistry::newGcWindow(GcWindows[i].id, mainWindow);

        // add the controls
        QWidget *x = dynamic_cast<GcWindow*>(newone)->controls();
        QWidget *c = (x != NULL) ? x : new QWidget(this);

        if (currentStyle == 2 && chartCursor >= 0)
            controlStack->insertWidget(chartCursor, c);
        else
            controlStack->addWidget(c);

        // watch for enter events!
        newone->installEventFilter(this);

        RideItem *notconst = (RideItem*)mainWindow->currentRideItem();
        newone->setProperty("ride", QVariant::fromValue<RideItem*>(notconst));

        // add to tabs
        switch (currentStyle) {

        case 0 :
            newone->setContentsMargins(0,25,0,0);
            newone->setResizable(false); // we need to show on tab selection!
            tabbed->addTab(newone, newone->property("title").toString());
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
                               - 40 /* left and right marings */
                               - ((widthFactor-1) * 20) /* internal spacing */
                               ) / widthFactor;

                double newheight = (winArea->height()
                               - 40 /* top and bottom marings */
                               - ((heightFactor-1) * 20) /* internal spacing */
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

    active = false;
}

bool
HomeWindow::removeChart(int num)
{
    if (num >= charts.count()) return false; // out of bounds (!)

    // better let the user confirm since this
    // is undoable etc - code swiped from delete
    // ride in MainWindow, seems to work ok ;)
    QMessageBox msgBox;
    msgBox.setText(tr("Are you sure you want to remove the chart?"));
    QPushButton *deleteButton = msgBox.addButton(tr("Remove"),QMessageBox::YesRole);
    msgBox.setStandardButtons(QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.exec();
    if(msgBox.clickedButton() != deleteButton) return false;

    charts[num]->hide();

    // just in case its currently selected
    if (clicked == charts[num]) clicked=NULL;

    // remove the controls
    QWidget *m = controlStack->widget(num);
    if (m) controlStack->removeWidget(m);

    switch(currentStyle) {
        case 0 : // delete tab and widget
            tabbed->removeTab(num);
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
    delete (GcWindow*)(charts[num]);
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
                           - 40 /* left and right marings */
                           - ((widthFactor-1) * 20) /* internal spacing */
                           ) / widthFactor;

            double newheight = (winArea->height()
                           - 40 /* top and bottom marings */
                           - ((heightFactor-1) * 20) /* internal spacing */
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

        int x = ((QMouseEvent*)(e))->pos().x();
        int y = ((QMouseEvent*)(e))->pos().y();

        if (y<=15) {

            // on the title bar! -- which one?
            for(int i=0; i<charts.count(); i++) {
                if (charts[i] == object) {

                    // close button?
                    if (x > (charts[i]->width()-15)) {
                        return removeChart(i);

                    } else {

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
                    return false;
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
        //winFlow->doLayout(winFlow->geometry(), false);
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

#if 0
    // code for bumping tiles movement
    // disaobled for now in preference for
    // a drag operation with a cursor that
    // highlights where the window will be
    // moved to...
    winArea->ensureVisible(pos.x(), pos.y(), 20, 20);

    // since the widget has moved, the mouse is no longer
    // in the same position relevant to the widget, so
    // move the mouse back to the same position relevant
    // to the widget
    QCursor::setPos(winWidget->mapToGlobal(pos));

    // Move the other windows out the way...
    for(int i=0; i<charts.count(); i++) {
        if (charts[i] != w && !charts[i]->gripped() && charts[i]->geometry().intersects(w->geometry())) {
            // shift whilst we move then
            QRect inter = charts[i]->geometry().intersected(w->geometry());
            // move left
            if (charts[i]->geometry().x() < w->geometry().x()) {
                int newx = inter.x() - 20 - charts[i]->geometry().width();
                if (charts[i]->geometry().x() > newx)
                    charts[i]->move(newx, charts[i]->geometry().y());
            } else {
                int newx = inter.x()+inter.width() + 20;
                if (charts[i]->geometry().x() < newx)
                charts[i]->move(newx, charts[i]->geometry().y());
            }
            windowMoving(charts[i]); // move the rest...
        }
    }

    // now if any are off the screen try and get them
    // back on the screen without disturbing the other
    // charts (unless they are off the screen too)
    for(int i=0; i<charts.count(); i++) {

        if (!charts[i]->gripped() && (charts[i]->x() < 20 || (charts[i]->x()+charts[i]->width()) > winArea->geometry().width())) {

            // to the left...
            if (charts[i]->geometry().x() < 20) {

                QRect back(20, charts[i]->geometry().y(),
                           charts[i]->geometry().width(),
                           charts[i]->geometry().height());

                // loop through the other charts and see
                // how far it can sneak back on...
                for(int j=0; j<charts.count(); j++) {
                    if (charts[j] != charts[i]) {
                        if (back.intersects(charts[j]->geometry())) {
                            QRect inter = back.intersected(charts[j]->geometry());
                            int newx = inter.x() - back.width() - 20;
                            if (back.x() > newx) {
                                back.setX(newx);
                                back.setWidth(charts[i]->width());
                            }
                        }
                    }
                }
#if 0
                // any space?
                if (back.x() < 20) {
                    // we're still off screen so
                    // lets shift all the windows to the
                    // right if we can
                    QList<GcWindow*> obstacles;
                    QRect fback(20, charts[i]->geometry().y(),
                            charts[i]->geometry().width(),
                            charts[i]->geometry().height());


                    // order obstacles left to right
                    for(int j=0; j<charts.count(); j++) {
                        if (charts[j] != charts[i] &&
                            fback.intersects(charts[j]->geometry()))
                        obstacles.append(charts[j]);
                        qSort(obstacles);
                    }

                    // where to stop? all or is the
                    // gripped window in there
                    // or stop if offscreen already
                    int stop=0;
                    while (stop < obstacles.count()) {
                        if (obstacles[stop]->gripped() ||
                            (obstacles[stop]->geometry().x() + obstacles[stop]->width()) > winArea->width())
                            break;
                        stop++;
                    }

                    // at this point we know that members
                    // 0 - stop are possibly in our way
                    // we need to move the leftmost, if it
                    // fits we stop, if not we move the next one
                    // and the leftmost, then the next one
                    // until we have exhausted all candidates

                    // try again!
                    // loop through the other charts and see
                    // how far it can sneak back on...
                    back = QRect(20, charts[i]->geometry().y(),
                           charts[i]->geometry().width(),
                           charts[i]->geometry().height());
                    for(int j=0; j<charts.count(); j++) {
                        if (charts[j] != charts[i]) {
                            if (back.intersects(charts[j]->geometry())) {
                                QRect inter = back.intersected(charts[j]->geometry());
                                back.setX(inter.x()-back.width()-20);
                            }
                        }
                    }
                }
#endif

                charts[i]->move(back.x(), back.y());

            } else {

                QRect back(winArea->width() - charts[i]->width() - 20,
                           charts[i]->geometry().y(),
                           charts[i]->geometry().width(),
                           charts[i]->geometry().height());

                // loop through the other charts and see
                // how far it can sneak back on...
                for(int j=0; j<charts.count(); j++) {
                    if (charts[j] != charts[i]) {
                        if (back.intersects(charts[j]->geometry())) {
                            QRect inter = back.intersected(charts[j]->geometry());
                            int newx = inter.x() + inter.width() + 20;
                            if (back.x() < newx) back.setX(newx);
                        }
                    }
                }

                // any space?
                charts[i]->move(back.x(), back.y());
            }
        }
    }
#endif
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

GcWindowDialog::GcWindowDialog(GcWinID type, MainWindow *mainWindow) : mainWindow(mainWindow), type(type)
{
    //setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    setWindowTitle("Chart Setup");
    setFixedHeight(500);
    setFixedWidth(800);
    setWindowModality(Qt::ApplicationModal);

    mainLayout = new QVBoxLayout(this);
    layout = new QHBoxLayout();
    mainLayout->addLayout(layout);
    chartLayout = new QVBoxLayout;
    layout->addLayout(chartLayout);

    title = new QLineEdit(this);
    chartLayout->addWidget(title);

    win = GcWindowRegistry::newGcWindow(type, mainWindow);
    chartLayout->addWidget(win);

    controlLayout = new QFormLayout;
    height=new QDoubleSpinBox(this);
    height->setRange(1,10);
    height->setSingleStep(1);
    height->setValue(2);
    width=new QDoubleSpinBox(this);
    width->setRange(1,10);
    width->setSingleStep(1);
    width->setValue(2);

    controlLayout->addRow(new QLabel("Height Factor",this), height);
    controlLayout->addRow(new QLabel("Width Factor",this), width);
    if (win->controls()) controlLayout->addRow(win->controls());
    layout->addLayout(controlLayout);

    RideItem *notconst = (RideItem*)mainWindow->currentRideItem();
    win->setProperty("ride", QVariant::fromValue<RideItem*>(notconst));

    layout->setStretch(0, 100);
    layout->setStretch(1, 50);

    QHBoxLayout *buttons = new QHBoxLayout;
    mainLayout->addLayout(buttons);

    buttons->addStretch();
    buttons->addWidget((cancel=new QPushButton("Cancel", this)));
    buttons->addWidget((ok=new QPushButton("OK", this)));

    connect(ok, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));

    hide();
}

void GcWindowDialog::okClicked()
{
    // give back to mainWindow so we can re-use
    // note that in reject they are not and will
    // get deleted (this has been verfied with
    // some debug statements in ~GcWindow).
    //chartLayout->removeWidget(win); // remove from layout!
    //win->setParent(mainWindow); // already is!
    //if (win->controls()) win->controls()->setParent(mainWindow);

    // set its title property and geometry factors
    win->setProperty("title", title->text());
    win->setProperty("widthFactor", width->value());
    win->setProperty("heightFactor", height->value());
    win->setProperty("GcWinID", type);
    accept();
}

void GcWindowDialog::cancelClicked() { reject(); }

GcWindow *
GcWindowDialog::exec()
{
    if (QDialog::exec()) {
        return win;
    } else return NULL;
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
    // XXX other special characters will not work
    // cross-platform but will work locally, so not a biggie
    // i.e. if thedefault charts.xml has a special character
    // in it it should be added here
    return s;
}

void
HomeWindow::saveState()
{
    // run through each chart and save its USER properties
    // we do not save all the other Qt properties since
    // we're not interested in them
    // XXX currently we support QString, int, double and bool types - beware custom types!!
    if (charts.count() == 0) return; // don't save empty, use default instead

    QString filename = mainWindow->home.absolutePath() + "/" + name + "-layout.xml";
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
HomeWindow::restoreState()
{
    // restore window state
    QString filename = mainWindow->home.absolutePath() + "/" + name + "-layout.xml";
    QFileInfo finfo(filename);

    // use a default if not there
    if (!finfo.exists()) filename = QString(":xml/%1-layout.xml").arg(name);

    // now go read...
    QFile file(filename);

    // setup the handler
    QXmlInputSource source(&file);
    QXmlSimpleReader xmlReader;
    ViewParser handler(mainWindow);
    xmlReader.setContentHandler(&handler);
    xmlReader.setErrorHandler(&handler);

    // parse and instantiate the charts
    xmlReader.parse(source);

    // layout the results
    styleSelector->setCurrentIndex(handler.style);
    foreach(GcWindow *chart, handler.charts) addChart(chart);
}

//
// view layout parser - reads in athletehome/xxx-layout.xml
//
bool ViewParser::startDocument()
{
    return TRUE;
}

bool ViewParser::endElement( const QString&, const QString&, const QString &qName )
{
    if (qName == "chart") { // add to the list
        charts.append(chart);
    }
    return TRUE;
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
        chart = GcWindowRegistry::newGcWindow(type, mainWindow);
        chart->hide();
        chart->setProperty("title", QVariant(title));
        chart->setProperty("instanceName", QVariant(name));

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
        if (type == "QString") chart->setProperty(name.toLatin1(), QVariant(QString(value)));
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
    return TRUE;
}

bool ViewParser::characters( const QString&)
{
    return TRUE;
}


bool ViewParser::endDocument()
{
    return TRUE;
}
