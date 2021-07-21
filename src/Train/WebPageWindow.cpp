/*
 * Copyright (c) 2016 Damien Grauser (Damien.Grauser@gmail.com)
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

#include "WebPageWindow.h"

#include "MainWindow.h"
#include "RideItem.h"
#include "RideFile.h"
#include "RideImportWizard.h"
#include "IntervalItem.h"
#include "IntervalTreeView.h"
#include "SmallPlot.h"
#include "Context.h"
#include "Athlete.h"
#include "Zones.h"
#include "Settings.h"
#include "Colors.h"
#include "Units.h"
#include "TimeUtils.h"
#include "HelpWhatsThis.h"
#include "Library.h"
#include "ErgFile.h"

#include <QtWebChannel>
#include <QWebEngineProfile>
#include <QWebEngineDownloadItem>

// overlay helper
#include "TabView.h"
#include "GcOverlayWidget.h"
#include "IntervalSummaryWindow.h"
#include <QDebug>

#include <QWebEngineUrlRequestInterceptor>
#include <QWebEngineSettings>
#include <QWebEngineUrlSchemeHandler>
#include <QWebEngineUrlRequestJob>

// request interceptor to get downloads in QtWebEngine
class WebDownloadInterceptor : public QWebEngineUrlRequestInterceptor
{
    public:
        WebDownloadInterceptor() : QWebEngineUrlRequestInterceptor(Q_NULLPTR) {}

    public slots:
        void interceptRequest(QWebEngineUrlRequestInfo &) {
            //qDebug()<<info.requestUrl().toString();
        }
};

// custom scheme handler
class WebSchemeHandler : public QWebEngineUrlSchemeHandler
{
    public:
        WebSchemeHandler(QWebEngineView *view) : QWebEngineUrlSchemeHandler(Q_NULLPTR), view(view) {}
    public slots:
        void requestStarted(QWebEngineUrlRequestJob *request) {

            QString inject = QString("window.resolveLocalFileSystemURL = window.resolveLocalFileSystemURL || window.webkitResolveLocalFileSystemURL; "
                                     "window.resolveLocalFileSystemURL('%1', function(fs) {console.warn(fs.name);}, function(evt) {console.warn(evt.target.error.code);} );")
                             .arg(request->requestUrl().toString());
            view->page()->runJavaScript(inject);
        }

    private:
        QWebEngineView *view;
};


// declared in main, we only want to use it to get QStyle
extern QApplication *application;

WebPageWindow::WebPageWindow(Context *context) : GcChartWindow(context), context(context), firstShow(true)
{
    //
    // reveal controls widget
    //

    // layout reveal controls
    QHBoxLayout *revealLayout = new QHBoxLayout;
    revealLayout->setContentsMargins(0,0,0,0);

    rButton = new QPushButton(application->style()->standardIcon(QStyle::SP_ArrowRight), "", this);
    rCustomUrl = new QLineEdit(this);
    revealLayout->addStretch();
    revealLayout->addWidget(rButton);
    revealLayout->addWidget(rCustomUrl);
    revealLayout->addStretch();

    connect(rCustomUrl, SIGNAL(returnPressed()), this, SLOT(userUrl()));
    connect(rButton, SIGNAL(clicked(bool)), this, SLOT(userUrl()));

    setRevealLayout(revealLayout);

    //
    // Chart settings
    //

    QWidget *settingsWidget = new QWidget(this);
    settingsWidget->setContentsMargins(0,0,0,0);
    HelpWhatsThis *helpSettings = new HelpWhatsThis(settingsWidget);
    settingsWidget->setWhatsThis(helpSettings->getWhatsThisText(HelpWhatsThis::Chart_Web));


    QFormLayout *commonLayout = new QFormLayout(settingsWidget);
    customUrlLabel = new QLabel(tr("URL"));
    customUrl = new QLineEdit(this);
    customUrl->setFixedWidth(250);
    customUrl->setText("");

    commonLayout->addRow(customUrlLabel, customUrl);
    commonLayout->addRow(new QLabel("Hit return to apply URL"));

    setControls(settingsWidget);

    setContentsMargins(0,0,0,0);
    layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(2,0,2,2);
    setChartLayout(layout);

    view = new QWebEngineView(this);
    connect(view, SIGNAL(loadFinished(bool)), this, SLOT(loadFinished(bool)));

    view->page()->profile()->setHttpUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.79 Safari/537.36 Edge/14.14393");

    // add a download interceptor
    WebDownloadInterceptor *interceptor = new WebDownloadInterceptor;
    view->page()->profile()->setUrlRequestInterceptor(interceptor);

    // cookies and storage
    view->page()->profile()->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);
    QString temp = const_cast<AthleteDirectoryStructure*>(context->athlete->directoryStructure())->temp().absolutePath();
    view->page()->profile()->setCachePath(temp);
    view->page()->profile()->setPersistentStoragePath(temp);

    // web scheme handler - not needed must compile with QT 5.9 or higher
    //WebSchemeHandler *handler = new WebSchemeHandler(view);
    //view->page()->profile()->installUrlSchemeHandler("filesystem:https", handler);
    //view->page()->profile()->installUrlSchemeHandler("filesystem", handler);

    // add some settings
    view->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    view->settings()->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);

    view->setPage(new simpleWebPage());
    view->setContentsMargins(0,0,0,0);
    view->page()->view()->setContentsMargins(0,0,0,0);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setAcceptDrops(false);
    layout->addWidget(view);

    HelpWhatsThis *help = new HelpWhatsThis(view);
    view->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Chart_Web));

    // if we change in settings, force replot by pressing return
    connect(customUrl, SIGNAL(returnPressed()), this, SLOT(forceReplot()));

    first = true;
    configChanged(CONFIG_APPEARANCE);

    // intercept downloads
    connect(view->page()->profile(), SIGNAL(downloadRequested(QWebEngineDownloadItem*)), this, SLOT(downloadRequested(QWebEngineDownloadItem*)));
    connect(view->page(), SIGNAL(linkHovered(QString)), this, SLOT(linkHovered(QString)));
}

WebPageWindow::~WebPageWindow()
{
  if (view) delete view->page();
}

void 
WebPageWindow::configChanged(qint32)
{
    setProperty("color", GColor(CPLOTBACKGROUND));
    // tinted palette for headings etc
    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Base, GCColor::alternateColor(GColor(CPLOTBACKGROUND)));
    setPalette(palette);
}

void
WebPageWindow::forceReplot()
{   
    view->setZoomFactor(dpiXFactor);
    view->setUrl(QUrl(customUrl->text()));
}

void
WebPageWindow::userUrl()
{
    // add http:// if scheme is missing
    QRegExp hasscheme("^[^:]*://.*");
    QString url = rCustomUrl->text();
    if (!hasscheme.exactMatch(url)) url = "http://" + url;
    view->setZoomFactor(dpiXFactor);
    view->setUrl(QUrl(url));
}

void
WebPageWindow::loadFinished(bool ok)
{
    QString string;
    if (ok) string = view->url().toString();
    rCustomUrl->setText(string);
}

void
WebPageWindow::finished(QNetworkReply * reply)
{
    if(reply->error() !=QNetworkReply::NoError) {
        QMessageBox::warning(0, QString("NetworkAccessManager NetworkReply"),
                           QString("QNetworkReply error string: \n") + reply->errorString(),
                           QMessageBox::Ok);
    }
}

bool
WebPageWindow::event(QEvent *event)
{
    // nasty nasty nasty hack to move widgets as soon as the widget geometry
    // is set properly by the layout system, by default the width is 100 and 
    // we wait for it to be set properly then put our helper widget on the RHS
    if (event->type() == QEvent::Resize && geometry().width() != 100) {

        // put somewhere nice on first show
        if (firstShow) {
            firstShow = false;
            //helperWidget()->move(mainWidget()->geometry().width()-275, 50);
        }

        // if off the screen move on screen
        /*if (helperWidget()->geometry().x() > geometry().width()) {
            helperWidget()->move(mainWidget()->geometry().width()-275, 50);
        }*/
    }
    return QWidget::event(event);
}

