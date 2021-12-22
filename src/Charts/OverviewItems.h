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
#include "MetricSelect.h"
#include "DataFilter.h"
#include <QGraphicsItem>

// qt charts for zone chart
#include <QtCharts>
#include <QBarSet>
#include <QBarSeries>
#include <QLineSeries>
#include <QCursor>

// subwidgets for viz inside each overview item
class RPErating;
class BPointF;
class Sparkline;
class BubbleViz;
class Routeline;
class ProgressBar;
class VScrollBar;
class ColorButton;

// sparklines number of points - look back 6 weeks
#define SPARKDAYS 42

// number of points in a route viz - trial and error gets 250 being reasonable
#define ROUTEPOINTS 250

// types we use start from 100 to avoid clashing with main chart types
enum OverviewItemType { RPE=100, METRIC, META, ZONE, INTERVAL, PMC, ROUTE, KPI,
                        TOPN, DONUT, ACTIVITIES, ATHLETE, DATATABLE, USERCHART };

//
// Configuration widget for ALL Overview Items
//
class OverviewItemConfig : public QWidget
{
    Q_OBJECT

    public:

        OverviewItemConfig(ChartSpaceItem *);
        ~OverviewItemConfig();

        static bool registerItems();

    public slots:

        // retrieve values when user edits them (if they're valid)
        void dataChanged();

        // set the config widgets to reflect current config
        void setWidgets();

        // program editor
        void setErrors(QStringList &errors);

        // legacy data table selector (connected to legacySelector below)
        void setProgram(int n);

    protected:

        // before show, lets make sure the widgets are set correctly
        void showEvent(QShowEvent *) override { setWidgets(); }

    private:

        // the widget we are configuring
        ChartSpaceItem *item;

        // export data button (data table)
        QPushButton *exp;

        // editor for program
        QComboBox *legacySelector; // used for configuring the data table widget
        DataFilterEdit *editor;
        SearchFilterBox *filterEditor;
        QLabel *errors;

        // block updates during initialisation
        bool block;

        QLineEdit *name, *string1; // all of them
        QDoubleSpinBox *double1, *double2; // KPI
        QCheckBox *cb1; // KPI/Zone
        MetricSelect *metric1, *metric2, *metric3; // Metric/Interval/PMC
        MetricSelect *meta1; // Meta
        SeriesSelect *series1; // Zone Histogram

        // background color
        ColorButton *bgcolor;

};

class KPIOverviewItem : public ChartSpaceItem
{
    Q_OBJECT

    public:

        KPIOverviewItem(ChartSpace *parent, QString name, double start, double stop, QString program, QString units, bool istime);
        ~KPIOverviewItem();

        void itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
        void itemGeometryChanged();
        void setData(RideItem *item);
        void setDateRange(DateRange);

        QWidget *config() { return configwidget ; }

        // create and config
        static ChartSpaceItem *create(ChartSpace *parent) { return new KPIOverviewItem(parent, "CP Estimate", 0, 360, "{ round(estimate(cp3,cp)); }", "watts", false); }

        // settings
        double start, stop;
        QString program, units;
        bool istime;

        // computed and ready for painting
        QString value;

        // progress bar viz
        ProgressBar *progressbar;
        OverviewItemConfig *configwidget;
};

class DataOverviewItem : public ChartSpaceItem
{
    Q_OBJECT

    public slots:
        void intervalHover(IntervalItem *); // watching intervals being hovered
        void exportData();                  // export table to a csv file

    public:

        DataOverviewItem(ChartSpace *parent, QString name, QString program);
        ~DataOverviewItem();

        bool sceneEvent(QEvent *event); // click thru
        void wheelEvent(QGraphicsSceneWheelEvent *event);

        void itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
        QRectF hotspot();
        void itemGeometryChanged();
        void setData(RideItem *item);
        void setDateRange(DateRange);
        void postProcess(); // work with data returned from program
        void dragChanged(bool x);

        QWidget *config() { return configwidget; }

        // create and config
        static ChartSpaceItem *create(ChartSpace *parent);

        // transition support, get a program to mimic
        // the look and feel of the old ride summary
        static QString getLegacyProgram(int, DataFilterRuntime &, bool trends);

