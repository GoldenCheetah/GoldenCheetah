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

// QGraphics
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsWidget>
#include <QGraphicsDropShadowEffect>

// qt
#include <QtGui>
#include <QScrollBar>

class OverviewWindow;

// keep it simple for now
class Card : public QGraphicsWidget
{
    public:

        Card(int deep) : QGraphicsWidget(NULL), column(0), order(0), deep(deep), onscene(false),
                                                placing(false), invisible(false) {

            // no mouse event delivery allowed to contained QWidgets-
            // this is so we can normal embed charts etc
            // but you can't interact (e.g. steal focus, mousewheel etc) whilst
            // they are in the dashboard. In order to interact you will need to
            // have focus explicitly enabled by the parent, e.g. by clicking on it
            // or maximising it etc
            //child->setAttribute(Qt::WA_TransparentForMouseEvents);
            //child->setAttribute(Qt::WA_ForceDisabled);

            // shadow (disabled, isn't appropriate)
            //QGraphicsDropShadowEffect * effect = new QGraphicsDropShadowEffect();
            //effect->setBlurRadius(3);
            //setGraphicsEffect(effect);

            setAutoFillBackground(false);
            brush = QBrush(GColor(CCARDBACKGROUND));
            setZValue(10);
        }

        // what to do if clicked XXX just a hack for now
        void clicked();

        // which column, sequence and size in rows
        int column, order, deep;
        bool onscene, placing;
        bool invisible;

        void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
            painter->setBrush(brush);
            painter->fillRect(QRectF(0,0,geometry().width()+1,geometry().height()+1), brush);

        }

        QBrush brush;
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

    public slots:

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

        // create a card
        Card *newCard(int column, int order, int deep) { Card *add = new Card(deep);
                                                         add->column = column;
                                                         add->order = order;
                                                         add->deep = deep;
                                                         cards.append(add);
                                                         return add;
                                                        }
    protected:

        // process events
        bool eventFilter(QObject *, QEvent *event);

    private:

        // gui setup
        Context *context;
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
