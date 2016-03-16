/*
 * Copyright (c) 2015 Mark Liversedge(liversedge@gmail.com)
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

#ifndef _GC_SolverDisplay_h
#define _GC_SolverDisplay_h
#include "GoldenCheetah.h"

#include <QWidget>
#include <QDialog>
#include <QLabel>
#include <QVector>
#include <QList>

#include "Context.h"
#include "CPSolver.h"

struct SolverPoint {
    public:
    SolverPoint(int x, int y, double z, int t) : x(x), y(y), z(z), t(t) {}
    int x,y;
    double z;
    int t;
};

class SolverDisplay : public QWidget
{
    Q_OBJECT

    public:
        SolverDisplay(QWidget *parent, Context *context);

        void setConstraints(CPSolverConstraints x) { constraints = x; }
        void addPoint(SolverPoint p);
        void reset();

    protected:
        void paintEvent(QPaintEvent *);
        void resizeEvent(QResizeEvent * event);

    private:
        Context *context;
        QVector<QList<SolverPoint> > points;
        CPSolverConstraints constraints;
        long count;
};

#endif
