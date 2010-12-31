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

HomeWindow::HomeWindow(MainWindow *mainWindow) :
    GcWindow(mainWindow), mainWindow(mainWindow), active(false), clicked(NULL)
{
    setInstanceName("Home Window");
    setControls(new QStackedWidget(this));
    setAcceptDrops(true);

    QVBoxLayout *layout = new QVBoxLayout(this);

#if 0
    QPalette mypalette;
    mypalette.setBrush(this->backgroundRole(), QBrush(QImage(":/images/dark.jpg")));
    setPalette(mypalette);
#endif

    QFont bigandbold;
    bigandbold.setPointSize(bigandbold.pointSize() + 2);
    bigandbold.setWeight(QFont::Bold);

    QHBoxLayout *titleBar = new QHBoxLayout;
    title = new QLabel("Home", this);
    title->setFont(bigandbold);
    titleBar->addWidget(title);
    titleBar->addStretch();

#ifdef Q_OS_MAC
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

    titleBar->addWidget(styleSelector);
    layout->addLayout(titleBar);

    style = new QStackedWidget(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(style);

    // each style has its own container widget
    tabbed = new QTabWidget(this);
    tabbed->setContentsMargins(0,0,0,0);
    tabbed->setTabsClosable(true);
    style->addWidget(tabbed);

    // tiled
    tileWidget = new QWidget(this);
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
    QPalette palette;
    palette.setBrush(winWidget->backgroundRole(), QBrush(QImage(":/images/carbon.jpg")));
    winWidget->setPalette(palette);
    //tileWidget->setMouseTracking(true);
    //tileWidget->installEventFilter(this);

    winFlow = new GcWindowLayout(winWidget, 0, 20, 20);
    winFlow->setContentsMargins(20,20,20,20);

    winArea = new QScrollArea(this);
    winArea->setWidgetResizable(true);
    winArea->setWidget(winWidget);
    winArea->setFrameStyle(QFrame::NoFrame);
    winArea->setContentsMargins(0,0,0,0);
    style->addWidget(winArea);

    currentStyle=2;
#ifdef Q_OS_MAC
    styleSelector->setEnabled (2, true);
#else
    styleSelector->setCurrentIndex(2);
#endif
    style->setCurrentIndex(2); // tile area

    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(mainWindow, SIGNAL(configChanged()), this, SLOT(configChanged()));
    connect(tabbed, SIGNAL(currentChanged(int)), this, SLOT(tabSelected(int)));
    connect(tabbed, SIGNAL(tabCloseRequested(int)), this, SLOT(removeChart(int)));
#ifdef Q_OS_MAC
    connect(styleSelector, SIGNAL(clicked(int,bool)), SLOT(styleChanged(int)));
#else
    connect(styleSelector, SIGNAL(currentIndexChanged(int)), SLOT(styleChanged(int)));
#endif

    restoreState();
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
    resizeEvent(NULL); // force a relayout
    rideSelected();
}

void
HomeWindow::rideSelected()
{
    if (amVisible()) {
        foreach(GcWindow *x, charts) {
            if (currentStyle) x->show(); // keep tabs hidden, show the rest
            x->setProperty("ride", property("ride"));
        }

        // show current tab
        if (!currentStyle && charts.count()) tabSelected(tabbed->currentIndex());
    }
}

void
HomeWindow::tabSelected(int index)
{
    if (active || currentStyle != 0) return;
    if (index >= 0) {
        charts[index]->show();
        charts[index]->setProperty("ride", property("ride"));
        dynamic_cast<QStackedWidget*>(controls())->setCurrentIndex(index);
    }
}

void
HomeWindow::styleChanged(int id)
{
    // ignore if out of bounds or we're already using that style
    if (id > 2 || id < 0 || id == currentStyle) return;

    active = true;

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
            charts[i]->setContentsMargins(0,0,0,0);
            charts[i]->hide(); // we need to show on tab selection!
            break;
        case 1 : // they are lists in a GridLayout
            tileGrid->addWidget(charts[i], i,0);
            charts[i]->setContentsMargins(0,15,0,0);
            charts[i]->show();
            break;
        case 2 : // thet are in a FlowLayout
            winFlow->addWidget(charts[i]);
            charts[i]->setContentsMargins(0,15,0,0);
            charts[i]->show();
        default:
            break;
        }

    }
    currentStyle = id;
    style->setCurrentIndex(currentStyle);

    active = false;

    if (currentStyle == 0 && charts.count()) tabSelected(0);
    resizeEvent(NULL); // XXX watch out in case resize event uses this!!
    update();
}

