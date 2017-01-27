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

// qt
#include <QtGui>

class OverviewWindow;

// keep it simple for now
class Card : public QGraphicsWidget
{
    public:

        Card(int deep) : QGraphicsWidget(NULL), column(0), order(0), deep(deep), onscene(false),
                                                invisible(false) {
            setAutoFillBackground(true);
            brush = QBrush(GColor(CCARDBACKGROUND));
            setZValue(10);
        }

        // what to do if clicked XXX just a hack for now
        void clicked();

        // which column, sequence and size in rows
        int column, order, deep;
        bool onscene;
        bool invisible;

        void paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *p=0) {
            painter->setBrush(brush);
            painter->fillRect(QRectF(0,0,geometry().width(),geometry().height()), brush);

        }

        QBrush brush;
};

class OverviewWindow : public GcChartWindow
{
    Q_OBJECT

    public:

        OverviewWindow(Context *context);

        // are we just looking or changing
        enum { VIEW, CONFIG } mode;

        // current state for event processing
        enum { NONE, DRAG } state;

   public slots:

        // trap signals
        void configChanged(qint32);

        // viewport resized
        void resizeEvent(QResizeEvent * event);

        // set geometry on the widgets (size and pos)
        void updateGeometry();

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
        bool eventFilter(QObject *obj, QEvent *event);

    private:

        // gui setup
        Context *context;
        QGraphicsScene *scene;
        QGraphicsView *view;
        QParallelAnimationGroup *group;

        // content
        QList<Card*> cards;

        // state data
        union OverviewState {
            struct {
                double offx, offy; // mouse grab position on card
                Card *card;        // index of card in QList
            } drag;
        } stateData;

};

#endif // _GC_OverviewWindow_h
