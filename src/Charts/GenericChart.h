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

#ifndef _GC_GenericChart_h
#define _GC_GenericChart_h 1

#include "GoldenCheetah.h"
#include "Settings.h"
#include "Context.h"
#include "Athlete.h"
#include "Colors.h"
#include "RCanvas.h"

#include "GenericSelectTool.h"
#include "GenericLegend.h"
#include "GenericPlot.h"

// the chart
class GenericChart : public QWidget {

    Q_OBJECT

    public:

        friend class GenericSelectTool;
        friend class GenericLegend;

        GenericChart(QWidget *parent, Context *context);

    public slots:

        // set chart settings
        bool initialiseChart(QString title, int type, bool animate, int legendpos, bool stack, int orientation);

        // add a curve, associating an axis
        bool addCurve(QString name, QVector<double> xseries, QVector<double> yseries, QString xname, QString yname,
                      QStringList labels, QStringList colors,
                      int line, int symbol, int size, QString color, int opacity, bool opengl);

        // configure axis, after curves added
        bool configureAxis(QString name, bool visible, int align, double min, double max,
                      int type, QString labelcolor, QString color, bool log, QStringList categories);

        // post processing clean up / add decorations / helpers etc
        void finaliseChart();

    protected:

        // legend and selector need acces to these
        QVBoxLayout *mainLayout;
        GenericPlot *plot; // for now...

        // layout options
        bool stack; // stack series instead of on same chart
        Qt::Orientation orientation; // layout horizontal or vertical

    private:
        Context *context;
};


#endif