        // sort the datatable
        void sort(int column, Qt::SortOrder order);

        // settings
        QString program;
        Leaf *fnames, *funits, *fvalues, *ffiles, *fintervals, *fheat;

        // the data
        QVector<QString> names, units, values, files, intervals;
        QVector<double> heat;

        // display control
        bool multirow;
        QList<double> columnWidths;
        OverviewItemConfig *configwidget;

        bool click; // for clickthru
        RideItem *clickthru;
        QString hovered;       // we got told it was hovered
        QString hoverinterval; // last interval we hovered on in paint
        QString hoversignal;   // last interval we signalled for hover
        int sortcolumn; // for sorting a column

        // watching cursor moves when scrolling
        QPoint globalpos;

        int lastsort; // the column we last sorted on
        Qt::SortOrder lastorder; // the order we last sorted on

        VScrollBar *scrollbar;

};

class RPEOverviewItem : public ChartSpaceItem
{
    Q_OBJECT

    public:

        RPEOverviewItem(ChartSpace *parent, QString name);
        ~RPEOverviewItem();

        void itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
        void itemGeometryChanged();
        void setData(RideItem *item);
        void setDateRange(DateRange) {} // doesn't support trends view

        QWidget *config() { return configwidget; }

        // create and config
        static ChartSpaceItem *create(ChartSpace *parent) { return new RPEOverviewItem(parent, "RPE"); }

        // RPE meta field
        Sparkline *sparkline;
        RPErating *rperating;

        // for setting sparkline & painting
        bool up, showrange;
        QString value, upper, lower, mean;
        OverviewItemConfig *configwidget;
};

class MetricOverviewItem : public ChartSpaceItem
{
    Q_OBJECT

    public:

        MetricOverviewItem(ChartSpace *parent, QString name, QString symbol);
        ~MetricOverviewItem();

        void itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
        void itemGeometryChanged();
        void setData(RideItem *item);
        void setDateRange(DateRange);

        QWidget *config() { return configwidget; }

        // create and config
        static ChartSpaceItem *create(ChartSpace *parent) { return new MetricOverviewItem(parent, "PowerIndex", "power_index"); }

        QString symbol;
        RideMetric *metric;
        QString units;

        bool up, showrange;
        QString value, upper, lower, mean;

        Sparkline *sparkline;

        int rank; // rank 1,2 or 3
        QString beststring;
        QPixmap gold, silver, bronze; // medals

        OverviewItemConfig *configwidget;
};

// top N uses this to hold details for date range
struct topnentry {

public:

    topnentry(QDate date, double v, QString value, QColor color, int tsb, RideItem *item) : date(date), v(v), value(value), color(color), tsb(tsb), item(item) {}
    inline bool operator<(const topnentry &other) const { return (v > other.v); }
    inline bool operator>(const topnentry &other) const { return (v < other.v); }
    QDate date;
    double v; // for sorting
    QString value; // as should be shown
    QColor color; // ride color
    int tsb; // on the day
    RideItem *item;
};

class TopNOverviewItem : public ChartSpaceItem
{
    Q_OBJECT

    // want a meta property for property animation
    Q_PROPERTY(int transition READ getTransition WRITE setTransition)

    public:

        TopNOverviewItem(ChartSpace *parent, QString name, QString symbol);
        ~TopNOverviewItem();

        // transition animation 0-100
        int getTransition() const {return transition;}
        void setTransition(int x) { if (transition !=x) {transition=x; update();}}

        bool sceneEvent(QEvent *event) override;
        void itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;
        void itemGeometryChanged() override;
        void setData(RideItem *) override {} // doesn't support analysis view
        void setDateRange(DateRange) override;
        QRectF hotspot() override;

        QWidget *config() override { return configwidget; }

        // create and config
        static ChartSpaceItem *create(ChartSpace *parent) { return new TopNOverviewItem(parent, "PowerIndex", "power_index"); }
        void configChanged(qint32) override;

        QString symbol;
        RideMetric *metric;
        QString units;

        QList<topnentry> ranked;

        // maximums to index from
        QString maxvalue;
        double maxv,minv;

