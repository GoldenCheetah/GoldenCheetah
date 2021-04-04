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

// google map
#include "RideMapWindow.h"

// QGraphics
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsWidget>
#include <QGraphicsDropShadowEffect>

// qt
#include <QtGui>
#include <QScrollBar>
#include <QIcon>
#include <QTimer>

// qt charts
#include <QtCharts>
#include <QBarSet>
#include <QBarSeries>
#include <QLineSeries>

// geometry basics
#define SPACING 80
#define ROWHEIGHT 80

// sparklines number of points - look back 6 weeks
#define SPARKDAYS 42

// number of points in a route viz - trial and error gets 250 being reasonable
#define ROUTEPOINTS 250

class OverviewWindow;
class Sparkline;
class Routeline;
class RPErating;
class BubbleViz;

// keep it simple for now
class Card : public QGraphicsWidget
{
    Q_OBJECT

    public:

        // what type am I?
        enum cardType { NONE, RPE, ROUTE, METRIC, META, SERIES, PMC, MODEL, INTERVAL, ZONE } type;
        typedef enum cardType CardType;

        Card(int deep, QString name) : QGraphicsWidget(NULL), name(name),
                                                column(0), order(0), deep(deep), onscene(false),
                                                placing(false), drag(false), invisible(false), fieldtype(-1) {

            setAutoFillBackground(false);
            setFlags(flags() | QGraphicsItem::ItemClipsToShape); // don't paint outside the card
            setAcceptHoverEvents(true);

            setZValue(10);

            // a sensible default?
            type = NONE;
            metric = NULL;
            chart = NULL;
            sparkline = NULL;
            rperating = NULL;

            // mem leak
            delcounter=0;

            // watch geom changes
            connect(this, SIGNAL(geometryChanged()), SLOT(geometryChanged()));
        }

        // watch mouse enter/leave
        bool sceneEvent(QEvent *event);
        bool inCorner();
        bool underMouse();

        // keep track of reuse of xyseries and delete to
        // try and minimise memory leak in Qt Chart
        int delcounter;

        // configuration
        void setType(CardType type); // NONE and Route
        void setType(CardType type, RideFile::SeriesType series);   // time in zone, data
        void setType(CardType type, QString symbol);                // metric meta
        void setType(CardType type, QString xsymbol, QString ysymbol, QString zsymbol); // interval

        // setup data after ride selected
        void setData(RideItem *item);

        QString value, units;
        RideMetric *metric;

        // when dragging around
        void setDrag(bool x);

        // settings
        struct {

            QString symbol, xsymbol, ysymbol, zsymbol;
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

        // ZONE bar chart
        QBarSet *barset;
        QBarSeries *barseries;
        QStringList categories;
        QBarCategoryAxis *barcategoryaxis;

        // RPE meta field
        RPErating *rperating;

        // METRIC sparkline
        Sparkline *sparkline;

        // ROUTE visualisation
        Routeline *routeline;

        // PMC stress values
        double lts, sts, stress, sb, rr;

        // INTERVAL bubble chart
        BubbleViz *bubble;

        QString upper, lower, mean;
        bool up;
        bool showrange;

        // which column, sequence and size in rows
        int column, order, deep;
        bool onscene, placing, drag;
        bool invisible;
        int fieldtype;

        void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);

        QColor ridecolor;
        bool incorner;

    public slots:

        void geometryChanged();
};

// for now the basics are x and y and a radius z, color fill
class BPointF {
public:

    BPointF() : x(0), y(0), z(0), fill(GColor(Qt::gray)) {}

    double score(BPointF &other);

    double x,y,z;
    QColor fill;
    QString label;
};


// bubble chart, very very basic just a visualisation
class BubbleViz : public QObject, public QGraphicsItem
{
    // need to be a qobject for metaproperties
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

    // want a meta property for property animation
    Q_PROPERTY(int transition READ getTransition WRITE setTransition)

    public:
        BubbleViz(Card *parent, QString name=""); // create and say how many days

        // we monkey around with this *A LOT*
        void setGeometry(double x, double y, double width, double height);
        QRectF geometry() { return geom; }

        // transition animation 0-255
        int getTransition() const {return transition;}
        void setTransition(int x) { if (transition !=x) {transition=x; update();}}

        // null members for now just get hooked up
        void setPoints(QList<BPointF>points);

        void setRange(double minx, double maxx, double miny, double maxy) {
            oldminx = this->minx;
            oldminy = this->miny;
            oldmaxx = this->maxx;
            oldmaxy = this->maxy;
            this->minx=minx;
            this->maxx=maxx;
            this->miny=miny;
            this->maxy=maxy;
        }
        void setAxisNames(QString xlabel, QString ylabel) { this->xlabel=xlabel; this->ylabel=ylabel; update(); }

        // needed as pure virtual in QGraphicsItem
        QVariant itemChange(GraphicsItemChange change, const QVariant &value);
        void paint(QPainter*, const QStyleOptionGraphicsItem *, QWidget*);
        QRectF boundingRect() const { return QRectF(parent->geometry().x() + geom.x(),
                                                    parent->geometry().y() + geom.y(),
                                                    geom.width(), geom.height());
        }

        // watch hovering
        bool sceneEvent(QEvent *event);

    private:
        Card *parent;
        QRectF geom;
        QString name;

        // where is the cursor?
        bool hover;
        QPointF plotpos;

        // for animated transition
        QList <BPointF> oldpoints; // for animation
        int transition;
        double oldmean;
        double oldminx,oldmaxx,oldminy,oldmaxy;
        QPropertyAnimation *animator;

        // chart settings
        QList <BPointF> points;
        double minx,maxx,miny,maxy;
        QString xlabel, ylabel;
        double mean, max;
};

// RPE rating viz and widget to set value
class RPErating : public QGraphicsItem
{

