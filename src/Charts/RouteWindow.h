/*
 * Copyright (c) 2011, 2014 Damien Grauser (Damien.Grauser@pev-geneve.ch)
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

#ifndef ROUTEWINDOW_H
#define ROUTEWINDOW_H 1
#include "GoldenCheetah.h"
#include "Context.h"
#include "IntervalItem.h"
#include "Route.h"
#include "RouteItem.h"

#if QT_VERSION >= 0x060000
#include <QtWebEngineWidgets/QWebEngineView>
#else
#include <QtWebKit/QWebView>
#endif
#include <QTableWidget>
#include <iostream>
#include <sstream>

class MainWindow;
class RouteWindow;

class WebBridgeForRoute : public QObject
{
    Q_OBJECT;

    private:
        Context *context;
        RouteWindow *gm;

    public:
        WebBridgeForRoute(Context *context, RouteWindow *gm) : context(context), gm(gm) {}

    public slots:
        Q_INVOKABLE void call(int count);

        // drawing basic route, and interval polylines
        Q_INVOKABLE QVariantList getLatLons(int i); // get array of latitudes for highlighted n

        // once map and basic route is loaded
        // this slot is called to draw additional
        // overlays e.g. shaded route, markers
        Q_INVOKABLE void drawOverlays();

        // display/toggle interval on map
        //Q_INVOKABLE void toggleInterval(int);
        //void intervalsChanged() { emit drawIntervals(); }

    signals:
        void drawIntervals();
};

class RouteWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT


    public:
        RouteWindow(Context *context);

        RouteItem *routeItem;
        void drawShadedRoute();

    protected:
        void createHtml();
        Context *context;


    private:
        void loadRide();
        std::string CreateMapToolTipJavaScript();

        QVBoxLayout *layout;
        QWebEngineView *view;
        WebBridgeForRoute *webBridge;
        QTreeWidget *treeWidget;
        QTableWidget* rideTable;


        Routes *routes;
        RouteSegment *route;



        // the web browser is loading a page, do NOT start another load
        bool loadingPage;
        // the ride has changed, load a new page
        bool newRideToLoad;

        bool routeToSave;

        // current HTML for the ride
        QString currentPage;

    private slots:
       void loadStarted();
       //void loadFinished(bool);
       void updateFrame();

       void resetRoutes();
       void deleteRoute();
       void renameRoute();
       void routeChanged(QTreeWidgetItem *item, int column);

       void showTreeContextMenuPopup(const QPoint &);
       void rideTreeWidgetSelectionChanged();

};

#endif // ROUTEWINDOW_H
