/*
 * Copyright (c) 2017 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_OverviewWindow_h
#define _GC_OverviewWindow_h 1

// basics
#include "GoldenCheetah.h"
#include "Settings.h"
#include "Units.h"
#include "Colors.h"
#include "Context.h"
#include "Athlete.h"
#include "RideItem.h"
#include "RideMetric.h"
#include "HrZones.h"

// QGraphics
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsWidget>
#include <QGraphicsDropShadowEffect>

// qt
#include <QtGui>
#include <QScrollBar>
#include <QIcon>

// qt charts
#include <QtCharts>
#include <QBarSet>
#include <QBarSeries>

// geometry basics
#define SPACING 80
#define ROWHEIGHT 80

class OverviewWindow;

// keep it simple for now
class Card : public QGraphicsWidget
{
    Q_OBJECT

    public:

        Card(int deep, QString name) : QGraphicsWidget(NULL), name(name),
                                                column(0), order(0), deep(deep), onscene(false),
                                                placing(false), drag(false), invisible(false) {

            setAutoFillBackground(false);
            setFlags(flags() | QGraphicsItem::ItemClipsToShape); // don't paint outside the card

            brush = QBrush(GColor(CCARDBACKGROUND));
            setZValue(10);

            // a sensible default?
            type = NONE;
            metric = NULL;
            chart = NULL;

            // watch geom changes
            connect(this, SIGNAL(geometryChanged()), SLOT(geometryChanged()));
        }


        void setData(RideItem *item);

        // what type am I?
        enum cardType { NONE, METRIC, META, SERIES, ZONE } type;
        typedef enum cardType CardType;
        QString value, units;
        RideMetric *metric;

        // settings
        struct {

            QString symbol;
            RideFile::SeriesType series;

        } settings;

        // my parent
        OverviewWindow *parent;

        // what to do if clicked XXX just a hack for now
        void clicked();

        // name
        QString name;

        // qt chart
        QChart *chart;

        // bar chart
        QBarSet *barset;
        QBarSeries *barseries;
        QStringList categories;
        QBarCategoryAxis *barcategoryaxis;

        // which column, sequence and size in rows
        int column, order, deep;
        bool onscene, placing, drag;
        bool invisible;

        void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);

        QBrush brush;

    public slots:

        void geometryChanged();
};

class OverviewWindow : public GcChartWindow
{
    Q_OBJECT

    Q_PROPERTY(QRectF viewRect READ getViewRect WRITE setViewRect)
    Q_PROPERTY(int viewY READ getViewY WRITE setViewY)

    public:

        OverviewWindow(Context *context);

        // are we just looking or changing
        enum { VIEW, CONFIG } mode;

        // current state for event processing
        enum { NONE, DRAG, XRESIZE, YRESIZE } state;

        // used by children
        Context *context;

        // to get paint device
        QGraphicsView *device() { return view; }

    public slots:

        // ride item changed
        void rideSelected();

        // for smooth scrolling
        void setViewY(int x) { if (_viewY != x) {_viewY =x; updateView();} }
        int getViewY() const { return _viewY; }

        // for smooth scaling
        void setViewRect(QRectF);
        QRectF getViewRect() const { return viewRect; }

        // trap signals
        void configChanged(qint32);

        // scale on first show
        void showEvent(QShowEvent *) { updateView(); }
        void resizeEvent(QResizeEvent *) { updateView(); }

        // scrolling
        void edgeScroll(bool down);
        void scrollTo(int y);
        void scrollFinished() { scrolling = false; updateView(); }
        void scrollbarMoved(int x) { if (!scrolling && !setscrollbar) { setViewY(x); }}

        // set geometry on the widgets (size and pos)
        void updateGeometry();

        // set scale, zoom etc appropriately
        void updateView();

        // create a card - zones / series
        Card *newCard(QString name, int column, int order, int deep, Card::CardType type, RideFile::SeriesType x) {
                                                         Card *add = new Card(deep, name);
                                                         add->column = column;
                                                         add->order = order;
                                                         add->deep = deep;
                                                         add->parent = this;
                                                         add->type = type;
                                                         add->settings.series = x;
                                                         cards.append(add);
                                                         add->chart = new QChart(add);
                                                         add->chart->setBackgroundVisible(false); // draw on canvas
                                                         add->chart->legend()->setVisible(false); // no legends

                                                         // we have a big font for charts
                                                         QFont mid;
                                                         mid.setPointSize(ROWHEIGHT/2);
                                                         add->chart->setFont(mid);

                                                         if (type == Card::ZONE) {
                                                            add->barset = new QBarSet(tr("Time In Zone"), this);
                                                            add->barset->setLabelFont(mid);
                                                            if (add->settings.series == RideFile::hr) {
                                                                add->barset->setLabelColor(GColor(CHEARTRATE));
                                                                add->barset->setBorderColor(GColor(CHEARTRATE));
                                                                add->barset->setBrush(GColor(CHEARTRATE));
                                                            } else if (add->settings.series == RideFile::watts) {
                                                                add->barset->setLabelColor(GColor(CPOWER));
                                                                add->barset->setBorderColor(GColor(CPOWER));
                                                                add->barset->setBrush(GColor(CPOWER));
                                                            } else if (add->settings.series == RideFile::wbal) {
                                                                add->barset->setLabelColor(GColor(CWBAL));
                                                                add->barset->setBorderColor(GColor(CWBAL));
                                                                add->barset->setBrush(GColor(CWBAL));
                                                            } else if (add->settings.series == RideFile::kph) {
                                                                add->barset->setLabelColor(GColor(CSPEED));
                                                                add->barset->setBorderColor(GColor(CSPEED));
                                                                add->barset->setBrush(GColor(CSPEED));
                                                            }
                                                            // how many?
                                                            if (context->athlete->hrZones(false)) {
                                                                // set the zero values
                                                                for(int i=0; i<context->athlete->hrZones(false)->getScheme().nzones_default; i++) {
                                                                    *add->barset << 0;
                                                                    add->categories << context->athlete->hrZones(false)->getScheme().zone_default_name[i];
                                                                }
                                                            }
                                                            add->barseries = new QBarSeries(this);
                                                            add->barseries->setLabelsPosition(QAbstractBarSeries::LabelsOutsideEnd);
                                                            add->barseries->setLabelsVisible(true);
                                                            add->barseries->setLabelsFormat("@value %");
                                                            add->barseries->append(add->barset);
                                                            add->chart->addSeries(add->barseries);
                                                            add->chart->setTitle(""); // none wanted
                                                            add->chart->setAnimationOptions(QChart::SeriesAnimations);
                                                            add->barcategoryaxis = new QBarCategoryAxis(this);
                                                            add->barcategoryaxis->setLabelsFont(mid);
                                                            add->barcategoryaxis->setLabelsColor(QColor(100,100,100));
                                                            add->barcategoryaxis->setGridLineVisible(false);
                                                            add->barcategoryaxis->setCategories(add->categories);
                                                            add->chart->createDefaultAxes();
                                                            add->chart->setAxisX(add->barcategoryaxis, add->barseries);
                                                            add->chart->axisY(add->barseries)->setGridLineVisible(false);
                                                            QPen axisPen(GColor(CCARDBACKGROUND));
                                                            axisPen.setWidth(0.5); // almost invisibke
                                                            add->barcategoryaxis->setLinePen(axisPen);
                                                            add->chart->axisY(add->barseries)->setLinePen(axisPen);
                                                            add->chart->axisY(add->barseries)->setLabelsVisible(false);
                                                            add->chart->axisY(add->barseries)->setRange(0,100);
                                                         }
                                                         return add;
                                                        }

        // create a card - metric
        Card *newCard(QString name, int column, int order, int deep, Card::CardType type=Card::NONE, QString symbol="") {
                                                         Card *add = new Card(deep, name);
                                                         add->column = column;
                                                         add->order = order;
                                                         add->deep = deep;
                                                         add->parent = this;
                                                         add->type = type;
                                                         add->settings.symbol = symbol;
                                                         cards.append(add);
                                                         return add;
                                                        }
    protected:

        // process events
        bool eventFilter(QObject *, QEvent *event);

    private:

        // gui setup
        QGraphicsScene *scene;
        QGraphicsView *view;
        QScrollBar *scrollbar;

        // for animating transitions
        QParallelAnimationGroup *group;
        QPropertyAnimation *viewchange;
        QPropertyAnimation *scroller;

        // scene and view
        int _viewY;
        QRectF sceneRect;
        QRectF viewRect;

        // content
        QVector<int> columns;       // column widths
        QList<Card*> cards;         // tiles

        // state data
        bool yresizecursor;          // is the cursor set to resize?
        bool xresizecursor;          // is the cursor set to resize?
        bool block;                  // block event processing
        bool scrolling;              // scrolling the view?
        bool setscrollbar;           // distinguish between user and program actions
        double lasty;                // to see if the mouse is moving up or down

        union OverviewState {
            struct {
                double offx, offy; // mouse grab position on card
                Card *card;        // index of card in QList
                int width;         // how big was I when I started dragging?
            } drag;

            struct {
                double posy;
                int deep;
                Card *card;
            } yresize;

            struct {
                double posx;
                int width;
                int column;
            } xresize;

        } stateData;

};

#endif // _GC_OverviewWindow_h