    public:
        RPErating(Card *parent, QString name=""); // create and say how many days

        // we monkey around with this *A LOT*
        void setGeometry(double x, double y, double width, double height);
        QRectF geometry() { return geom; }

        void setValue(QString);

        // needed as pure virtual in QGraphicsItem
        QVariant itemChange(GraphicsItemChange change, const QVariant &value);
        void paint(QPainter*, const QStyleOptionGraphicsItem *, QWidget*);
        QRectF boundingRect() const { return QRectF(parent->geometry().x() + geom.x(),
                                                    parent->geometry().y() + geom.y(),
                                                    geom.width(), geom.height());
        }

        // for interaction
        bool sceneEvent(QEvent *event);
        void cancelEdit();
        void applyEdit();

    private:
        Card *parent;
        QString name;
        QString description;
        QRectF geom;
        QString value, oldvalue;
        QColor color;

        // interaction
        bool hover;

};

// tufte style sparkline to plot metric history
class Sparkline : public QGraphicsItem
{
    public:
        Sparkline(QGraphicsWidget *parent, int count,QString name=""); // create and say how many days

        // we monkey around with this *A LOT*
        void setGeometry(double x, double y, double width, double height);
        QRectF geometry() { return geom; }

        void setPoints(QList<QPointF>);
        void setRange(double min, double max); // upper lower

        // needed as pure virtual in QGraphicsItem
        QVariant itemChange(GraphicsItemChange change, const QVariant &value);
        void paint(QPainter*, const QStyleOptionGraphicsItem *, QWidget*);
        QRectF boundingRect() const { return QRectF(parent->geometry().x() + geom.x(),
                                                    parent->geometry().y() + geom.y(),
                                                    geom.width(), geom.height());
        }

    private:
        QGraphicsWidget *parent;
        QRectF geom;
        QString name;
        double min, max;
        QList<QPointF> points;
};

// visualisation of a GPS route as a shape
class Routeline : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

    Q_PROPERTY(int transition READ getTransition WRITE setTransition)

    public:
        Routeline(QGraphicsWidget *parent, QString name=""); // create and say how many days

        // transition animation 0-255
        int getTransition() const {return transition;}
        void setTransition(int x) { if (transition !=x) {transition=x; update();}}

        // we monkey around with this *A LOT*
        void setGeometry(double x, double y, double width, double height);
        QRectF geometry() { return geom; }

        void setData(RideItem *item);

        // needed as pure virtual in QGraphicsItem
        QVariant itemChange(GraphicsItemChange change, const QVariant &value);
        void paint(QPainter*, const QStyleOptionGraphicsItem *, QWidget*);
        QRectF boundingRect() const { return QRectF(parent->geometry().x() + geom.x(),
                                                    parent->geometry().y() + geom.y(),
                                                    geom.width(), geom.height());
        }

    private:
        QGraphicsWidget *parent;
        QRectF geom;
        QString name;
        QPainterPath path, oldpath;
        double width, height; // size of painterpath, so we scale to fit on paint

        // animating
        int transition;
        QPropertyAnimation *animator;
        double owidth, oheight; // size of painterpath, so we scale to fit on paint
};

class OverviewWindow : public GcChartWindow
{
    Q_OBJECT

    Q_PROPERTY(QString config READ getConfiguration WRITE setConfiguration USER true)
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
        QGraphicsView *view;
        QFont titlefont, bigfont, midfont, smallfont;

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

        // get/set config
        QString getConfiguration() const;
        void setConfiguration(QString x);

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

        // create a route card
        Card *newCard(QString name, int column, int order, int deep, Card::CardType type) {
                                                         Card *add = new Card(deep, name);
                                                         add->column = column;
                                                         add->order = order;
                                                         add->deep = deep;
                                                         add->parent = this;
                                                         add->setType(type);
                                                         cards.append(add);
                                                         return add;
                                                        }

        Card *newCard(QString name, int column, int order, int deep) {
                                                         Card *add = new Card(deep, name);
                                                         add->column = column;
                                                         add->order = order;
                                                         add->deep = deep;
                                                         add->parent = this;
                                                         add->setType(Card::NONE);
                                                         cards.append(add);
                                                         return add;
                                                        }

        // create a card - zones / series
        Card *newCard(QString name, int column, int order, int deep, Card::CardType type, RideFile::SeriesType x) {
                                                         Card *add = new Card(deep, name);
                                                         add->column = column;
                                                         add->order = order;
                                                         add->deep = deep;
                                                         add->parent = this;
                                                         add->setType(type, x);
                                                         cards.append(add);
                                                         return add;
                                                        }

        // create a card - metric or PMC (metric is the input)
        Card *newCard(QString name, int column, int order, int deep, Card::CardType type, QString symbol) {
                                                         Card *add = new Card(deep, name);
                                                         add->column = column;
                                                         add->order = order;
                                                         add->deep = deep;
                                                         add->parent = this;
                                                         add->setType(type, symbol);
                                                         cards.append(add);
                                                         return add;
                                                        }
        // create a card - interval
        Card *newCard(QString name, int column, int order, int deep, Card::CardType type, QString xsymbol,
                       QString ysymbol, QString zsymbol) {
                                                         Card *add = new Card(deep, name);
                                                         add->column = column;
                                                         add->order = order;
                                                         add->deep = deep;
                                                         add->parent = this;
                                                         add->setType(type, xsymbol, ysymbol, zsymbol);
                                                         cards.append(add);
                                                         return add;
                                                        }
    protected:

        // process events
        bool eventFilter(QObject *, QEvent *event);

    private:

        // gui setup
        QGraphicsScene *scene;
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

        bool stale;
        bool configured;
        RideItem *current;
};

#endif // _GC_OverviewWindow_h