        // animation
        int transition;
        QPropertyAnimation *animator;

        // interaction
        bool click;
        RideItem *clickthru;

        OverviewItemConfig *configwidget;
};

class MetaOverviewItem : public ChartSpaceItem
{
    Q_OBJECT

    public:

        MetaOverviewItem(ChartSpace *parent, QString name, QString symbol);
        ~MetaOverviewItem();

        void itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;
        void itemGeometryChanged() override;
        void setData(RideItem *item) override;
        void setDateRange(DateRange) override {} // doesn't support trends view

        QWidget *config() override { return configwidget; }

        // create and config
        static ChartSpaceItem *create(ChartSpace *parent) { return new MetaOverviewItem(parent, tr("Workout Code"), "Workout Code"); }

        void configChanged(qint32) override;

        QString symbol;
        int fieldtype;

        // for numeric metadata items
        bool up, showrange;
        QString value, upper, lower, mean;

        Sparkline *sparkline;

        OverviewItemConfig *configwidget;
};

class PMCOverviewItem : public ChartSpaceItem
{
    Q_OBJECT

    public:

        PMCOverviewItem(ChartSpace *parent, QString symbol);
        ~PMCOverviewItem();

        void itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
        void itemGeometryChanged();
        void setData(RideItem *item);
        void setDateRange(DateRange) {} // doesn't support trends view

        QWidget *config() { return configwidget ; }

        // create and config
        static ChartSpaceItem *create(ChartSpace *parent) { return new PMCOverviewItem(parent, "coggan_tss"); }

        QString symbol;

        double sts, lts, sb, rr, stress;

        OverviewItemConfig *configwidget;
};

class ZoneOverviewItem : public ChartSpaceItem
{
    Q_OBJECT

    public:

        ZoneOverviewItem(ChartSpace *parent, QString name, RideFile::seriestype, bool polarized);
        ~ZoneOverviewItem();

        void itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;
        void itemGeometryChanged() override;
        void setData(RideItem *item) override;
        void setDateRange(DateRange) override;
        void dragChanged(bool x) override;

        QWidget *config() override { return configwidget; }

        // create and config
        static ChartSpaceItem *create(ChartSpace *parent) { return new ZoneOverviewItem(parent, tr("Power Zones"), RideFile::watts, false); }
        void configChanged(qint32) override;

        RideFile::seriestype series;
        bool polarized;

        QChart *chart;
        QBarSet *barset;
        QBarSeries *barseries;
        QStringList categories;
        QBarCategoryAxis *barcategoryaxis;

        OverviewItemConfig *configwidget;
};

struct aggmeta {
    aggmeta(QString category, double value, double percentage, double count)
    : category(category), value(value), percentage(percentage), count(count) {}

    QString category;
    double value, percentage, count;
};

class DonutOverviewItem : public ChartSpaceItem
{
    Q_OBJECT

    public:

        DonutOverviewItem(ChartSpace *parent, QString name, QString symbol, QString meta);
        ~DonutOverviewItem();

        void itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
        void itemGeometryChanged();
        void setData(RideItem *) {} // trends view only
        void setDateRange(DateRange);
        void dragChanged(bool x);

        QWidget *config() { return configwidget ; }

        // create and config
        static ChartSpaceItem *create(ChartSpace *parent) { return new DonutOverviewItem(parent, tr("Sport"), "ride_count", "Sport"); }

        // config
        QString symbol, meta;
        RideMetric *metric;

        // Categories and values
        QVector<aggmeta> values;
        QString value, valuename; // currently hovered...

        // Viz
        QChart *chart;
        QBarSet *barset;
        QBarSeries *barseries;
        QBarCategoryAxis *barcategoryaxis;

        OverviewItemConfig *configwidget;

    public slots:
        void hoverSlice(QPieSlice *slice, bool state);
};

class RouteOverviewItem : public ChartSpaceItem
{
    Q_OBJECT

    public:

        RouteOverviewItem(ChartSpace *parent, QString name);
        ~RouteOverviewItem();

        void itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
        void itemGeometryChanged();
        void setData(RideItem *item);
        void setDateRange(DateRange) {} // doesn't support trends view

