/*
 * Copyright (c) 2013 Mark Liversedge (liversedge@gmail.com)
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

#include "TabView.h"
#include "Tab.h"
#include "Context.h"
#include "Athlete.h"
#include "RideItem.h"
#include "BlankState.h"
#include "Perspective.h"
#include "GcWindowRegistry.h"
#include "TrainDB.h"
#include "RideNavigator.h"
#include "MainWindow.h"
#include "IdleTimer.h"

#include "Settings.h"
#include "GcUpgrade.h"
#include "LTMWindow.h"

TabView::TabView(Context *context, int type) : 
    QWidget(context->tab), context(context), type(type),
    _sidebar(true), _tiled(false), _selected(false), lastHeight(130*dpiYFactor), sidewidth(0),
    active(false), bottomRequested(false), bottomHideOnIdle(false), perspectiveactive(false),
    stack(NULL), splitter(NULL), mainSplitter(NULL), 
    sidebar_(NULL), bottom_(NULL), page_(NULL), blank_(NULL),
    loaded(false)
{
    // setup the basic widget
    QVBoxLayout *layout = new QVBoxLayout(this);
    setContentsMargins(0,0,0,0);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);

    stack = new QStackedWidget(this);
    stack->setContentsMargins(0,0,0,0);
    stack->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
    stack->setMinimumWidth(500*dpiXFactor);
    stack->setMinimumHeight(500*dpiYFactor);

    layout->addWidget(stack);

    // the splitter
    splitter = new QSplitter(this);
    splitter->setHandleWidth(1);
    splitter->setStyleSheet(" QSplitter::handle { background-color: rgb(120,120,120); color: darkGray; }");
    splitter->setFrameStyle(QFrame::NoFrame);
    splitter->setContentsMargins(0, 0, 0, 0); // attempting to follow some UI guides
    splitter->setOpaqueResize(true); // redraw when released, snappier UI
    stack->insertWidget(0, splitter); // splitter always at index 0

    QString heading = tr("Compare Activities and Intervals");
    if (type == VIEW_HOME) heading = tr("Compare Date Ranges");
    else if (type == VIEW_TRAIN) heading = tr("Intensity Adjustments and Workout Control");

    mainSplitter = new ViewSplitter(Qt::Vertical, heading, this);
    mainSplitter->setHandleWidth(23 *dpiXFactor);
    mainSplitter->setFrameStyle(QFrame::NoFrame);
    mainSplitter->setContentsMargins(0, 0, 0, 0); // attempting to follow some UI guides
    mainSplitter->setOpaqueResize(true); // redraw when released, snappier UI

    // the animator
    anim = new QPropertyAnimation(mainSplitter, "hpos");

    connect(splitter,SIGNAL(splitterMoved(int,int)), this, SLOT(splitterMoved(int,int)));
    connect(context,SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(&IdleTimer::getInstance(), SIGNAL(userIdle()), this, SLOT(onIdle()));
    connect(&IdleTimer::getInstance(), SIGNAL(userActive()), this, SLOT(onActive()));
}

TabView::~TabView()
{
    saveState(); // writes xxx-perspectives.xml

    foreach(Perspective *p, pages_) delete p;
    pages_.clear();
}

void
TabView::setRide(RideItem*ride)
{
    if (loaded) page()->setProperty("ride", QVariant::fromValue<RideItem*>(dynamic_cast<RideItem*>(ride)));
}

void
TabView::splitterMoved(int pos,int)
{
    if (sidebar_ == NULL) return; // we haven't set sidebar yet.

    // show / hide sidebar as dragged..
    if ((pos == 0  && sidebarEnabled())) setSidebarEnabled(false);

    // remember the size the user selected
    sidewidth = splitter->sizes()[0];

    // we now have splitter settings for each view
    QString setting = QString("%1/%2").arg(GC_SETTINGS_SPLITTER_SIZES).arg(type);
    appsettings->setCValue(context->athlete->cyclist, setting, splitter->saveState());

    // if user moved us then tell ride navigator if
    // we are the analysis view
    // all a bit of a hack to stop the column widths from
    // being adjusted as the splitter gets resized and reset
    if (type == VIEW_ANALYSIS && active == false && context->rideNavigator->geometry().width() != 100)
        context->rideNavigator->setWidth(context->rideNavigator->geometry().width());
}

void
TabView::resizeEvent(QResizeEvent *)
{
    active = true; // we're mucking about, so ignore in splitterMoved ...

    if (sidewidth < 200) {
        sidewidth = splitter->sizes()[0];
    } else {

        // splitter sizes will have changed, so lets get the new
        // total width and set the handle back to where it was
        QList<int> csizes = splitter->sizes();
        if (csizes.count() == 2) {

            // total will have changed
            int tot = csizes[0] + csizes[1];
            if (tot >= sidewidth) {

                // set left back to where it was before!
                csizes[0] = sidewidth;
                csizes[1] = tot-sidewidth;
                splitter->setSizes(csizes);
                sidewidth = splitter->sizes()[0];
            }
        }
    }

    active = false;
}

void
TabView::setSidebar(QWidget *sidebar)
{
    sidebar_ = sidebar;
    splitter->insertWidget(0, sidebar);

    configChanged(CONFIG_APPEARANCE);
}

QString
TabView::ourStyleSheet()
{
    return QString::fromUtf8("QScrollBar { background-color: %1; border: 0px; }"
#ifndef Q_OS_MAC
           "QTabWidget { background: %1; }"
           "QTabWidget::pane { border: 1px solid %2; } "
           "QTextEdit { background: %1; }"
           "QTextEdit#metadata { background: %3; }"
#endif
           "QTreeView { background: %1; }"
           "QScrollBar:vertical {"
           "    border: 0px solid darkGray; "
           "    background:%1;"
           "    width: %4px;    "
           "    margin: 0px 0px 0px 0px;"
           "}"
           "QScrollBar::handle:vertical:enabled:hover {"
           "    background: lightGray; "
           "    min-height: %4px;"
           ""
           "}"
           "QScrollBar::handle:vertical:enabled {"
           "    background: darkGray; "
           "    min-height: %4px;"
           ""
           "}"
           "QScrollBar::handle:vertical {"
           "    background: %1; "
           "    min-height: %4px;"
           ""
           "}"
           "QScrollBar::sub-page:vertical { background: %1; }"
           "QScrollBar::add-page:vertical { background: %1; }"
           "QScrollBar::add-line:vertical {"
           "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
           "    stop: 0  rgb(32, 47, 130), stop: 0.5 rgb(32, 47, 130),  stop:1 rgb(32, 47, 130));"
           "    height: px;"
           "    subcontrol-position: bottom;"
           "    subcontrol-origin: margin;"
           "}"
           "QScrollBar::sub-line:vertical {"
           "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
           "    stop: 0  rgb(32, 47, 130), stop: 0.5 rgb(32, 47, 130),  stop:1 rgb(32, 47, 130));"
           "    height: 0px;"
           "    subcontrol-position: top;"
           "    subcontrol-origin: margin;"
           "}"
           "QScrollBar:horizontal {"
           "    border: 0px solid darkGray; "
           "    background-color:%1;"
           "    height: %4px;    "
           "    margin: 1px 1px 1px 1px;"
           "}"
           "QScrollBar::handle:horizontal {"
           "    background: darkGray; "
           "    min-width: 0px;"
           ""
           "}"
           "QScrollBar::sub-page:horizontal { background: %1; }"
           "QScrollBar::add-page:horizontal { background: %1; }"
           "QScrollBar::add-line:horizontal {"
           "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
           "    stop: 0  rgb(32, 47, 130), stop: 0.5 rgb(32, 47, 130),  stop:1 rgb(32, 47, 130));"
           "    width: 0px;"
           "    subcontrol-position: bottom;"
           "    subcontrol-origin: margin;"
           "}"
           "QScrollBar::sub-line:horizontal {"
           "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
           "    stop: 0  rgb(32, 47, 130), stop: 0.5 rgb(32, 47, 130),  stop:1 rgb(32, 47, 130));"
           "    width: 0px;"
           "    subcontrol-position: top;"
           "    subcontrol-origin: margin;"
           "}"
           "QTableWidget::item:hover { color: black; background: lightGray; }"
           "QTreeView::item:hover { color: black; background: lightGray; }"
           "").arg(GColor(CPLOTBACKGROUND).name())
            .arg(GColor(CPLOTGRID).name())
            .arg(GCColor::alternateColor(GColor(CPLOTBACKGROUND)).name())
            .arg(8 * dpiXFactor)
            ;
}

void
TabView::configChanged(qint32)
{
#if (defined Q_OS_LINUX) || (defined Q_OS_WIN)
    // style that sucker
    if (sidebar_) {
        sidebar_->setStyleSheet(ourStyleSheet());
    }
#endif
}

void
TabView::saveState()
{
    // if its not loaded, don't save a blank file !
    if (!loaded) return;

    // run through each perspective and save its charts

    // we do not save all the other Qt properties since
    // we're not interested in them
    // NOTE: currently we support QString, int, double and bool types - beware custom types!!

    QString view = "none";
    switch(type) {
    case VIEW_ANALYSIS: view = "analysis"; break;
    case VIEW_TRAIN: view = "train"; break;
    case VIEW_DIARY: view = "diary"; break;
    case VIEW_HOME: view = "home"; break;
    }

    QString filename = context->athlete->home->config().canonicalPath() + "/" + view + "-perspectives.xml";
    QFile file(filename);
    if (!file.open(QFile::WriteOnly)) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(tr("Problem Saving User GUI configuration"));
        msgBox.setInformativeText(tr("File: %1 cannot be opened for 'Writing'. Please check file properties.").arg(filename));
        msgBox.exec();
        return;
    };
    file.resize(0);
    QTextStream out(&file);
    out.setCodec("UTF-8");

    // is just a collection of layout (aka old HomeWindow name-layout.xml)
    out<<"<layouts>\n";

    // so lets run through the perspectives
    foreach(Perspective *page, pages_) {

        out<<"<layout name=\""<< page->title <<"\" style=\"" << page->currentStyle <<"\">\n";

        // iterate over charts
        foreach (GcChartWindow *chart, page->charts) {
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
    out<<"</layouts>\n";
    file.close();
}

void
TabView::restoreState(bool useDefault)
{
    QString view = "none";
    switch(type) {
    case VIEW_ANALYSIS: view = "analysis"; break;
    case VIEW_TRAIN: view = "train"; break;
    case VIEW_DIARY: view = "diary"; break;
    case VIEW_HOME: view = "home"; break;
    }

    // restore window state
    QString filename = context->athlete->home->config().canonicalPath() + "/" + view + "-perspectives.xml";
    QFileInfo finfo(filename);

    QString content = "";
    bool legacy = false;

    // set content from the default
    if (useDefault) {

        // for getting config
        QNetworkAccessManager nam;

        // remove the current saved version
        QFile::remove(filename);

        // fetch from the goldencheetah.org website
        QString request = QString("%1/%2-perspectives.xml")
                             .arg(VERSION_CONFIG_PREFIX)
                             .arg(view);

        QNetworkReply *reply = nam.get(QNetworkRequest(QUrl(request)));

        if (reply->error() == QNetworkReply::NoError) {

            // lets wait for a response with an event loop
            // it quits on a 5s timeout or reply coming back
            QEventLoop loop;
            QTimer timer;
            timer.setSingleShot(true);
            connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
            connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
            timer.start(5000);

            // lets block until signal received
            loop.exec(QEventLoop::WaitForMoreEvents);

            // all good?
            if (reply->error() == QNetworkReply::NoError) {
                content = reply->readAll();
            }
        }
    }

    //  if we don't have content read from file/resource
    if (content == "") {

        // if no local perspectives file fall back to old
        // layout file (pre-version 3.6) - will get a single perspective
        if (!finfo.exists()) {
            filename = context->athlete->home->config().canonicalPath() + "/" + view + "-layout.xml";
            finfo.setFile(filename);
            useDefault = false;
            legacy = true;
        }

        // drop back to what is baked in
        if (!finfo.exists()) {
            filename = QString(":xml/%1-perspectives.xml").arg(view);
            useDefault = true;
            legacy = false;
        }
        QFile file(filename);
        if (file.open(QIODevice::ReadOnly)) {
            content = file.readAll();
            file.close();
        }
    }

    // if we *still* don't have content then something went
    // badly wrong, so only reset if its not blank
    if (content != "") {

        // whilst this happens don't show user
        setUpdatesEnabled(false);

        // setup the handler
        QXmlInputSource source;
        source.setData(content);
        QXmlSimpleReader xmlReader;
        ViewParser handler(context);
        xmlReader.setContentHandler(&handler);
        xmlReader.setErrorHandler(&handler);

        // parse and instantiate the charts
        xmlReader.parse(source);
        pages_ = handler.perspectives;

    }
    if (legacy && pages_.count() == 1) pages_[0]->title = "General";

    if (pages_.count() == 0) {
        page_ = new Perspective(context, "empty", type);
        pages_ << page_;
    }

    // add to stack
    foreach(Perspective *page, pages_) {
        pstack->addWidget(page);
        cstack->addWidget(page->controls());
        page->configChanged(0); // set colors correctly- will have missed from startup
    }

    // default to first one
    page_ = pages_[0];

    // if this is analysis view then lets select the first ride now
    if (type == VIEW_ANALYSIS) {

        // used to happen in Tab.cpp on create, but we do it later now
        QDateTime now = QDateTime::currentDateTime();
        for (int i=context->athlete->rideCache->rides().count(); i>0; --i) {
            if (context->athlete->rideCache->rides()[i-1]->dateTime <= now) {
                context->athlete->selectRideFile(context->athlete->rideCache->rides()[i-1]->fileName);
                break;
            }
        }

        // otherwise just the latest
        if (context->currentRideItem() == NULL && context->athlete->rideCache->rides().count() != 0) {
            context->athlete->selectRideFile(context->athlete->rideCache->rides().last()->fileName);
        }
    }

}

void
TabView::addPerspective(QString name)
{
    Perspective *page = new Perspective(context, name, type);

    // tabbed unless on train view
    if (type == VIEW_TRAIN) page->styleChanged(2);
    else page->styleChanged(0);

    pages_ << page;
    pstack->addWidget(page);
    cstack->addWidget(page->controls());
    page->configChanged(0); // set colors correctly- will be empty...
}

void
TabView::setPages(QStackedWidget *pages)
{
    pstack = pages;

    // add to mainSplitter
    // now reset the splitter
    mainSplitter->insertWidget(-1, pages);
    mainSplitter->setStretchFactor(0,0);
    mainSplitter->setCollapsible(0, false);
    splitter->insertWidget(-1, mainSplitter);

    // restore sizes
    QString setting = QString("%1/%2").arg(GC_SETTINGS_SPLITTER_SIZES).arg(type);
    QVariant splitterSizes = appsettings->cvalue(context->athlete->cyclist, setting); 

    // new (3.1) mechanism 
    if (splitterSizes.toByteArray().size() > 1 ) {
        splitter->restoreState(splitterSizes.toByteArray());
    } else {

        // use old (v3 or earlier) mechanism
        QVariant splitterSizes = appsettings->cvalue(context->athlete->cyclist, GC_SETTINGS_SPLITTER_SIZES); 
        if (splitterSizes.toByteArray().size() > 1 ) {

            splitter->restoreState(splitterSizes.toByteArray());

        } else {

            // sensible default as never run before!
            QList<int> sizes;

            sizes.append(200);
            sizes.append(context->mainWindow->width()-200);
            splitter->setSizes(sizes);
            
        }
    }
}

void
TabView::setBottom(QWidget *widget)
{
    bottom_ = widget;
    bottom_->hide();
    mainSplitter->insertWidget(-1, bottom_);
    mainSplitter->setCollapsible(1, true); // XXX we need a ComparePane widget ...
    mainSplitter->setStretchFactor(1,1);
}

void 
TabView::dragEvent(bool x)
{
    setBottomRequested(x);
    context->mainWindow->setToolButtons(); // toolbuttons reflect show/hide status
}

// hide and show bottom - but with a little animation ...
void
TabView::setShowBottom(bool x) 
{
    if (x == isShowBottom())
    {
        return;
    }

    // remember last height used when hidind
    if (!x && bottom_) lastHeight = bottom_->height();

    // basic version for now .. remembers and sets horizontal position precisely
    // adding animation should be easy from here


    if (bottom_) {
        if (x) {

            // set to the last value....
            bottom_->show();

            anim->setDuration(lastHeight * 3);
            anim->setEasingCurve(QEasingCurve(QEasingCurve::Linear));
            anim->setKeyValueAt(0,mainSplitter->maxhpos()-22);
            anim->setKeyValueAt(1,mainSplitter->maxhpos()-(lastHeight+22));
            anim->start();

        } else {

            // need a hide animator to hide on timeout
            //anim->setDuration(200);
            //anim->setEasingCurve(QEasingCurve(QEasingCurve::Linear));
            //anim->setKeyValueAt(0,mainSplitter->maxhpos()-(lastHeight+22));
            //anim->setKeyValueAt(1,mainSplitter->maxhpos()-22);
            //anim->start();

            bottom_->hide();
        }
    }
}

void
TabView::setBlank(BlankStatePage *blank)
{
    blank_ = blank;
    blank->hide();
    stack->insertWidget(1, blank); // blank state always at index 1

    // and when stuff happens lets check
    connect(blank, SIGNAL(closeClicked()), this, SLOT(checkBlank()));
    connect(context, SIGNAL(rideAdded(RideItem*)), this, SLOT(checkBlank()));
    connect(context, SIGNAL(rideDeleted(RideItem*)), this, SLOT(checkBlank()));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(checkBlank()));
    connect(trainDB, SIGNAL(dataChanged()), this, SLOT(checkBlank()));

}


void
TabView::sidebarChanged()
{
    // wait for main window to catch up
    if (sidebar_ == NULL) return;

    // tell main window qmenu we changed
    if (context->mainWindow->init) context->mainWindow->showhideSidebar->setChecked(_sidebar);

    if (sidebarEnabled()) {

        setUpdatesEnabled(false);

        sidebar_->show();

        // Restore sizes
        QString setting = QString("%1/%2").arg(GC_SETTINGS_SPLITTER_SIZES).arg(type);
        QVariant splitterSizes = appsettings->cvalue(context->athlete->cyclist, setting);
        if (splitterSizes.toByteArray().size() > 1 ) {
            splitter->restoreState(splitterSizes.toByteArray());
            splitter->setOpaqueResize(true); // redraw when released, snappier UI
        }

        // if it was collapsed we need set to at least 200
        // unless the mainwindow isn't big enough
        if (sidebar_->width()<10) {
            int size = width() - 200 * dpiXFactor;
            if (size>(200* dpiXFactor)) size = 200* dpiXFactor;

            QList<int> sizes;
            sizes.append(size);
            sizes.append(width()-size);
            splitter->setSizes(sizes);
        }

        // if user moved us then tell ride navigator if
        // we are the analysis view
        // all a bit of a hack to stop the column widths from
        // being adjusted as the splitter gets resized and reset
        if (context->mainWindow->init && context->tab->init && type == VIEW_ANALYSIS && active == false && context->rideNavigator->geometry().width() != 100)
            context->rideNavigator->setWidth(context->rideNavigator->geometry().width());
        setUpdatesEnabled(true);

    } else sidebar_->hide();
}

void
TabView::setPerspectives(QComboBox *perspectiveSelector)
{
    perspectiveactive=true;

    // lazy load of charts- only do it when this view is selected
    if (!loaded)  restoreState(false);

    perspectiveSelector->clear();
    foreach(Perspective *page, pages_) {
        perspectiveSelector->addItem(page->title);
    }
    perspectiveSelector->addItem("Add New Perspective...");
    perspectiveSelector->addItem("Manage Perspectives...");
    perspectiveSelector->insertSeparator(pages_.count());
    perspectiveactive=false;

    // if we only just loaded the charts and views, we need to select
    // one to get the ride item and date range selected
    if (!loaded) {
        loaded = true;
        perspectiveSelected(0);
    }
}


void
TabView::perspectiveSelected(int index)
{
    if (perspectiveactive || index <0) return;

    // switch the stack to show the one selected
    // set page to the one we want
    setUpdatesEnabled(false);
    if (index < pages_.count()) {

        page_ = pages_[index];

        // switch to this perspective's charts and controls
        pstack->setCurrentIndex(index);
        cstack->setCurrentIndex(index);
        pstack->show();
        cstack->show();

        // set properties on the perspective as they propagate to charts
        RideItem *notconst = (RideItem*)context->currentRideItem();
        page_->setProperty("ride", QVariant::fromValue<RideItem*>(notconst));
        page_->setProperty("dateRange", property("dateRange"));

        // set to whatever we have currently selected
        // bit of spooky updates at a distance here... xxx fixme back to Perspective class
        for(int i = 0; i < page_->charts.count(); i++) {

            page_->charts[i]->setProperty("ride", QVariant::fromValue<RideItem*>(notconst));
            page_->charts[i]->setProperty("dateRange", property("dateRange"));
            if (page_->currentStyle != 0) page_->charts[i]->show();
        }

        setUpdatesEnabled(true);
        //if (page_->currentStyle == 0 && page_->charts.count()) page_->tabSelected(0);
    }
}


void
TabView::tileModeChanged()
{
    if (page_) page_->setStyle(isTiled() ? 2 : 0);
}

void
TabView::selectionChanged()
{
    // we got selected..
    if (isSelected()) {

        // makes sure menu now reflects our setting
        context->mainWindow->showhideSidebar->setChecked(_sidebar);

        // or do we need to show blankness?
        if (isBlank() && blank_ && page_ && blank_->canShow()) {

            splitter->hide();
            blank()->show();

            stack->setCurrentIndex(1);

        } else if (blank_ && page_) {

            blank()->hide();
            splitter->show();

            emit onSelectionChanged(); // give view a change to prepare
            page()->selected(); // select the view

            stack->setCurrentIndex(0);
        }
    } else {

        emit onSelectionChanged(); // give view a change to clear

    }
}

void
TabView::resetLayout()
{
    // delete all current perspectives
    // XXX TODO

    // reload from default (website / baked in)
    // XXX TODO
}

void
TabView::addChart(GcWinID id)
{
    if (page_) page_->appendChart(id);
}

void
TabView::checkBlank()
{
    selectionChanged(); // run through the code again
}

void
TabView::setBottomRequested(bool x)
{
    bottomRequested = x;
    setShowBottom(x);
}

void
TabView::setHideBottomOnIdle(bool x)
{
    bottomHideOnIdle = x;
}

void
TabView::onIdle()
{
    if (bottomHideOnIdle)
    {
        setShowBottom(false);
    }
}

void
TabView::onActive()
{
    if (bottomRequested)
    {
        setShowBottom(true);
    }
}

//
// view layout parser - reads in athletehome/xxx-layout.xml
//
bool ViewParser::startDocument()
{
    page = NULL;
    chart = NULL;
    return true;
}

bool ViewParser::endElement( const QString&, const QString&, const QString &qName )
{
    if (qName == "chart" && chart && page) { // add chart to homewindow
        page->addChart(chart);
    }

    if (qName == "layout" && page) {

        // one we just did needs resolving translate the charts and add to the perspective

        // are we english language?
        QVariant lang = appsettings->value(NULL, GC_LANG, QLocale::system().name());
        bool english = lang.toString().startsWith("en") ? true : false;

        // translate the metrics, but only if the built-in "default.XML"s are read (and only for LTM charts)
        // and only if the language is not English (i.e. translation is required).
#if 0 // XXX fixme
        if (useDefault && !english) {

            // translate the titles
            Perspective::translateChartTitles(charts);

            // translate the LTM settings
            for (int i=0; i<charts.count(); i++) {
                // find out if it's an LTMWindow via dynamic_cast
                LTMWindow* ltmW = dynamic_cast<LTMWindow*> (charts[i]);
                if (ltmW) {
                    // the current chart is an LTMWindow, let's translate

                    // now get the LTMMetrics
                    LTMSettings workSettings = ltmW->getSettings();
                    // replace name and unit for translated versions
                    workSettings.translateMetrics(GlobalContext::context()->useMetricUnits);
                    ltmW->applySettings(workSettings);
                }
            }
        }
#endif
        page->styleChanged(style);
    }
    return true;
}

bool ViewParser::startElement( const QString&, const QString&, const QString &name, const QXmlAttributes &attrs )
{
    if (name == "layout") {

        QString name="General";
        for(int i=0; i<attrs.count(); i++) {
            if (attrs.qName(i) == "style") {
                style = Utils::unprotect(attrs.value(i)).toInt();
            }
            if (attrs.qName(i) == "name") {
                name =  Utils::unprotect(attrs.value(i));
            }
        }

        // we need a new perspective
        page = new Perspective(context, name, VIEW_HOME); //XXX fixme VIEW_HOME!!! XXX
        perspectives.append(page);
    }
    else if (name == "chart") {

        QString name="", title="", typeStr="";
        GcWinID type;

        // get attributes
        for(int i=0; i<attrs.count(); i++) {
            if (attrs.qName(i) == "name") name = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "title") title = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "id")  typeStr = Utils::unprotect(attrs.value(i));
        }

        // new chart
        type = static_cast<GcWinID>(typeStr.toInt());
        chart = GcWindowRegistry::newGcWindow(type, context);
        if (chart != NULL) {
            chart->hide();
            chart->setProperty("title", QVariant(title));
        }
    }
    else if (name == "property") {

        QString name, type, value;

        // get attributes
        for(int i=0; i<attrs.count(); i++) {
            if (attrs.qName(i) == "name") name = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "value") value = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "type")  type = Utils::unprotect(attrs.value(i));
        }

        // set the chart property
        if (type == "int" && chart) chart->setProperty(name.toLatin1(), QVariant(value.toInt()));
        if (type == "double" && chart) chart->setProperty(name.toLatin1(), QVariant(value.toDouble()));

        // deprecate dateRange asa chart property THAT IS DSAVED IN STATE
        if (type == "QString" && name != "dateRange" && chart) chart->setProperty(name.toLatin1(), QVariant(QString(value)));
        if (type == "QDate" && chart) chart->setProperty(name.toLatin1(), QVariant(QDate::fromString(value)));
        if (type == "bool" && chart) chart->setProperty(name.toLatin1(), QVariant(value.toInt() ? true : false));
        if (type == "LTMSettings" && chart) {
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
