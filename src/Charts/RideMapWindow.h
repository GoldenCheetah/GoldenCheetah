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

#ifndef _GC_RideMapWindow_h
#define _GC_RideMapWindow_h
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

#include <QWebEnginePage>
#include <QWebEngineView>

class QMouseEvent;
class RideItem;
class Context;
class QColor;
class QVBoxLayout;
class QTabWidget;
class RideMapWindow;
class IntervalSummaryWindow;
class SmallPlot;


struct PositionItem {
    PositionItem(double lat, double lng): lat(lat), lng(lng) {}

    double lat, lng;
};


// trick the maps api into ignoring gestures by
// pretending to be chrome. see: http://developer.qt.nokia.com/forums/viewthread/1643/P15
class mapWebPage : public QWebEnginePage
{
};

class MapWebBridge : public QObject
{
    Q_OBJECT

    private:
        Context *context;
        RideMapWindow *mw;

        RideFilePoint const * point = nullptr;
        bool m_startDrag = false;
        bool m_drag = false;
        int selection = 1;

        RideFilePoint const *searchPoint(double lat, double lng) const;

    public:
        MapWebBridge(Context *context, RideMapWindow *mw) : context(context), mw(mw) {}

    public slots:
        Q_INVOKABLE void call(int count);

        // drawing basic route, and interval polylines
        Q_INVOKABLE int intervalCount();
        Q_INVOKABLE QVariantList getLatLons(int i); // get array of latitudes for highlighted n

        // once map and basic route is loaded
        // this slot is called to draw additional
        // overlays e.g. shaded route, markers
        Q_INVOKABLE void drawOverlays();

        // display/toggle interval on map
        Q_INVOKABLE void toggleInterval(int);
        Q_INVOKABLE void hoverInterval(int);
        Q_INVOKABLE void clearHover();
        Q_INVOKABLE void hoverPath(double lat, double lng);
        Q_INVOKABLE void clickPath(double lat, double lng);
        Q_INVOKABLE void mouseup();

        // Comparemode
        Q_INVOKABLE QVariantList setCompareIntervals();

        void intervalsChanged() { emit drawIntervals(); }
        void compareIntervalsChanged() { emit drawCompareIntervals(); }

    signals:
        void drawIntervals();
        void drawCompareIntervals();
};

class RideMapWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    // properties can be saved/restored/set by the layout manager

    Q_PROPERTY(int maptype READ mapType WRITE setMapType USER true)
    Q_PROPERTY(bool showmarkers READ showMarkers WRITE setShowMarkers USER true)
    Q_PROPERTY(bool showfullplot READ showFullPlot WRITE setFullPlot USER true)
    Q_PROPERTY(bool showintervals READ showIntervals WRITE setShowIntervals USER true)
    Q_PROPERTY(bool hideShadedZones READ hideShadedZones WRITE setHideShadedZones USER true)
    Q_PROPERTY(bool hideYellowLine READ hideYellowLine WRITE setHideYellowLine USER true)
    Q_PROPERTY(bool hideRouteLineOpacity READ hideRouteLineOpacity WRITE setRouteLineOpacity USER true)
    Q_PROPERTY(int osmts READ osmTS WRITE setOsmTS USER true)
    Q_PROPERTY(QString googleKey READ googleKey WRITE setGoogleKey USER true)

    public:
        typedef enum {
            OSM,
            GOOGLE,
        } MapType;

        RideMapWindow(Context *, int mapType);
        virtual ~RideMapWindow();

        QWebEngineView *browser() { return view; }

        // set/get properties
        int mapType() const { return mapCombo->currentIndex(); }
        void setMapType(int x) { mapCombo->setCurrentIndex(x >= 0 && x < mapCombo->count() ? x : OSM); } // default to OSM for invalid mapType, s.t. deprecated Bing

        bool showIntervals() const { return showInt->isChecked(); }
        void setShowIntervals(bool x) { showInt->setChecked(x); }

        bool hideShadedZones() const { return hideShadedZonesCk->isChecked(); }
        void setHideShadedZones(bool x) { hideShadedZonesCk->setChecked(x); }

        bool hideYellowLine() const { return hideYellowLineCk->isChecked(); }
        void setHideYellowLine(bool x) { hideYellowLineCk->setChecked(x); }

        bool hideRouteLineOpacity() const { return hideRouteLineOpacityCk->isChecked(); }
        void setRouteLineOpacity(bool x) { hideRouteLineOpacityCk->setChecked(x); }

        bool showMarkers() const { return ( showMarkersCk->checkState() == Qt::Checked); }
        void setShowMarkers(bool x) { if (x) showMarkersCk->setCheckState(Qt::Checked); else showMarkersCk->setCheckState(Qt::Unchecked) ;}

        bool showFullPlot() const { return ( showFullPlotCk->checkState() == Qt::Checked); }
        void setFullPlot(bool x) { if (x) showFullPlotCk->setCheckState(Qt::Checked); else showFullPlotCk->setCheckState(Qt::Unchecked) ;}

        int osmTS() const { return ( tileCombo->itemData(tileCombo->currentIndex()).toInt()); }
        void setOsmTS(int x) {
            tileCombo->setCurrentIndex(tileCombo->findData(x));
            setTileServerUrlForTileType(x);
        }

        QString googleKey() const { return gkey->text(); }
        void setGoogleKey(QString x) { gkey->setText(x); }


    public slots:
        void mapTypeSelected(int x);
        void tileTypeSelected(int x);
        void showMarkersChanged(int value);
        void showFullPlotChanged(int value);
        void hideShadedZonesChanged(int value);
        void hideYellowLineChanged(int value);
        void hideRouteLineOpacityChanged(int value);
        void showIntervalsChanged(int value);
        void osmCustomTSURLEditingFinished();


        void forceReplot();
        void rideSelected();
        void createMarkers();
        void drawShadedRoute();
        void zoomInterval(IntervalItem*);
        void configChanged(qint32);

        void drawTempInterval(IntervalItem *current);
        void clearTempInterval();

        void showPosition(double mins);
        void hidePosition();

        void compareIntervalsStateChanged(bool state);
        void compareIntervalsChanged();

    private:

        bool first;

        QComboBox *mapCombo, *tileCombo;
        QCheckBox *showMarkersCk, *showFullPlotCk, *showInt;
        QCheckBox* hideShadedZonesCk, * hideYellowLineCk, * hideRouteLineOpacityCk;
        QLabel *osmTSTitle, *osmTSLabel, *osmTSUrlLabel;
        QLineEdit *osmTSUrl;

        QLineEdit *gkey;
        QLabel *gkeylabel;

        Context *context;
        QVBoxLayout *layout;

        QWebEngineView *view;
        MapWebBridge *webBridge;

        RideMapWindow();  // default ctor
        int range;
        int rideCP; // rider's CP
        QString currentPage;
        RideItem *current;
        bool firstShow;
        IntervalSummaryWindow *overlayIntervals;

        QList<PositionItem> positionItems;

        QString osmTileServerUrlDefault;

        QColor GetColor(int watts);
        void createHtml();
        void buildPositionList();

        bool getCompareBoundingBox(double &minLat, double &maxLat, double &minLon, double &maxLon) const;

        int countCompareIntervals = 0;

    private slots:
        void loadRide();
        void updateFrame();

    protected:
        SmallPlot *smallPlot;
        bool event(QEvent *event);
        bool stale;

        void setCustomTSWidgetVisible(bool value);
        void setTileServerUrlForTileType(int x);
};

#endif
