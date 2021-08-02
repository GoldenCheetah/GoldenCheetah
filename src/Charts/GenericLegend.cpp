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

GenericLegendItem::GenericLegendItem(Context *context, GenericLegend *parent, QString name, QColor color) :
    QWidget(parent), context(context), name(name), color(color), legend(parent), datetime(false)
{

    value=0;
    enabled=true;
    clickable=true;
    hasvalue=false;

    // set height and width, gets reset when configchanges
    configChanged(0);

    // we want to track our own events - for hover and click
    installEventFilter(this);
    setMouseTracking(true);

    // watch for changes...
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

}


void
GenericLegendItem::configChanged(qint32)
{
    static const double gl_margin = 5 * dpiXFactor*legend->plot()->scale();
    static const double gl_spacer = 2 * dpiXFactor*legend->plot()->scale();
    static const double gl_block = 7 * dpiXFactor*legend->plot()->scale();
    static const double gl_linewidth = 1 * dpiXFactor*legend->plot()->scale();

    // we just set geometry for now.
    QFont f; // based on what just got set in prefs
    // we need to scale...
    f.setPointSizeF(f.pointSizeF() * legend->plot()->scale());
    QFontMetricsF fm(f);

    // so now the string we would display
    QString valuelabel = QString ("%1").arg("9999999.999"); // xxx later we might have scale/dp

    // maximum width of widget = margin + block + space + name + space + value + margin
    double width = gl_margin + gl_block + gl_spacer + fm.boundingRect(name).width()
                              + gl_spacer + fm.boundingRect(valuelabel).width() + gl_margin;

    // maximum height of widget = margin + textheight + spacer + line
    double height = (gl_margin*2) + fm.boundingRect(valuelabel).height() + gl_spacer + gl_linewidth;

    // now set geometry of widget
    setFixedWidth(width);
    setFixedHeight(height);

    // calculate all the rects used by the painter now since static
    blockrect = QRectF(gl_margin, gl_margin, gl_block, height-gl_margin);
    linerect = QRectF(gl_margin+gl_block, gl_spacer+height-gl_linewidth-(gl_margin*2), width-gl_block-(gl_margin*2), gl_linewidth);
    namerect = QRectF(gl_margin + gl_block + gl_spacer, gl_margin, fm.boundingRect(name).width(), fm.boundingRect(name).height());
    valuerect =QRectF(namerect.x() + namerect.width() + gl_spacer, gl_margin, fm.boundingRect(valuelabel).width(), fm.boundingRect(valuelabel).height());
    hoverrect = QRectF(gl_block, 0, width-gl_block,height);


    // redraw
    update();
}

bool
GenericLegendItem::eventFilter(QObject *obj, QEvent *e)
{
    if (obj != this) return false;

    switch (e->type()) {
    case QEvent::MouseButtonRelease: // for now just one event, but may do more later
        {
            if (clickable && underMouse()) {
                enabled=!enabled;
                if (!enabled) hasvalue=false;
                emit clicked(name, enabled);
            }
        }

    case QEvent::Enter:
    case QEvent::Leave:
        // hover indicator show/hide as mouse hovers over the item
        update();
        break;
    }
    return false;
}