        QWidget *config() { return configwidget; }

        // create and config
        static ChartSpaceItem *create(ChartSpace *parent) { return new RouteOverviewItem(parent, tr("Route")); }

        Routeline *routeline;

        OverviewItemConfig *configwidget;
};

class IntervalOverviewItem : public ChartSpaceItem
{
    Q_OBJECT

    public:

        IntervalOverviewItem(ChartSpace *parent, QString name, QString xs, QString ys, QString zs);
        ~IntervalOverviewItem();

        void itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
        void itemGeometryChanged();
        void setData(RideItem *item);
        void setData(RideItem *item, bool animate);
        void setDateRange(DateRange);

        QWidget *config() { return configwidget; }

        // create and config
        static ChartSpaceItem *createInterval(ChartSpace *parent) { return new IntervalOverviewItem(parent, tr("Intervals"), "elapsed_time", "average_power", "workout_time"); }
        static ChartSpaceItem *createActivities(ChartSpace *parent) { return new IntervalOverviewItem(parent, tr("Activities"), "activity_date", "average_power", "coggan_tss"); }

        QString xsymbol, ysymbol, zsymbol;
        int xdp, ydp;
        BubbleViz *bubble;

        bool block; // block when signals occur too quickly
        RideItem *item;  // remember what we are showing
        IntervalItem *hover; // currently being hovered

        OverviewItemConfig *configwidget;

    public slots:
        void intervalSelectRefresh();
        void intervalHover(IntervalItem *);
};


//
// below are theviz widgets used by the overview items
//

// for now the basics are x and y and a radius z, color fill
class BPointF {
public:

    BPointF() : x(0), y(0), z(0), xoff(0), yoff(0), fill(GColor(Qt::gray)), item(NULL) {}

    double score(BPointF &other);

    double x,y,z;
    double xoff, yoff; // add to x,y,z when converting to string (used for dates)
    QColor fill;
    QString label;
    RideItem *item;
};


// bubble chart, very very basic just a visualisation
class BubbleViz : public QObject, public QGraphicsItem
{
    // need to be a qobject for metaproperties
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

    // want a meta property for property animation
    Q_PROPERTY(int transition READ getTransition WRITE setTransition)
    Q_PROPERTY(QPointF yaxis READ getYAxis WRITE setYAxis)
    Q_PROPERTY(QPointF xaxis READ getXAxis WRITE setXAxis)

    public:
        BubbleViz(IntervalOverviewItem *parent, QString name=""); // create and say how many days
        ~BubbleViz();

        // we monkey around with this *A LOT*
        void setGeometry(double x, double y, double width, double height);
        QRectF geometry() { return geom; }

        // set hover signal for intervals
        void setIntervalHoverSignal(bool x) { intervalsignal=x; }

        // transition animation 0-255
        int getTransition() const {return transition;}
        void setTransition(int x) { if (transition !=x) {transition=x; update();}}

        // axes
        QPointF getYAxis() const { return QPointF(miny,maxy); }
        void setYAxis(QPointF x) { miny=x.x(); maxy=x.y(); update(); }
        QPointF getXAxis() const { return QPointF(minx,maxx); }
        void setXAxis(QPointF x) { minx=x.x(); maxx=x.y(); update(); }

        // null members for now just get hooked up
        void setPoints(QList<BPointF>points, double minx, double maxx, double miny, double maxy, bool animate=true);

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
        bool click;
        RideItem *clickthru;
        QPointF plotpos;

        // we should signal interval hovers?
        bool intervalsignal;
        QString intervalhover, lastintervalsignal;

        // for animated transition
        QList <BPointF> oldpoints; // for animation
        int transition;
        double oldmean;

        QSequentialAnimationGroup *group;
        QPropertyAnimation *transitionAnimation, *xaxisAnimation, *yaxisAnimation;

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
        Sparkline(QGraphicsWidget *parent, QString name="", bool bigdot=true); // create and say how many days

        // we monkey around with this *A LOT*
        void setGeometry(double x, double y, double width, double height);
        QRectF geometry() { return geom; }

