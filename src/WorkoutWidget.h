/*
 * Copyright (c) 2015 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_WorkoutWidget_h
#define _GC_WorkoutWidget_h 1
#include "GoldenCheetah.h"

#include "Context.h"

#include "Settings.h"
#include "Units.h"
#include "Colors.h"

#include <QWidget>
#include <QRect>
#include <QPoint>
#include <QVector>

class ErgFile;
class WorkoutWindow;
class WorkoutWidget;
class WWPoint;
class WorkoutWidgetItem {

    public:

        WorkoutWidgetItem(WorkoutWidget *w);
        virtual ~WorkoutWidgetItem() {}

        // where are we attached?
        WorkoutWidget *workoutWidget() const { return w; }

        // Reimplement in children
        virtual int type() = 0;

        // paint on parent - it must use the parent methods
        // to get rect to paint in and map from time/power to x/y
        // since the painter draws onto the parent directly
        virtual void paint(QPainter *painter) = 0;

        // locate me on the parent widget in paint coordinates
        virtual QRectF bounding() = 0;

    private:
        WorkoutWidget *w;
};

// base for all commands that operate on a workout (e.g. create
// point, drag point, delete etc etc used by redo/undo
class WorkoutWidgetCommand
{
    public:

        WorkoutWidgetCommand(WorkoutWidget *w); // will add to stack
        virtual ~WorkoutWidgetCommand() {}

        WorkoutWidget *workoutWidget() { return workoutWidget_; }

        // need to reimplement these in subclass
        virtual void undo() = 0;
        virtual void redo() = 0;

    private:
        WorkoutWidget *workoutWidget_;
};

class WorkoutWidget : public QWidget
{
    Q_OBJECT

    public:

        WorkoutWidget(WorkoutWindow *parent, Context *context);

        // adding items and points
        void addItem(WorkoutWidgetItem*x) { children_.append(x); }
        void addPoint(WWPoint*x) { points_.append(x); }

        // get list of my items
        QList<WorkoutWidgetItem*> &children() { return children_; }

        // get list of my items
        QList<WWPoint*> &points() { return points_; }

        // range for scales in plot units not draw units
        double maxX(); // e.g. max watts
        double maxY(); // e.g. max seconds
        double minX() { return 0.0f; } // might change later
        double minY() { return 0.0f; } // might change later

        // transform from plot to painter co-ordinate
        QPoint transform(double x, double y);

        // transform from painter to plot co-ordinate
        QPointF reverseTransform(int, int);

        // ergFile being edited
        ErgFile *ergFile;

        // get regions for items to paint in
        QRectF left();
        QRectF right();
        QRectF bottom();
        QRectF top();
        QRectF canvas();

        // shared with items
        QFont bigFont;
        QFont markerFont;
        QPen markerPen;
        QPen gridPen;

   public slots:

        // and erg file was selected
        void ergFileSelected(ErgFile *);

        // recompute metrics etc
        void recompute();

        // trap signals
        void configChanged(qint32);

        // timeout on click timer
        void timeout();

        // if done mid stack, the stack is truncated
        void addCommand(WorkoutWidgetCommand *cmd);

        // redo / undo command by going
        // up and down the stack
        void redo();
        void undo();

    protected:

        // interaction state
        enum { none, drag } state;
        WWPoint *dragging;

        // interacting with points
        bool movePoint(QPoint p);
        bool createPoint(QPoint p);
        bool scale(QPoint p);

        void paintEvent(QPaintEvent *);
        bool eventFilter(QObject *obj, QEvent *event);

    private:

        WorkoutWindow *parent;
        Context *context;
        QList<WorkoutWidgetItem*> children_;

        // we keep these separate to make the maintenance
        // and code simpler, it helps when moving them around
        // as don't have to keep searching through all objects
        QList<WWPoint*> points_;

        double maxX_, maxY_;

        // command stack
        QList<WorkoutWidgetCommand *> stack;
        int stackptr;

        // where were we when we changed state?
        QPoint onCreate;
        QPointF onDrag;
};

#endif // _GC_WorkoutWidget_h