void
GenericLegendItem::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.save();

    // fill background first
    painter.setBrush(QBrush(legend->plot()->backgroundColor()));
    painter.setPen(Qt::NoPen);
    painter.drawRect(0,0,geometry().width()-1, geometry().height()-1);

    // under mouse show
    if (clickable && underMouse()) {
        QColor color = GColor(CPLOTMARKER);
        color.setAlphaF(0.2); // same as plotarea
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawRect(hoverrect);
    }

    // block and line - gray means disabled
    if (enabled) painter.setBrush(QBrush(color));
    else painter.setBrush(QBrush(Qt::gray));
    painter.setPen(Qt::NoPen);
    //painter.drawRect(blockrect);
    painter.drawRect(linerect);

    // just paint the value for now
    QString string;
    if (hasvalue) {
        if (datetime) string=QDateTime::fromMSecsSinceEpoch(value).toString(datetimeformat);
        else string=QString("%1").arg(value, 0, 'f', 2);
    } else string="   ";

    // remove redundat dps (e.g. trailing zeroes)
    string = Utils::removeDP(string);

    // set pen to series color for now
    if (enabled)  painter.setPen(GCColor::invertColor(legend->plot()->backgroundColor())); // use invert - usually black or white
    else painter.setPen(Qt::gray);

    QFont f;
    f.setPointSizeF(f.pointSize() * legend->plot()->scale());
    painter.setFont(f);

    // series
    painter.drawText(namerect, Qt::TextSingleLine, name);
    painter.drawText(valuerect, string, Qt::AlignHCenter|Qt::AlignVCenter);
    painter.restore();

}

GenericLegend::GenericLegend(Context *context, GenericPlot *plot) : context(context), plot_(plot)
{
    layout = new QHBoxLayout(this);
    layout->addStretch();

    orientation=Qt::Horizontal;
    xname="";
    clickable=true;
    setAutoFillBackground(false);

    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    configChanged(0);
}


void
GenericLegend::configChanged(qint32)
{
    QPalette pal;
    pal.setBrush(backgroundRole(), plot()->backgroundColor());
    setPalette(pal);
    repaint();
}

void
GenericLegend::addSeries(QString name, QColor color)
{
    // if it already exists remove it
    if (items.value(name,NULL) != NULL) removeSeries(name);

    GenericLegendItem *add = new GenericLegendItem(context, this, name, color);
    add->setClickable(clickable);
    layout->insertWidget(0, add);
    items.insert(name,add);

    // lets see ya!
    add->show();

    // connect signals
    connect(add, SIGNAL(clicked(QString,bool)), this, SIGNAL(clicked(QString,bool)));
}

void
GenericLegend::setOrientation(Qt::Orientation o)
{
    if (orientation == o) return; // already is.

    // set new orientation
    orientation = o;

    QBoxLayout *newlayout;
    if (orientation == Qt::Horizontal)  newlayout = new QHBoxLayout();
    else newlayout = new QVBoxLayout();
    newlayout->addStretch();

    // remove from layout and redo
    QMapIterator<QString, GenericLegendItem*> i(items);
    while (i.hasNext()) {
        i.next();
        layout->removeWidget(i.value());
        newlayout->insertWidget(0, i.value());
    }

    // replace the layout
    delete layout;
    setLayout(newlayout);
    layout = newlayout;

}
void
GenericLegend::addX(QString name, bool datetime, QString datetimeformat)
{
    // if it already exists remove it
    if (items.value(name,NULL) != NULL) removeSeries(name);

    GenericLegendItem *add = new GenericLegendItem(context, this, name, GColor(CPLOTMARKER));
    add->setClickable(false);
    add->setDateTime(datetime, datetimeformat);
    layout->insertWidget(0, add);
    items.insert(name,add);

    // lets see ya!
    add->show();

    // remember the x axis
    xname = name;

    // we don't connect -- there is no such series, its a meta legend item
    // NOPE: connect(add, SIGNAL(clicked(QString,bool)), this, SIGNAL(clicked(QString,bool)));
}

void
GenericLegend::addLabel(QLabel *label)
{
    layout->addWidget(label);
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
GenericLegend::setValue(GPointF value, QString name)
{
    GenericLegendItem *call = items.value(name, NULL);
    if (call) call->setValue(value.y());

    // xaxis
    GenericLegendItem *xaxis = items.value(xname, NULL);
    if (xaxis) xaxis->setValue(value.x());
}

void
GenericLegend::setClickable(bool clickable)
{
    this->clickable=clickable;
    QMapIterator<QString, GenericLegendItem*> i(items);
    while (i.hasNext()) {
        i.next();
        if (i.key() != xname)
            i.value()->setClickable(clickable);
    }
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
