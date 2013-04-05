/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#include "TreeMapPlot.h"
#include "LTMTool.h"
#include "TreeMapWindow.h"
#include "MetricAggregator.h"
#include "SummaryMetrics.h"
#include "RideMetric.h"
#include "Settings.h"
#include "Colors.h"

#include "StressCalculator.h" // for LTS/STS calculation

#include <QSettings>

#include <math.h> // for isinf() isnan()

// Treemap sorter - reversed to do descending
bool TreeMapLessThan(const TreeMap *a, const TreeMap *b) {
    return (a->value) > (b->value);
}

TreeMapPlot::TreeMapPlot(TreeMapWindow *parent, MainWindow *main, QDir home)
            : QWidget (parent), parent(parent), main(main), home(home)
{
    setInstanceName("TreeMap Plot");

    // get application settings
    useMetricUnits = main->useMetricUnits;
    settings = NULL;

    root = new TreeMap;
    setMouseTracking(true);
    installEventFilter(this);

    // no margins
    setContentsMargins(0,0,0,0);

    configUpdate(); // set basic colors
    connect(main, SIGNAL(configChanged()), this, SLOT(configUpdate()));
}

TreeMapPlot::~TreeMapPlot()
{
}

void
TreeMapPlot::configUpdate()
{
    // get application settings
    useMetricUnits = main->useMetricUnits;
}

void
TreeMapPlot::setData(TMSettings *settings)
{
    root->clear();

    foreach (SummaryMetrics rideMetrics, *(settings->data)) {

        // don't plot if filtered
        if (main->isfiltered && !main->filters.contains(rideMetrics.getFileName())) continue;

        double value = rideMetrics.getForSymbol(settings->symbol);
        QString text1 = rideMetrics.getText(settings->field1, "(unknown)");
        QString text2 = rideMetrics.getText(settings->field2, "(unknown)");
        if (text1 == "") text1 = "(unknown)";
        if (text2 == "") text2 = "(unknown)";

        TreeMap *first = root->insert(text1, 0.0);
        first->insert(text2, value);
    }

    // layout and paint
    resizeEvent(NULL);
    repaint();
}

void
TreeMapPlot::resizeEvent(QResizeEvent *)
{
    // layout the map
    if (root) root->layout(QRect(9,9,geometry().width()-18, geometry().height()-18));
}


void
TreeMapPlot::paintEvent(QPaintEvent *)
{
    if (!root) return;

    // labels
    QFont font;

    // Init paint settings
    QPainter painter(this);
    QColor color = Qt::white;
    QPen pen(Qt::white);
    pen.setWidth(10); // root
    QBrush brush(Qt::white);
    painter.setBrush(brush);
    painter.setPen(pen);

    // draw border and background
    painter.drawRect(root->rect.x()+4,
                     root->rect.y()+4,
                     root->rect.width()-8,
                     root->rect.height()-8);

    // first level
    font.setPointSize(18);
    painter.setFont(font);
    pen.setWidth(5);
    pen.setColor(Qt::white);
    painter.setPen(pen);
    color = Qt::black;
    //color.setAlpha(127);

    int n=1;
    QColor cHSV, cRGB;
    double factor = double(1) / double(root->children.count());
    foreach (TreeMap *first, root->children) {

        cHSV.setHsv((double)255 / (factor*n++), 255, 150);
        cRGB = cHSV.convertTo(QColor::Rgb);
        brush.setColor(cRGB);
        painter.setBrush(brush);
        painter.drawRect(first->rect);
        painter.drawText(first->rect,
                         Qt::AlignVCenter|Qt::AlignHCenter,
                         first->name);
    }

    // second level
    font.setPointSize(8);
    painter.setFont(font);
    pen.setWidth(1);
    pen.setColor(Qt::lightGray);
    painter.setPen(pen);
    color = Qt::darkGray;
    color.setAlpha(127);
    brush.setColor(color);
    QColor hcolor(Qt::lightGray);
    hcolor.setAlpha(127);
    QBrush hbrush(hcolor);

    foreach (TreeMap *first, root->children)
        foreach (TreeMap *second, first->children) {
            if (second == highlight) painter.setBrush(hbrush);
            else painter.setBrush(brush);
            painter.setPen(Qt::NoPen);
            painter.drawRect(second->rect.x()+2,
                             second->rect.y()+2,
                             second->rect.width()-4,
                             second->rect.height()-4);
            painter.setPen(pen);
            painter.drawText(second->rect, Qt::AlignTop|Qt::AlignLeft, second->name);
        }
}

bool
TreeMapPlot::eventFilter(QObject *, QEvent *e)
{

    if (e->type() == QEvent::MouseMove) {
       QPoint pos = static_cast<QMouseEvent*>(e)->pos();
       TreeMap *underMouse = NULL;

        // look at the bottom rung.
        foreach (TreeMap *first, root->children)
            if ((underMouse = first->findAt(pos)) != NULL)
                break;

        // if this one isn't the one that is
        // currently highlighted repaint to
        // highlight it!
        if (underMouse && highlight != underMouse) {
            highlight = underMouse;
            repaint();
            return true;
        }

    } else if (e->type() == QEvent::Leave) {
        highlight = NULL;
        repaint();
        return true;

    } else if (e->type() == QEvent::MouseButtonPress) {

        QPoint pos = static_cast<QMouseEvent*>(e)->pos();
        Qt::MouseButton button = static_cast<QMouseEvent*>(e)->button();

        if (button == Qt::LeftButton) {
            TreeMap *underMouse = NULL;

            // look at the bottom rung.
            foreach (TreeMap *first, root->children)
                if ((underMouse = first->findAt(pos)) != NULL)
                    break;

            // got one?
            if (underMouse) {
                emit clicked(underMouse->parent->name, underMouse->name);
            }
        }
    }

    return false;
}
