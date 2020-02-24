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

#include "GenericLegend.h"
#include "GenericPlot.h"

#include "Colors.h"
#include "TabView.h"
#include "RideFileCommand.h"
#include "Utils.h"

#include <limits>

GenericLegendItem::GenericLegendItem(Context *context, QWidget *parent, QString name, QColor color) :
    QWidget(parent), context(context), name(name), color(color)
{

    value=0;
    hasvalue=false;

    // set height and width, gets reset when configchanges
    configChanged(0);

    // watch for changes...
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

}


void
GenericLegendItem::configChanged(qint32)
{
    static const double gl_margin = 3 * dpiXFactor;
    static const double gl_spacer = 3 * dpiXFactor;
    static const double gl_block = 7 * dpiXFactor;
    static const double gl_linewidth = 1 * dpiXFactor;

    // we just set geometry for now.
    QFont f; // based on what just got set in prefs
    QFontMetricsF fm(f);

    // so now the string we would display
    QString valuelabel = QString ("%1").arg("9999999.999"); // xxx later we might have scale/dp

    // maximum width of widget = margin + block + space + name + space + value + margin
    double width = gl_margin + gl_block + gl_spacer + fm.boundingRect(name).width()
                              + gl_spacer + fm.boundingRect(valuelabel).width() + gl_margin;

    // maximum height of widget = margin + textheight + spacer + line
    double height = gl_margin + fm.boundingRect(valuelabel).height() + gl_spacer + gl_linewidth;

    // now set geometry of widget
    setFixedWidth(width);
    setFixedHeight(height);

    // calculate all the rects used by the painter now since static
    blockrect = QRectF(gl_margin, gl_margin, gl_block, height-gl_margin);
    linerect = QRectF(gl_margin+gl_block, height-gl_linewidth, width-gl_margin, gl_linewidth);
    namerect = QRectF(gl_margin + gl_block + gl_spacer, gl_margin, fm.boundingRect(name).width(), fm.boundingRect(name).height());
    valuerect =QRectF(namerect.x() + namerect.width() + gl_spacer, gl_margin, fm.boundingRect(valuelabel).width(), fm.boundingRect(valuelabel).height());


    // redraw
    update();
}

void
GenericLegendItem::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.save();

    // fill background first
    painter.setBrush(QBrush(GColor(CPLOTBACKGROUND)));
    painter.setPen(Qt::NoPen);
    painter.drawRect(0,0,geometry().width()-1, geometry().height()-1);

    // block and line
    painter.setBrush(QBrush(color));
    painter.setPen(Qt::NoPen);
    //painter.drawRect(blockrect);
    painter.drawRect(linerect);

    // just paint the value for now
    QString string;
    if (hasvalue) string=QString("%1").arg(value, 0, 'f', 2);
    else string="   ";

    // remove redundat dps (e.g. trailing zeroes)
    string = Utils::removeDP(string);

    // set pen to series color for now
    painter.setPen(GCColor::invertColor(GColor(CPLOTBACKGROUND))); // use invert - usually black or white
    painter.setFont(QFont());

    // series
    painter.drawText(namerect, name, Qt::AlignHCenter|Qt::AlignVCenter);
    painter.drawText(valuerect, string, Qt::AlignHCenter|Qt::AlignVCenter);
    painter.restore();

}

GenericLegend::GenericLegend(Context *context, GenericPlot *plot) : context(context), plot(plot)
{
    layout = new QHBoxLayout(this);
    layout->addStretch();

    xname="";

}

void
GenericLegend::addSeries(QString name, QAbstractSeries *series)
{
    // if it already exists remove it
    if (items.value(name,NULL) != NULL) removeSeries(name);

    GenericLegendItem *add = new GenericLegendItem(context, this, name, GenericPlot::seriesColor(series));
    layout->insertWidget(0, add);
    items.insert(name,add);

    // lets see ya!
    add->show();
}

void
GenericLegend::addX(QString name)
{
    // if it already exists remove it
    if (items.value(name,NULL) != NULL) removeSeries(name);

    GenericLegendItem *add = new GenericLegendItem(context, this, name, GColor(CPLOTMARKER));
    layout->insertWidget(0, add);
    items.insert(name,add);

    // lets see ya!
    add->show();

    // remember the x axis
    xname = name;
}

void
GenericLegend::removeSeries(QString name)
{
    GenericLegendItem *remove = items.value(name, NULL);
    if (remove) {
        layout->removeWidget(remove);
        items.remove(name);
        delete remove;
    }
}

void
GenericLegend::removeAllSeries()
{
    xname = "";
    QMapIterator<QString, GenericLegendItem*> i(items);
    while (i.hasNext()) {
        i.next();
        removeSeries(i.key());
    }
}

void
GenericLegend::hover(QPointF value, QString name, QAbstractSeries*)
{
    GenericLegendItem *call = items.value(name, NULL);
    if (call) call->setValue(value.y());

    // xaxis
    GenericLegendItem *xaxis = items.value(xname, NULL);
    if (xaxis) xaxis->setValue(value.x());
}

void
GenericLegend::unhover(QString name)
{
    GenericLegendItem *xaxis = items.value(name,NULL);
    if (xaxis) xaxis->noValue();
}

void
GenericLegend::unhoverx()
{
    if (xname != "") {
        GenericLegendItem *xaxis = items.value(xname, NULL);
        if (xaxis) xaxis->noValue();
    }
}
