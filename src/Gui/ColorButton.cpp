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

#include "ColorButton.h"
#include "Colors.h"

#include <QPainter>
#include <QColorDialog>

ColorButton::ColorButton(QWidget *parent, QString name, QColor color) : QPushButton("", parent), color(color), name(name)
{
#if defined(WIN32) || defined (Q_OS_LINUX)
    // are we in hidpi mode? if so undo global defaults for toolbar pushbuttons
    if (dpiXFactor > 1) {
        QString nopad = QString("QPushButton { padding-left: 0px; padding-right: 0px; "
                                "              padding-top:  0px; padding-bottom: 0px; }");
        setStyleSheet(nopad);
    }
#endif
    setColor(color);
    connect(this, SIGNAL(clicked()), this, SLOT(clicked()));

};

void
ColorButton::setColor(QColor ncolor)
{
    color = ncolor;

    setFlat(true);
    QPixmap pix(20*dpiXFactor, 20*dpiYFactor);
    QPainter painter(&pix);
    if (color.isValid()) {
        painter.setPen(Qt::gray);
        painter.setBrush(QBrush(color));
        painter.drawRect(0, 0, 20*dpiXFactor, 20*dpiYFactor);
    }
    QIcon icon;
    icon.addPixmap(pix);
    setIcon(icon);
    setIconSize(QSize(20*dpiXFactor, 20*dpiYFactor));
    setContentsMargins(2*dpiXFactor,2*dpiYFactor,2*dpiXFactor,2*dpiYFactor);
    setFixedSize(24 * dpiXFactor, 24 * dpiYFactor);
}

void
ColorButton::clicked()
{
    // Color picker dialog
    QColorDialog picker(this);
    picker.setCurrentColor(color);
    QColor rcolor = picker.getColor(color, this, tr("Choose Color"), QColorDialog::DontUseNativeDialog);

    // if we got a good color use it and notify others
    if (rcolor.isValid()) {
        setColor(rcolor);
        colorChosen(color);
    }
}
