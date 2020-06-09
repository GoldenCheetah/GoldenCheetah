/*
 * Copyright (c) 2016 Damien Grauser (Damien.Grauser@gmail.com)
 * updated (c) 2020 Peter Kanatselis (pkanatselis@gmail.com)
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

#ifndef _GC_LiveMapWebPageWindow_h
#define _GC_LiveMapWebPageWindow_h
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
#include <QWebEnginePage>
#include <QWebEngineView>


class QMouseEvent;
class RideItem;
class Context;
class QColor;
class QVBoxLayout;
class QTabWidget;
class LLiveMapWebPageWindow;
class IntervalSummaryWindow;
class SmallPlot;

class LiveMapsimpleWebPage : public QWebEnginePage
{
};

class LiveMapWebPageWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    public:
        LiveMapWebPageWindow(Context *);
        ~LiveMapWebPageWindow();

        bool markerIsVisible;
        double plotLon = 0;
        double plotLat = 0;
        QString currentPage;
        //QVariantList latlons;
        QString routeLatLngs;
        void createHtml();

    public slots:

        void configChanged(qint32);

    private:
        Context *context;
        QVBoxLayout *layout;

        QWebEngineView *view;

        LiveMapWebPageWindow();  // default ctor

    private slots:
        void telemetryUpdate(RealtimeData rtd);
        void start();
        void stop();
        void pause();

    protected:

};

#endif