        void setDays(int n) { sparkdays=n; } // defaults to SPARKDAYS
        void setFill(bool x) { fill = x; }
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
        int sparkdays;
        bool bigdot;
        bool fill;
        QList<QPointF> points;
};

// vertical scrollbar
class VScrollBar : public QGraphicsItem
{
    public:
        VScrollBar(QGraphicsWidget *parent, ChartSpace *space); // create empty, geometry and boundary gets changed all the time

        // we monkey around with this *A LOT*
        void setGeometry(double x, double y, double width, double height);
        QRectF geometry() { return geom; }

        // the size of the scroll area
        void setAreaHeight(double n);

        // pos - this is the position in the area being scrolled
        //       that should be painted in the viewable area
        //       since its a vertical scrollbar its the y position
        //       it can be set by parent when e.g. catch key events
        void setPos(double x);
        void movePos(int x);
        double pos() const;

        bool isDragging() { return state == DRAG; }

        // needed as pure virtual in QGraphicsItem
        QRectF boundingRect() const override { return geom; }
        QVariant itemChange(GraphicsItemChange change, const QVariant &value) override
        {
            if (change == ItemPositionChange && parent->scene())  prepareGeometryChange();
            return QGraphicsItem::itemChange(change, value);
        }

        // the usual
        void paint(QPainter*, const QStyleOptionGraphicsItem *, QWidget*) override;

        // spotting mouse events hover, click move and wheel (but only in small area of scrollbar)
        bool sceneEvent(QEvent *event) override;

        bool canscroll; // can we do scrolling?

    private:

        QGraphicsWidget *parent;
        ChartSpace *space;
        QRectF geom;
        double height;
        bool hover; // is mouse in our rect?
        bool barhover; // is mouse over the scrollbar itself?

        QPointF origin;
        enum { NONE, DRAG } state;
        double barpos, obarpos;
};

// visualisation of a GPS route as a shape
class Routeline : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
    Q_PROPERTY(int transition READ getTransition WRITE setTransition)

    public:
        Routeline(QGraphicsWidget *parent, QString name=""); // create and say how many days
        ~Routeline();

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

// progress bar to show percentage progress from start to goal
class ProgressBar : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
    Q_PROPERTY(double value READ getCurrentValue WRITE setCurrentValue)

    public:
        ProgressBar(QGraphicsWidget *parent, double start, double stop, double value);
        ~ProgressBar();

        void setValue(double start, double stop, double value);

        // we monkey around with this *A LOT*
        void setGeometry(double x, double y, double width, double height);
        QRectF geometry() { return geom; }

        // needed as pure virtual in QGraphicsItem
        QVariant itemChange(GraphicsItemChange change, const QVariant &value);
        void paint(QPainter*, const QStyleOptionGraphicsItem *, QWidget*);
        QRectF boundingRect() const { return QRectF(parent->geometry().x() + geom.x(),
                                                    parent->geometry().y() + geom.y(),
                                                    geom.width(), geom.height());
        }

        // transition animation between old and new values
        double getCurrentValue() const {return value;}
        void setCurrentValue(double x) { if (value !=x) {value=x; update();}}

    private:
        QGraphicsWidget *parent;
        QRectF geom;

        double start, stop, value;

        QPropertyAnimation *animator;
};

// simple button to use on graphics views
class Button : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

    public:
        Button(QGraphicsItem *parent, QString text);

        void setText(QString text) { this->text = text; update(); }
        void setFont(QFont font) { this->font = font; }

        // we monkey around with this *A LOT*
        void setGeometry(double x, double y, double width, double height);
        QRectF geometry() { return geom; }


        // needed as pure virtual in QGraphicsItem
        void paint(QPainter*, const QStyleOptionGraphicsItem *, QWidget*);

        QRectF boundingRect() const {
            QPointF pos=mapToParent(geom.x(), geom.y());
            return QRectF(pos.x(), pos.y(), geom.width(), geom.height());
        }

        // for interaction
        bool sceneEvent(QEvent *event);

    signals:
        void clicked();

    private:
        QGraphicsItem *parent;
        QString text;
        QFont font;

        QRectF geom;
        enum { None, Clicked } state;
};

#endif // _GC_OverviewItem_h
