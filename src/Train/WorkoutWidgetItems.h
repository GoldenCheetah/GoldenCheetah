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

#ifndef _GC_WorkoutWidgetItems_h
#define _GC_WorkoutWidgetItems_h 1

#include "GoldenCheetah.h"

#include "WorkoutWidget.h" // also declares workoutwidgetitem

#include "Settings.h"
#include "Units.h"
#include "Colors.h"

#include <QWidget>
#include <QRect>

// types
#define GCWW_SCALE     1
#define GCWW_POINT     2
#define GCWW_LINE      3
#define GCWW_WBALSCALE 4 //W'bal
#define GCWW_WBLINE    5
#define GCWW_RECT      6 // selection rectangle
#define GCWW_BCURSOR   7 // interval "block" cursor
#define GCWW_BRECT     8 // block selection
#define GCWW_MMPCURVE  9
#define GCWW_SGUIDE    10 // smart guides appear as we drag
#define GCWW_TTE       11 // highlight impossible sections
#define GCWW_LAP       12 // lap markers
#define GCWW_NOW       13 // now marker
#define GCWW_TELEMETRY 14 // now marker

//
// ITEMS
//
class WWPowerScale : public WorkoutWidgetItem {

    public:

        WWPowerScale(WorkoutWidget *w, Context *c);

        // Reimplement in children
        int type() { return GCWW_SCALE; }

        void paint(QPainter *painter);

        // locate me on the parent widget in paint coordinates
        QRectF bounding() { return workoutWidget()->left(); }

    private:

        Context *context; // for athlete zones etc
};

class WWWBalScale : public WorkoutWidgetItem {

    public:

        WWWBalScale(WorkoutWidget *w, Context *c);

        // Reimplement in children
        int type() { return GCWW_WBALSCALE; }

        void paint(QPainter *painter);

        // locate me on the parent widget in paint coordinates
        QRectF bounding() { return workoutWidget()->right(); }

    private:

        Context *context; // for athlete zones etc
};

// mark regions of workout that are not possible
class WWTTE : public WorkoutWidgetItem {

    public :
        WWTTE(WorkoutWidget *w) : WorkoutWidgetItem(w) { w->addItem(this); }

        int type() { return GCWW_TTE; }

        void paint(QPainter *painter);

        // locate me
        QRectF bounding() { return workoutWidget()->bottomgap(); }
};

class WWMMPCurve : public WorkoutWidgetItem {

    public:

        WWMMPCurve(WorkoutWidget *w) : WorkoutWidgetItem(w) { w->addItem(this); }

        // Reimplement in children
        int type() { return GCWW_MMPCURVE; }

        void paint(QPainter *painter);

        // locate me on the parent widget in paint coordinates
        QRectF bounding() { return workoutWidget()->canvas(); }

};

// lap marker at top 
class WWLap : public WorkoutWidgetItem {
    public:

        WWLap(WorkoutWidget *w) : WorkoutWidgetItem(w) { w->addItem(this); }

        // Reimplement in children
        int type() { return GCWW_LAP; }

        void paint(QPainter *painter);

        // locate me on the parent widget in paint coordinates
        QRectF bounding() { return workoutWidget()->top(); }

    private:
};

// is a point, can be manipulated ...
class WWPoint : public WorkoutWidgetItem {

    public:

        WWPoint(WorkoutWidget *w, double x, double y, bool append=true) :
                WorkoutWidgetItem(w), hover(false), selected(false), selecting(false), x(x), y(y) { if (append) w->addPoint(this); }

        // Reimplement in children
        int type() { return GCWW_POINT; }

        void paint(QPainter *painter);

        // locate me on the parent widget in paint coordinates
        QRectF bounding() { return bound; }

        bool hover;     // mouse hovering
        bool selected;  // has been selected
        bool selecting; // in the process of being selected (rect tool)

        double x, y;

    private:
        QRectF bound; // set when we paint
};

// draws a line between all the points
class WWLine : public WorkoutWidgetItem {

    public:

        WWLine(WorkoutWidget *w) : WorkoutWidgetItem(w) { w->addItem(this); }

        // Reimplement in children
        int type() { return GCWW_LINE; }

        void paint(QPainter *painter);

        // locate me on the parent widget in paint coordinates
        QRectF bounding() { return QRectF(); }

    private:

};

// draws all the telemetry
class WWTelemetry : public WorkoutWidgetItem {

    public:

        WWTelemetry(WorkoutWidget *w, Context *c) : WorkoutWidgetItem(w), context(c) { w->addItem(this); }

        // Reimplement in children
        int type() { return GCWW_TELEMETRY; }

        void paint(QPainter *painter);

        // locate me on the parent widget in paint coordinates
        QRectF bounding() { return QRectF(); }

    private:
        Context *context;

        template<typename T>
        void updateAvg(const QList<T> &source, QList<double> &avg, int number)
        {
            // Calc moving average
            if (source.count() > avg.count())
            {
                for (int j=avg.count(); j<source.count(); j++)
                {
                    double temp = 0;
                    for (int i=0; i< qMin(number, j); i++)
                    {
                        temp += source.at(j-i);
                    }
                    avg.append(temp/qMin(number, j));
                }
            }
        }

