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

#ifndef _GC_ChartSpace_h
#define _GC_ChartSpace_h 1

// basics
#include "GoldenCheetah.h"
#include "Settings.h"
#include "Units.h"
#include "Colors.h"
#include "Context.h"
#include "Athlete.h"
#include "RideItem.h"

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

// geometry basics
#define SPACING 80
#define ROWHEIGHT 80

extern double gl_major;

class ChartSpace;
class ChartSpaceItemFactory;

// we need a scope for a chart space, one or more of
enum OverviewScope { ANALYSIS=0x01, TRENDS=0x02, ATHLETES=0x04 };

// must be subclassed to add items to a ChartSpace
class ChartSpaceItem : public QGraphicsWidget
{
    Q_OBJECT

    friend class ::ChartSpace;

    public:

        // When subclassing you must reimplement these
        virtual void itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) =0;
        virtual void itemGeometryChanged() =0;
        virtual void setData(RideItem *item)=0;
        virtual void setDateRange(DateRange )=0;
        virtual QColor color();
        virtual QRectF hotspot() { return QRectF(0,0,0,0); } // don't steal events from this area of the item

        virtual QWidget *config()=0; // must supply a widget to configure
        virtual void configChanged(qint32) {}

        // turn off/on the config corner button
        void setShowConfig(bool x) { showconfig=x; update(); }
        bool showConfig() const { return showconfig; }

        // let item know that dragging is in process
        // for some widgets (e.g. UserChart) this means
        // its best to hide the widgets as rendering into
        // the scene can be really slow. For other widgets
        // it can be safely ignored.
        virtual void dragging(bool) { }

        // what type am I- managed by user
        int type;

        ChartSpaceItem(ChartSpace *parent, QString name) : QGraphicsWidget(NULL),
                                       parent(parent), name(name),
                                       column(0), order(0), deep(5), onscene(false),
                                       placing(false), drag(false), invisible(false),
                                       showconfig(true)  {

            setAutoFillBackground(false);
            setFlags(flags() | QGraphicsItem::ItemClipsToShape); // don't paint outside the card
            setAcceptHoverEvents(true);

            setZValue(10);

            // a sensible default?
            span = 1;
            type = 0;
            delcounter=0;

#if 0
            effect = new QGraphicsDropShadowEffect();
            effect->setXOffset(6);
            effect->setYOffset(6);
            effect->setColor(QColor(127,127,127,64));
            effect->setBlurRadius(10);
            this->setGraphicsEffect(effect);
#endif

            bgcolor = StandardColor(CCARDBACKGROUND).name();

            // watch geom changes
            connect(this, SIGNAL(geometryChanged()), SLOT(geometryChanged()));
        }

        // watch mouse enter/leave
        bool sceneEvent(QEvent *event);
        bool inCorner();
        bool inHotspot();
        bool underMouse();

        // keep track of reuse of xyseries and delete to
        // try and minimise memory leak in Qt Chart
        int delcounter;

        // when dragging around
        void setDrag(bool x);
        virtual void dragChanged(bool) {}

        // my parent
        ChartSpace *parent;

        // what to do if clicked XXX just a hack for now
        void clicked();

        // name
        QString name;

        // datafilter - only relevant on trends view
        QString datafilter;

        // which column, sequence and size in rows
        int column, span, order, deep;
        bool onscene, placing, drag;
        bool incorner;
        bool invisible;
        bool showconfig;
        QString bgcolor;
        QGraphicsDropShadowEffect *effect;

        // base paint
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);

    public slots:

        void geometryChanged();
};

// we copy the current items and manage their layout (which is an iterative process)
// and once the layout is done we create an animation to move from the current positions
// to the new positions using this class to temporarily store new co-ordinates
class LayoutChartSpaceItem {

    public:
        LayoutChartSpaceItem(ChartSpaceItem *from) :
            column(from->column), span(from->span), order (from->order ), deep(from->deep), onscene(from->onscene),
            placing(from->placing), drag(from->drag), incorner(from->incorner), invisible(from->invisible),
            item(from), geometry(from->geometry()) {}

        static bool LayoutChartSpaceItemSort(const LayoutChartSpaceItem left, const LayoutChartSpaceItem right);

        int column, span, order, deep;
        bool onscene, placing, drag;
        bool incorner;
        bool invisible;

        ChartSpaceItem *item;
        QRectF geometry;
};

