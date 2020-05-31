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

class ChartSpace;

// must be subclassed to add items to a ChartSpace
class ChartSpaceItem : public QGraphicsWidget
{
    Q_OBJECT

    public:

        // When subclassing you must reimplement these
        virtual void itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) =0;
        virtual void itemGeometryChanged() =0;
        virtual void setData(RideItem *item)=0;

        // what type am I- managed by user
        int type;

        ChartSpaceItem(ChartSpace *parent, QString name) : QGraphicsWidget(NULL),
                                       parent(parent), name(name),
                                       column(0), order(0), deep(5), onscene(false),
                                       placing(false), drag(false), invisible(false)  {

            setAutoFillBackground(false);
            setFlags(flags() | QGraphicsItem::ItemClipsToShape); // don't paint outside the card
            setAcceptHoverEvents(true);

            setZValue(10);

            // a sensible default?
            type = 0;
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

        // when dragging around
        void setDrag(bool x);
        virtual void dragChanged(bool) {}

        // my parent
        ChartSpace *parent;

        // what to do if clicked XXX just a hack for now
        void clicked();

        // name
        QString name;

        // which column, sequence and size in rows
        int column, order, deep;
        bool onscene, placing, drag;
        bool incorner;
        bool invisible;

        // base paint
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);

    public slots:

        void geometryChanged();
};

class ChartSpace : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QRectF viewRect READ getViewRect WRITE setViewRect)
    Q_PROPERTY(int viewY READ getViewY WRITE setViewY)

    public:

        ChartSpace(Context *context);

        // current state for event processing
        enum { NONE, DRAG, XRESIZE, YRESIZE } state;

        // used by children
        Context *context;
        QGraphicsView *view;
        QFont titlefont, bigfont, midfont, smallfont;

        // the item we are currently showing
        RideItem *current;

        // to get paint device
        QGraphicsView *device() { return view; }
        const QList<ChartSpaceItem*> allItems() { return items; }


    public slots:

        // ride item changed
        void rideSelected(RideItem *item);

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

        // add a ChartSpaceItem to the view
        void addItem(int row, int column, int deep, ChartSpaceItem *item);

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
                double posx;
                int width;
                int column;
            } xresize;

        } stateData;

        bool stale;
        bool configured;
};

#endif // _GC_ChartSpace_h
