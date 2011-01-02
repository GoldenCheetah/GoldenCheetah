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

#include "GoldenCheetah.h"
#include "Colors.h"
#include <QDebug>
#include <QPainter>
#include <QPixmap>

QWidget *GcWindow::controls() const
{
    return _controls;
}

void GcWindow::setControls(QWidget *x)
{
    _controls = x;
    emit controlsChanged(_controls);
}

QString GcWindow::instanceName() const
{
    return _instanceName;
}

void GcWindow::_setInstanceName(QString x)
{
    _instanceName = x;
}

QString GcWindow::title() const
{
    return _title;
}

void GcWindow::setTitle(QString x)
{
    _title = x;
    emit titleChanged(_title);
}

RideItem* GcWindow::rideItem() const
{
    return _rideItem;
}

void GcWindow::setRideItem(RideItem* x)
{
    _rideItem = x;
    emit rideItemChanged(_rideItem);
}

int GcWindow::widthFactor() const
{
    return _widthFactor;
}

void GcWindow::setWidthFactor(int x)
{
    _widthFactor = x;
    emit widthFactorChanged(x);
}

int  GcWindow::heightFactor() const
{
    return _heightFactor;
}

void GcWindow::setHeightFactor(int x)
{
    _heightFactor = x;
    emit heightFactorChanged(x);
}

GcWindow::GcWindow()
{
}

GcWindow::GcWindow(QWidget *parent) : QFrame(parent) {
    qRegisterMetaType<QWidget*>("controls");
    qRegisterMetaType<RideItem*>("ride");
    qRegisterMetaType<GcWinID>("type");
    setParent(parent);
    setControls(NULL);
    setRideItem(NULL);
    setTitle("");
    setContentsMargins(0,0,0,0);
}

GcWindow::~GcWindow()
{
    //qDebug()<<"deleting.."<<property("title").toString()<<property("instanceName").toString()<<_type;
}

bool
GcWindow::amVisible()
{
    // if we're hidden then say false!
    if (isHidden()) return false;
    return true; // XXX need to sort this!!
}



void
GcWindow::paintEvent(QPaintEvent * /*event*/)
{
    static QPixmap closeImage = QPixmap(":images/toolbar/popbutton.png");
    static QPixmap aluBar = QPixmap(":images/aluBar.png");
    static QPixmap aluBarDark = QPixmap(":images/aluBarDark.png");
    static QPixmap aluLight = QPixmap(":images/aluLight.jpg");
    static QPixmap carbon = QPixmap(":images/carbon.jpg");

    if (contentsMargins().top() > 0) {
        // draw a rectangle in the contents margins
        // at the top of the widget

        // setup a painter and the area to paint
        QPainter painter(this);

        // background light gray for now?
        QRect all(0,0,width(),height());
        painter.drawTiledPixmap(all, aluLight);

        // fill in the title bar
        QRect bar(0,0,width(),contentsMargins().top());
        QColor bg;
        if (property("active").toBool() == true) {
            bg = GColor(CTILEBARSELECT);
            painter.drawPixmap(bar, aluBarDark);
        } else {
            bg = GColor(CTILEBAR);
            painter.drawPixmap(bar, aluBar);
        }


        // heading
        QFont font;
        font.setPointSize((contentsMargins().top()/2)+2);
        font.setWeight(QFont::Bold);
        QString title = property("title").toString();
        painter.setFont(font);
        painter.drawText(bar, title, Qt::AlignVCenter | Qt::AlignCenter);

        // border
        painter.setBrush(Qt::NoBrush);
        if (underMouse()) {
            QPixmap sized = closeImage.scaled(QSize(contentsMargins().top(),
                                                    contentsMargins().top()));
            painter.setPen(Qt::black);
            painter.drawRect(QRect(0,0,width()-1,height()-1));
            painter.drawPixmap(width()-sized.width(), 0, sized.width(), sized.height(), sized);
        } else {
            painter.setPen(bg);
            painter.drawRect(QRect(0,0,width()-1,height()-1));
        }
    } else {
        // is this a layout manager?
        // background light gray for now?
        QPainter painter(this);
        QRect all(0,0,width(),height());
        if (property("isManager").toBool() == true) {
            painter.drawTiledPixmap(all, carbon);
        } else {
            painter.drawTiledPixmap(all, aluLight);
        }
    }
}
