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

class OverviewWindow;

// keep it simple for now
class Card : public QGraphicsWidget
{
    public:

        Card(int deep) : QGraphicsWidget(NULL), column(0), order(0), deep(deep), onscene(false) {
            setAutoFillBackground(true);
        }

        // which column, sequence and size in rows
        int column, order, deep;
        bool onscene;
};

// qt
#include <QtGui>

class OverviewWindow : public GcChartWindow
{
    Q_OBJECT

    public:

        OverviewWindow(Context *context);


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
        bool eventFilter(QObject *obj, QEvent *event);

    private:

        Context *context;
        QGraphicsScene *scene;
        QGraphicsView *view;

        QList<Card*> cards;

};

#endif // _GC_OverviewWindow_h
