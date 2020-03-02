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

#include "GenericChart.h"

#include "Colors.h"
#include "TabView.h"
#include "RideFileCommand.h"
#include "Utils.h"

#include <limits>

GenericChart::GenericChart(QWidget *parent, Context *context) : QWidget(parent), context(context)
{
    // intitialise state info
    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);

    plot = new GenericPlot(this,context);
    mainLayout->addWidget(plot);
}

// set chart settings
bool
GenericChart::initialiseChart(QString title, int type, bool animate, int legendpos, bool stack, int orientation)
{
    // for now ...
    Q_UNUSED(stack)
    Q_UNUSED(orientation)

    return plot->initialiseChart(title, type, animate, legendpos);
}

// add a curve, associating an axis
bool
GenericChart::addCurve(QString name, QVector<double> xseries, QVector<double> yseries, QString xname, QString yname,
                      QStringList labels, QStringList colors,
                      int line, int symbol, int size, QString color, int opacity, bool opengl)
{

    return plot->addCurve(name, xseries, yseries, xname, yname, labels, colors, line, symbol, size, color, opacity, opengl);
}

// configure axis, after curves added
bool
GenericChart::configureAxis(QString name, bool visible, int align, double min, double max,
                      int type, QString labelcolor, QString color, bool log, QStringList categories)
{
    return plot->configureAxis(name, visible, align, min, max, type, labelcolor, color, log, categories);
}

// post processing clean up / add decorations / helpers etc
void
GenericChart::finaliseChart()
{
    plot->finaliseChart();
}