void
WebPageWindow::downloadRequested(QWebEngineDownloadItem *item)
{
    // only do it if I am visible, as shared across web page instances
    if (!amVisible()) return;

    //qDebug()<<"Download Requested:"<<item->path()<<item->url().toString();

    //qDebug() << "Format: " <<  item->savePageFormat();
    //qDebug() << "Path: " << item->path();
    //qDebug() << "Type: " << item->type();
    //qDebug() << "MimeType: " << item->mimeType();

    // lets go get it!
    filenames.clear();
    filenames << QDir(item->downloadDirectory()).absoluteFilePath(item->downloadFileName());

    // set save
    connect(item, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(downloadProgress(qint64,qint64)));
    connect(item, SIGNAL(finished()), this, SLOT(downloadFinished()));

    // kick off download
    item->accept(); // lets download it!
}

void
WebPageWindow::downloadFinished()
{
    // now try and import it (if download failed file won't exist)
    // dialog is self deleting
    QStringList rides, workouts;
    foreach(QString filename, filenames) {
        if (ErgFile::isWorkout(filename)) workouts << filename;
        else rides << filename;
    }

    if (rides.count()) {
        RideImportWizard *dialog = new RideImportWizard(rides, context);
        dialog->process(); // do it!
    }
    if (workouts.count()) {
        Library::importFiles(context, filenames, true);
    }
}

void
WebPageWindow::downloadProgress(qint64 a, qint64 b)
{
    Q_UNUSED(a)
    Q_UNUSED(b)
    //qDebug()<<"downloading..." << a<< b;
}

void
WebPageWindow::linkHovered(QString)
{
    //qDebug()<<"hovering over:" << link;
}