void
HomeWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->formats().contains("application/x-qabstractitemmodeldatalist"))
        event->accept();
    else
        event->accept();
}

void
HomeWindow::dropEvent(QDropEvent *event)
{
    QStandardItemModel model;
    model.dropMimeData(event->mimeData(), Qt::CopyAction, 0,0, QModelIndex());
    QString chart = model.data(model.index(0,0), Qt::DisplayRole).toString();

    // which one am i?
    for (int i = 0; GcWindows[i].id != GcWindowTypes::None; i++) {
        if (GcWindows[i].name == chart) {

            event->accept();

            // GcWindowDialog is delete on close, so no need to delete
            GcWindowDialog *f = new GcWindowDialog(GcWindows[i].id, mainWindow);
            GcWindow *newone = f->exec();

            // returns null if cancelled or closed
            if (newone) {
                addChart(newone);
                newone->show();
            }
            return;
        }
    }

    // nope not one of ours
    event->ignore();
    return;
}

void
HomeWindow::addChart(GcWindow* newone)
{
    int chartnum = charts.count();
    if (newone) {
        //GcWindow *newone = GcWindowRegistry::newGcWindow(GcWindows[i].id, mainWindow);

        // add the controls
        QStackedWidget *m = dynamic_cast<QStackedWidget*>(controls());

        QWidget *x = dynamic_cast<GcWindow*>(newone)->controls();
        QWidget *c = (x != NULL) ? x : new QWidget(this);
        m->addWidget(c);

        // watch for enter events!
        newone->installEventFilter(this);

        RideItem *notconst = (RideItem*)mainWindow->currentRideItem();
        newone->setProperty("ride", QVariant::fromValue<RideItem*>(notconst));

        // add to tabs
        switch (currentStyle) {

        case 0 :
            newone->setContentsMargins(0,0,0,0);
            tabbed->addTab(newone, newone->property("title").toString());
            break;
        case 1 :
            {
            // add to tiles
            // set geometry
            newone->setFixedWidth((tileArea->width()-50));
            newone->setFixedHeight(newone->width() * 0.7);
            int row = chartnum; // / 2;
            int column = 0; //chartnum % 2;
            newone->setContentsMargins(0,15,0,0);
            tileGrid->addWidget(newone, row, column);
            }
            break;
        case 2 :
            {
                int heightFactor = newone->property("heightFactor").toInt();
                int widthFactor = newone->property("widthFactor").toInt();

                // width of area minus content margins and spacing around each item
                // divided by the number of items

#ifdef Q_OS_MAC
                // Mac QT fucks about with spacing limits, and doesn't honor
                // spacing < 6 or thereabouts anyway

                int newwidth = (winArea->width() - 20 -
                              (2*(winArea->contentsMargins().left()+winArea->contentsMargins().right()))
                              - ((2*widthFactor) * 6) ) / widthFactor;
#else
                int newwidth = (winArea->width() - 20 /* scrollbar */
                               - 40 /* left and right marings */
                               - ((widthFactor-1) * 20) /* internal spacing */
                               ) / widthFactor;
#endif

                int newheight = (winArea->height() -
                              (winArea->contentsMargins().left()+winArea->contentsMargins().right())
                              - ((1+heightFactor) * 5) ) / heightFactor;

                int minWidth = 10;
                int minHeight = 10;
                if (newwidth < minWidth) newwidth = minWidth;
                newone->setFixedWidth(newwidth);
                if (newheight < minHeight) newheight = minHeight;
                else newone->setFixedHeight(newheight);
                newone->setContentsMargins(0,15,0,0);
                winFlow->addWidget(newone);
            }
            break;
        }
        // add to the list
        charts.append(newone);
        newone->hide();
    }
}