class ChartSpace : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QRectF viewRect READ getViewRect WRITE setViewRect)
    Q_PROPERTY(int viewY READ getViewY WRITE setViewY)

    public:

        ChartSpace(Context *context, int scope, GcWindow *window);
        QGraphicsScene *getScene() { return scene; }

        // current state for event processing
        enum { NONE, DRAG, SPAN, XRESIZE, YRESIZE } state;

        void setMinimumColumns(int x) { mincols=x; updateGeometry(); updateView(); }
        int minimumColumns() const { return mincols; }

        // used by children
        Context *context;
        int scope;
        int mincols;

        QGraphicsView *view;
        QFont titlefont, bigfont, midfont, smallfont, tinyfont;

        // the item we are currently showing
        RideItem *currentRideItem;
        DateRange currentDateRange;

        // to get paint device
        QGraphicsView *device() { return view; }
        const QList<ChartSpaceItem*> allItems() { return items; }

        // window we are rendered in
        GcWindow *window;


    signals:
        void itemConfigRequested(ChartSpaceItem*);

    public slots:

        // user selection
        void rideSelected(RideItem *item);
        void dateRangeChanged(DateRange);
        void filterChanged();

        // column sizing
        QVector<int> columnWidths() { return columns; }
        void setColumnWidths(QVector<int> x) { columns =x; updateGeometry(); }

        // for smooth scrolling
        void setViewY(int x) { if (_viewY != x) {_viewY =x; updateView();} }
        int getViewY() const { return _viewY; }

        // for smooth scaling
        void setViewRect(QRectF);
        QRectF getViewRect() const { return viewRect; }

        // trap signals
        void refresh();
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
        QList<LayoutChartSpaceItem> layoutItems(); // moves geom and lays out items

        // set scale, zoom etc appropriately
        void updateView();

        // add a ChartSpaceItem to the view
        void addItem(int row, int column, int span, int deep, ChartSpaceItem *item);

        // remove an item
        void removeItem(ChartSpaceItem *item);

        // mostly for athlete view, we just set a fixed
        // zoom width, so we don't get a massive athlete
        // card when only one athlete
        void setFixedZoom(int width);

        // which column are we in for position x
        int columnForX(int x);
        // how many items are in this column?
        int columnCount(int x);

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
        int fixedZoom;
        int _viewY;
        QRectF sceneRect;
        QRectF viewRect;

        // content
        QVector<int> columns;                // column widths
        QList<ChartSpaceItem*> items;         // tiles

        // state data
        bool yresizecursor;          // is the cursor set to resize?
        bool xresizecursor;          // is the cursor set to resize?
        bool block;                  // block event processing
        bool scrolling;              // scrolling the view?
        bool setscrollbar;           // distinguish between user and program actions
        double lasty;                // to see if the mouse is moving up or down

        union ChartSpaceState {
            struct {
                double offx, offy; // mouse grab position on card
                ChartSpaceItem *item;        // index of card in QList
                int width;         // how big was I when I started dragging?
            } drag;

            struct {
                double posy;
                int deep;
                ChartSpaceItem *item;
            } yresize;

            struct {
                ChartSpaceItem *item; // span resize
                double posx;
                int width;
                int column;
            } xresize;

        } stateData;

        bool stale;
        bool configured;
};

// each chart has an entry like this in the registry
struct ChartSpaceItemDetail {

    ChartSpaceItemDetail() : ChartSpaceItemDetail(0,"","",0,NULL) {}
    ChartSpaceItemDetail(int type, QString quick, QString description, int scope, ChartSpaceItem* (*create)(ChartSpace *))
        : type(type), scope(scope), quick(quick), description(description), create(create) {}

    int type, scope; // scope needs enums at some point
    QString quick, description;
    ChartSpaceItem* (*create)(ChartSpace *);
};

// registry of items and their details
class ChartSpaceItemRegistry {

    public:

        // singleton instance
        static ChartSpaceItemRegistry &instance() {
            if (!_instance) _instance = new ChartSpaceItemRegistry();
            return *_instance;
        }

        // register chartspace items
        bool addItem(int type, QString quick, QString description, int scope, ChartSpaceItem* (*create)(ChartSpace *)) {

            // add to registry
            _items.append(ChartSpaceItemDetail(type, quick, description, scope, create));
            return true;
        }

        // get the detail spec for type requested
        ChartSpaceItemDetail detailForType(int type);

        // get the registry
        QList<ChartSpaceItemDetail> &items() { return _items; }

    private:

        static ChartSpaceItemRegistry *_instance;
        QList<ChartSpaceItemDetail> _items;

        // constructors
        ChartSpaceItemRegistry() {}
};
#endif // _GC_ChartSpace_h
