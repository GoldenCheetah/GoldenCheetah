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
#include "Athlete.h"

#include "WPrime.h"

#include "Settings.h"
#include "Units.h"
#include "Colors.h"

#include <QWidget>
#include <QRect>
#include <QPoint>
#include <QVector>
#include <QPainterPath>

class ErgFile;
class WorkoutWindow;
class WorkoutWidget;
class WWPoint;

// memento represent a point, used to save state before/after commands
// or whilst in the process of creating things like blocks
struct PointMemento {

    public:
        PointMemento(double x, double y, int index) : x(x), y(y), index(index) {}
        double x,y;
        int index;
};

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

        // interaction state;
        // none - initial state
        // drag - dragging a point around
        // dragblock - dragging a block aroung
        // rect - rectangle select tool active
        // create - clicked to create
        enum { none, create, drag, dragblock, rect } state;

        // adding items and points
        void addItem(WorkoutWidgetItem*x) { children_.append(x); }
        void addPoint(WWPoint*x) { points_.append(x); }

        // get list of my items
        QList<WorkoutWidgetItem*> &children() { return children_; }

        // get list of my items
        QList<WWPoint*> &points() { return points_; }

        // get WPrime values
        WPrime &wprime() { return wpBal; }

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

        // CP data
        QVector<int> wattsArray;
        QVector<int>mmpArray;

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

        // where were we when we changed state?
        QPointF onCreate;   // select a point
        QPointF onDrag;    // drag a point
        QPointF onRect, atRect;    // rectangle select tool, top left, bottom right

        // the points that make up the block thats being created
        QList<PointMemento> cr8block;

        // the block the cursor is hovering in
        QPainterPath cursorBlock;
        QString cursorBlockText, cursorBlockText2;

        // the block area created by the current selections
        QPainterPath selectionBlock;
        QString selectionBlockText, selectionBlockText2;

        // the point we are currently dragging
        WWPoint *dragging;

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

        // working with the clipboard
        void cut();
        void copy();
        void paste();

    protected:

        // interacting with points
        bool movePoint(QPoint p);
        bool createPoint(QPoint p);
        bool scale(QPoint p);
        bool movePoints(int, Qt::KeyboardModifiers);

        // deleting
        bool deleteSelected();

        // selecting
        bool selectAll();
        bool selectPoints(); // mark for selection with rect tool
        bool selectedPoints(); // make selected at end rect tool
        bool selectClear(); // clear all selections

        // working with blocks
        bool createBlock(QPoint p);
        bool moveBlock(QPoint p);
        bool setBlockCursor();

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

        // for computing W'bal
        WPrime wpBal;
};

#endif // _GC_WorkoutWidget_h
