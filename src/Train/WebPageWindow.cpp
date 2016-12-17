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

#ifdef NOWEBKIT
#include <QtWebChannel>
#endif

// overlay helper
#include "TabView.h"
#include "GcOverlayWidget.h"
#include "IntervalSummaryWindow.h"
#include <QDebug>

WebPageWindow::WebPageWindow(Context *context) : GcChartWindow(context), context(context), firstShow(true)
{

    //
    // Chart settings
    //

    QWidget *settingsWidget = new QWidget(this);
    settingsWidget->setContentsMargins(0,0,0,0);
    //HelpWhatsThis *helpSettings = new HelpWhatsThis(settingsWidget);
    //settingsWidget->setWhatsThis(helpSettings->getWhatsThisText(HelpWhatsThis::ChartRides_Critical_MM_Config_Settings));


    QFormLayout *commonLayout = new QFormLayout(settingsWidget);



    customUrlLabel = new QLabel(tr("URL"));
    customUrl = new QLineEdit("");
    customUrl->setFixedWidth(250);
    customUrl->setText(QString("http://www.youtube.com"));

    commonLayout->addRow(customUrlLabel, customUrl);

    connect(customUrl, SIGNAL(editingFinished()), this, SLOT(customURLEditingFinished()));
    connect(customUrl, SIGNAL(textChanged(QString)), this, SLOT(customURLTextChanged(QString)));

    setControls(settingsWidget);

    setContentsMargins(0,0,0,0);
    layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(2,0,2,2);
    setChartLayout(layout);

#ifdef NOWEBKIT
    view = new QWebEngineView(this);
#else
    view = new QWebView();
#endif
    view->setPage(new simpleWebPage());
    view->setContentsMargins(0,0,0,0);
    view->page()->view()->setContentsMargins(0,0,0,0);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setAcceptDrops(false);
    layout->addWidget(view);

    HelpWhatsThis *help = new HelpWhatsThis(view);
    view->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ChartRides_Map));

    first = true;
    configChanged(CONFIG_APPEARANCE);

#ifdef NOWEBKIT
    view->page()->setHtml(currentPage);
#else

    QString urlstr = "";

        urlstr = QString("https://www.strava.com/oauth/authorize?");
        urlstr.append("client_id=").append(GC_STRAVA_CLIENT_ID).append("&");
        urlstr.append("scope=view_private,write&");
        urlstr.append("redirect_uri=http://www.goldencheetah.org/&");
        urlstr.append("response_type=code&");
        urlstr.append("approval_prompt=force");

    QUrl url = QUrl(urlstr);
    view->setUrl(url);

    // connects
    connect(view, SIGNAL(urlChanged(const QUrl&)), this,
            SLOT(urlChanged(const QUrl&)));
    connect(view, SIGNAL(loadFinished(bool)), this,
            SLOT(loadFinished(bool)));

    QNetworkAccessManager*nam=view->page()->networkAccessManager();

    connect(nam,SIGNAL(finished(QNetworkReply*)),this,
            SLOT(finished(QNetworkReply*)));

#endif
}

WebPageWindow::~WebPageWindow()
{
}

void
WebPageWindow::customURLTextChanged(QString text)
{
    if (currentUrl != text) {
        if (first) {
            forceReplot();
        }
    }
}

void
WebPageWindow::customURLEditingFinished()
{
    if (currentUrl != customUrl->text()) {
        currentUrl = customUrl->text();
        forceReplot();
    }
}

void 
WebPageWindow::configChanged(qint32)
{
    setProperty("color", GColor(CPLOTBACKGROUND));
#ifndef Q_OS_MAC
    overlayIntervals->setStyleSheet(TabView::ourStyleSheet());
#endif
}

void
WebPageWindow::forceReplot()
{   
#ifdef NOWEBKIT
    view->page()->setHtml(currentPage);
#else
    view->page()->mainFrame()->load(QUrl(currentUrl));
#endif


}

void WebPageWindow::updateFrame()
{
}

void
WebPageWindow::urlChanged(const QUrl &url)
{
    qDebug() << "urlChanged" << url;
}

void
WebPageWindow::loadFinished(bool ok) {
    qDebug() << "loadFinished" << ok;
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

void
WebPageWindow::onError(QNetworkReply::NetworkError err)
{
    if(err !=QNetworkReply::NoError) {
       QMessageBox::warning(0, QString("Error!"),
                           QString("Error loading webpage: \n") + QString::number(err),
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