void
HomeWindow::removeChart(int num)
{
    if (num >= charts.count()) return; // out of bounds (!)

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
    if(msgBox.clickedButton() != deleteButton) return;

    charts[num]->hide();

    // just in case its currently selected
    if (clicked == charts[num]) clicked=NULL;

    // remove the controls
    QWidget *m = dynamic_cast<QStackedWidget*>(controls())->widget(num);
    if (m) dynamic_cast<QStackedWidget*>(controls())->removeWidget(m);

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
}

void
HomeWindow::resizeEvent(QResizeEvent *)
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
            int heightFactor = x->property("heightFactor").toInt();
            int widthFactor = x->property("widthFactor").toInt();


#ifdef Q_OS_MAC
                // Mac QT fucks about with spacing limits, and doesn't honor
                // spacing < 6 or thereabouts anyway

            int newwidth = (winArea->width() - 20 -
                          (2*(winArea->contentsMargins().left()+winArea->contentsMargins().right()))
                          - ((2*widthFactor) * 6) ) / widthFactor;
#else
                int newwidth = (winArea->width() - 20 /* scrollbar */
                               - 40 /* left and right marings */
                               - ((widthFactor-1) * 20) /* internal spacing */
                               ) / widthFactor;
#endif
            int newheight = (winArea->height() -
                          (winArea->contentsMargins().left()+winArea->contentsMargins().right())
                          - ((1+heightFactor) * 5) ) / heightFactor;

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
    // is it a mouse enter event?
    if (e->type() == QEvent::Enter) {

        // ok which one?
        for(int i=0; i<charts.count(); i++) {
            if (charts[i] == object) {

                // if we havent clicked on a chart lets set the controls
                // on a mouse over
                if (!clicked) dynamic_cast<QStackedWidget*>(controls())->setCurrentIndex(i);

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
                        removeChart(i);

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
                            dynamic_cast<QStackedWidget*>(controls())->setCurrentIndex(i);

                        } else { // already clicked so unclick it (toggle)
                            charts[i]->setProperty("active", false);
                            charts[i]->repaint();
                            clicked = NULL;
                        }
                    }
                    return true;
                }
            }
        }
        return false;
    }
    return false;
}

GcWindowDialog::GcWindowDialog(GcWinID type, MainWindow *mainWindow) : mainWindow(mainWindow), type(type)
{
    setAttribute(Qt::WA_DeleteOnClose);
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
    height=new QSpinBox(this);
    height->setRange(1,10);
    height->setSingleStep(1);
    height->setValue(2);
    width=new QSpinBox(this);
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
    win->setParent(mainWindow);
    if (win->controls()) win->controls()->setParent(mainWindow);

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

    QString filename = mainWindow->home.absolutePath() + "/" + "home" + "-layout.xml";
    QFile file(filename);
    file.open(QFile::WriteOnly);
    file.resize(0);
    QTextStream out(&file);

    out<<"<layout name=\"home\">\n";

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
               if (QString(p.typeName()) == "LTMSettings") qDebug()<<"not supported yet...";
               else {
               out<<"\t\t<property name=\""<<xmlprotect(p.name())<<"\" "
                  <<"type=\""<<p.typeName()<<"\" "
                  <<"value=\"";

                if (QString(p.typeName()) == "int") out<<p.read(chart).toInt();
                if (QString(p.typeName()) == "double") out<<p.read(chart).toDouble();
                if (QString(p.typeName()) == "QString") out<<xmlprotect(p.read(chart).toString());
                if (QString(p.typeName()) == "bool") out<<p.read(chart).toBool();

                out<<"\" />\n";
                }
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
    QString filename = mainWindow->home.absolutePath() + "/" + "home" + "-layout.xml";
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
        // XXX do nothing - need to remember style (tab/scroll/tile) ...
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
        if (type == "LTMSettings") qDebug()<<"not supported yet...";

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