        template<typename T>
        void paintSampleList(QPainter* painter, const QColor& color, const QList<T>& list,
                             const WorkoutWidget::WwSeriesType& seriesType)
        {
            QPen linePen(color);
            double width = appsettings->value(workoutWidget(), GC_LINEWIDTH, 1.0*dpiXFactor).toDouble();
            linePen.setWidth(width);
            painter->setPen(linePen);

            int index = 0;
            QPointF last;
            foreach (T sample, list)
            {
                double now = workoutWidget()->sampleTimes.at(index) / 1000.0f;
                if (!index) {
                    last = workoutWidget()->transform(now, sample, seriesType);
                }
                else {
                    // join the dots
                    QPointF here = workoutWidget()->transform(now, sample, seriesType);
                    painter->drawLine(last, here);
                    last = here;
                }

                index++;
            }
        }
};

// draws a selection rectangle
class WWRect : public WorkoutWidgetItem {

    public:

        WWRect(WorkoutWidget *w) : WorkoutWidgetItem(w) { w->addItem(this); }

        // Reimplement in children
        int type() { return GCWW_RECT; }

        void paint(QPainter *painter);

        // locate me on the parent widget in paint coordinates
        QRectF bounding() { return QRectF(workoutWidget()->onRect,
                                          workoutWidget()->atRect); }

    private:

};

// flash up guide lines when things align when the user is creating
// moving or dragging points or blocks around
class WWSmartGuide : public WorkoutWidgetItem {

    public:

        WWSmartGuide(WorkoutWidget *w) : WorkoutWidgetItem(w) { w->addItem(this); }

        // Reimplement in children
        int type() { return GCWW_SGUIDE; }

        void paint(QPainter *painter);

        // locate me on the parent widget in paint coordinates
        // should not be selectable (!!)
        QRectF bounding() { return QRectF(); }
};

// highlight as we hover over blocks
class WWBlockCursor : public WorkoutWidgetItem {

    public:

        WWBlockCursor(WorkoutWidget *w) : WorkoutWidgetItem(w) { w->addItem(this); }

        // Reimplement in children
        int type() { return GCWW_BCURSOR; }

        void paint(QPainter *painter);

        // locate me on the parent widget in paint coordinates
        QRectF bounding() { return workoutWidget()->cursorBlock.boundingRect(); }

    private:
};

class WWBlockSelection : public WorkoutWidgetItem {

    public:

        WWBlockSelection(WorkoutWidget *w) : WorkoutWidgetItem(w) { w->addItem(this); }

        // Reimplement in children
        int type() { return GCWW_BRECT; }

        void paint(QPainter *painter);

        // locate me on the parent widget in paint coordinates
        QRectF bounding();
};

// draws the W'bal curve
class WWWBLine : public WorkoutWidgetItem {

    public:

        WWWBLine(WorkoutWidget *w, Context *context) : WorkoutWidgetItem(w), context(context) { w->addItem(this); }

        // Reimplement in children
        int type() { return GCWW_WBLINE; }

        void paint(QPainter *painter);

        // locate me on the parent widget in paint coordinates
        QRectF bounding() { return QRectF(); }

    private:
        Context *context;
};

// draws the NOW bar when recording
class WWNow : public WorkoutWidgetItem {

    public:

        WWNow(WorkoutWidget *w, Context *context) : WorkoutWidgetItem(w), context(context) { w->addItem(this); }

        // Reimplement in children
        int type() { return GCWW_NOW; }

        void paint(QPainter *painter);

        // locate me on the parent widget in paint coordinates
        QRectF bounding() { return QRectF(); }

    private:
        Context *context;
};

//
// COMMANDS
//

class CreatePointCommand : public WorkoutWidgetCommand
{
    public:
        CreatePointCommand(WorkoutWidget *w, double x, double y, int index);
        void redo();
        void undo();

    private:

        //state info
        double x,y;
        int index;
};

class CreateBlockCommand : public WorkoutWidgetCommand
{
    public:
        CreateBlockCommand(WorkoutWidget *w, QList<PointMemento>&points);
        void redo();
        void undo();

    private:

        //state info
        QList<PointMemento> points;
};


class MovePointCommand : public WorkoutWidgetCommand
{
    public:
        MovePointCommand(WorkoutWidget *w, QPointF before, QPointF after, int index);
        void redo();
        void undo();

    private:

        //state info
        QPointF before, after;
        int index;
};

class MovePointsCommand : public WorkoutWidgetCommand
{
    public:
        MovePointsCommand(WorkoutWidget *w, QList<PointMemento> before, QList<PointMemento> after);
        void redo();
        void undo();

    private:

        QList<PointMemento> before, after;
};

class ScaleCommand : public WorkoutWidgetCommand
{
    public:
        ScaleCommand(WorkoutWidget *w, double up, double down, bool scaleup);

        void redo();
        void undo();

    private:
        double up, down;
        bool scaleup;
};

class DeleteWPointsCommand : public WorkoutWidgetCommand
{
    public:

        DeleteWPointsCommand(WorkoutWidget*w, QList<PointMemento>points);

        void redo();
        void undo();

    private:
        QList<PointMemento> points;
};

class CutCommand : public WorkoutWidgetCommand
{
    public:

        CutCommand(WorkoutWidget*w, QList<PointMemento> copyIndexes, QList<PointMemento> deleteIndexes, double shift);

        void redo();
        void undo();

    private:
        QList<PointMemento> copyIndexes, deleteIndexes;
        double shift;
};

class PasteCommand : public WorkoutWidgetCommand
{
    public:

        PasteCommand(WorkoutWidget*w, int here, double offset, double shift, QList<PointMemento> points);

        void redo();
        void undo();

    private:
        int here;
        double offset, shift;
        QList<PointMemento> points;
};

class QWKCommand : public WorkoutWidgetCommand
{
    public:

        QWKCommand(WorkoutWidget *w, QString before, QString after);

        void redo();
        void undo();

    private:
        QString before, after;
};

#endif // _GC_WorkoutWidgetItems_h
