/*
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
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

#include "GProgressDialog.h"

GProgressDialog::GProgressDialog(QString title, int min, int max, bool modal, QWidget *parent) : 

    // sheet on mac and no window manager chrome
    QDialog(modal ? parent : NULL, Qt::Sheet | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint),

    // defaults
    title(title), min(min), max(max),

    // moving about
    dragState(None)
{
    // only block mainwindow
    if (modal) setWindowModality(Qt::WindowModal); // only block mainwindow

    // zap me when closed and make me see through
    setAttribute(Qt::WA_DeleteOnClose, true);
    setAttribute(Qt::WA_TranslucentBackground, true);

    // not too big dude
    setFixedSize(200*dpiXFactor, 130*dpiYFactor);

    // trap mouse events
    setMouseTracking(true);

    // watch for ESC key
    installEventFilter(this);
}

bool
GProgressDialog::eventFilter(QObject *o, QEvent *e)
{
    // hit escape and we minimise -- but not reject !
    if (o == this && e->type() == QEvent::KeyPress && static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape) {
        showMinimized(); // will only work after first launch when we have a parent
        return true;
    }
    return false;
}

void GProgressDialog::paintEvent(QPaintEvent *)
{
    // setup the colors
    QColor translucentGray = GColor(CPLOTMARKER);
    translucentGray.setAlpha(240);
    QColor translucentWhite = GColor(CPLOTBACKGROUND);
    translucentWhite.setAlpha(240);
    QColor black = GInvertColor(translucentWhite);

    // setup a painter and the area to paint
    QPainter painter(this);

    painter.save();
    QRect all(0,0,width(),height());

    // fill
    painter.setPen(Qt::NoPen);
    painter.fillRect(all, translucentWhite);

    // border
    QRect border(0,0,width()-1,height()-1);
    black.setAlpha(50);
    painter.setPen(QPen(black, 0.5f));
    painter.drawRect(border);

    // heading background and text
    QRectF titlebox(0,0,width(),25);
    QLinearGradient active = GCColor::instance()->linearGradient(25, true, false);
    painter.fillRect(titlebox, QBrush(Qt::white));
    painter.fillRect(titlebox, active);

    QString chrome = appsettings->value(this, GC_CHROME, "Mac").toString();
    if (chrome == "Mac") {
        painter.setPen(QPen(Qt::black));
    } else {
        black.setAlpha(240);
        painter.setPen(QPen(black));
    }
    painter.drawText(titlebox, title, Qt::AlignVCenter | Qt::AlignCenter);

    // informative text
    black.setAlpha(240);
    painter.setPen(QPen(black));
    QRectF labelbox(0,25,width(),height()-33);
    painter.drawText(labelbox, text, Qt::AlignVCenter | Qt::AlignCenter);

    // progressbar
    QRectF progress(0, height()-8, double(value) / double(max) * double(width()), 8);
    painter.fillRect(progress, translucentGray);
    painter.restore();
}

void GProgressDialog::setValue(int x)
{
    value = x;
    if (max < x) max = x;
    repaint();
}

// set the progress text
void GProgressDialog::setLabelText(QString label)
{
    text = label;
    repaint();
}

void
GProgressDialog::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::NoButton || isHidden()) {
        setDragState(None);
        return;
    }

    setDragState(Move);

    // get current window state
    oX = pos().x();
    oY = pos().y();
    mX = e->globalX();
    mY = e->globalY();
}

void
GProgressDialog::mouseReleaseEvent(QMouseEvent *)
{
    setDragState(None);
    repaint();
}

void
GProgressDialog::mouseMoveEvent(QMouseEvent *e)
{
    if (dragState == None) {
        return;
    }

    // work out the relative move x and y
    int relx = e->globalX() - mX;
    int rely = e->globalY() - mY;

    move(oX + relx, oY + rely);
}

void
GProgressDialog::setDragState(DragState d)
{
    dragState = d;
}

