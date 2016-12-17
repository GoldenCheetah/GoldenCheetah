/*
 * Copyright (c) 2009 Greg Lonnon (greg.lonnon@gmail.com)
 *               2011 Mark Liversedge (liversedge@gmail.com)
 *               2016 Damien Grauser (Damien.Grauser@gmail.com)
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

#ifndef _GC_WebPageWindow_h
#define _GC_WebPageWindow_h
#include "GoldenCheetah.h"

#include <QWidget>
#include <QDialog>

#include <string>
#include <iostream>
#include <sstream>
#include <string>
#include "RideFile.h"
#include "IntervalItem.h"
#include "Context.h"

#include <QDialog>
#include <QSslSocket>
#ifdef NOWEBKIT
#include <QWebEnginePage>
#include <QWebEngineView>
#else
#include <QtWebKit>
#include <QWebPage>
#include <QWebView>
#include <QWebFrame>
#endif

class QMouseEvent;
class RideItem;
class Context;
class QColor;
class QVBoxLayout;
class QTabWidget;
class WebPageWindow;
class IntervalSummaryWindow;
class SmallPlot;

// trick the maps api into ignoring gestures by
// pretending to be chrome. see: http://developer.qt.nokia.com/forums/viewthread/1643/P15
#ifdef NOWEBKIT
class mapWebPage : public QWebEnginePage
{
};
#else
class simpleWebPage : public QWebPage
{
#if 0
    virtual QString userAgentForUrl(const QUrl&) const {
        return "Mozilla/5.0";
    }
#endif
};
#endif

class WebPageWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    // properties can be saved/restored/set by the layout manager
    Q_PROPERTY(QString url READ url WRITE setUrl USER true)


    public:
        WebPageWindow(Context *);
        ~WebPageWindow();
        bool first;

        QLabel *customUrlLabel;
        QLineEdit *customUrl;

        // set/get properties
        QString url() const { return customUrl->text(); }
        void setUrl(QString x) { customUrl->setText(x) ;}


    public slots:
        void customURLEditingFinished();
        void customURLTextChanged(QString text);


        void forceReplot();
        void configChanged(qint32);

    private:
        Context *context;
        QVBoxLayout *layout;

#ifdef NOWEBKIT
        QWebEngineView *view;
#else
        QWebView *view;
#endif

        WebPageWindow();  // default ctor
        QString currentPage;
        bool firstShow;

        QString defaultUrl;
        QString currentUrl;
    private slots:
        void updateFrame();
        void urlChanged(const QUrl& url);
        void loadFinished(bool ok);
        void finished(QNetworkReply*reply);
        void onError(QNetworkReply::NetworkError err);

    protected:
        bool event(QEvent *event);
};

#endif
