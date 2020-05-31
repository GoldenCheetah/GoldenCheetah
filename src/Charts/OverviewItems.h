/*
 * Copyright (c) 2020 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_OverviewItem_h
#define _GC_OverviewItem_h 1

// basics
#include "ChartSpace.h"
#include <QGraphicsItem>

// qt charts for zone chart
#include <QtCharts>
#include <QBarSet>
#include <QBarSeries>
#include <QLineSeries>

// subwidgets for viz inside each overview item
class RPErating;
class BPointF;
class Sparkline;
class BubbleViz;
class Routeline;

// sparklines number of points - look back 6 weeks
#define SPARKDAYS 42

// number of points in a route viz - trial and error gets 250 being reasonable
#define ROUTEPOINTS 250

// types we use
enum OverviewItemType { RPE, METRIC, META, ZONE, INTERVAL, PMC, ROUTE };

class RPEOverviewItem : public ChartSpaceItem
{
    Q_OBJECT

    public:

        RPEOverviewItem(ChartSpace *parent, QString name);

        void itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
        void itemGeometryChanged();
        void setData(RideItem *item);

        // RPE meta field
        Sparkline *sparkline;
        RPErating *rperating;

        // for setting sparkline & painting
        bool up, showrange;
        QString value, upper, lower, mean;
};

class MetricOverviewItem : public ChartSpaceItem
{
    Q_OBJECT

    public:

        MetricOverviewItem(ChartSpace *parent, QString name, QString symbol);

        void itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
        void itemGeometryChanged();
        void setData(RideItem *item);

        QString symbol;
        RideMetric *metric;
        QString units;

        bool up, showrange;
        QString value, upper, lower, mean;

        Sparkline *sparkline;
};

class MetaOverviewItem : public ChartSpaceItem
{
    Q_OBJECT

    public:

        MetaOverviewItem(ChartSpace *parent, QString name, QString symbol);

        void itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
        void itemGeometryChanged();
        void setData(RideItem *item);

        QString symbol;
        int fieldtype;

        // for numeric metadata items
        bool up, showrange;
        QString value, upper, lower, mean;

        Sparkline *sparkline;
};

class PMCOverviewItem : public ChartSpaceItem
{
    Q_OBJECT

    public:

        PMCOverviewItem(ChartSpace *parent, QString symbol);

        void itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
        void itemGeometryChanged();
        void setData(RideItem *item);

        QString symbol;

        double sts, lts, sb, rr, stress;
};

class ZoneOverviewItem : public ChartSpaceItem
{
    Q_OBJECT

    public:

        ZoneOverviewItem(ChartSpace *parent, QString name, RideFile::seriestype);

        void itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
        void itemGeometryChanged();
        void setData(RideItem *item);
        void dragChanged(bool x);

        RideFile::seriestype series;

        QChart *chart;
        QBarSet *barset;
        QBarSeries *barseries;
        QStringList categories;
        QBarCategoryAxis *barcategoryaxis;
};

class RouteOverviewItem : public ChartSpaceItem
{
    Q_OBJECT

    public:

        RouteOverviewItem(ChartSpace *parent, QString name);

        void itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
        void itemGeometryChanged();
        void setData(RideItem *item);

        Routeline *routeline;
};

class IntervalOverviewItem : public ChartSpaceItem
{
    Q_OBJECT

    public:

        IntervalOverviewItem(ChartSpace *parent, QString name, QString xs, QString ys, QString zs);

        void itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
        void itemGeometryChanged();
        void setData(RideItem *item);

        QString xsymbol, ysymbol, zsymbol;
        BubbleViz *bubble;
};

//
// below are theviz widgets used by the overview items
//

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
        BubbleViz(IntervalOverviewItem *parent, QString name=""); // create and say how many days

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
        IntervalOverviewItem *parent;
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
        RPErating(RPEOverviewItem *parent, QString name=""); // create and say how many days

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
        RPEOverviewItem *parent;
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

#endif // _GC_OverviewItem_h
