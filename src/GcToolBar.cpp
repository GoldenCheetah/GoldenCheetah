/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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

#include "GcToolBar.h"

GcToolBar::GcToolBar(QWidget *parent) : QWidget(parent)
{
    setFixedHeight(44);
    setContentsMargins(0,0,0,0);
    layout = new QHBoxLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(0,0,0,0);
    installEventFilter(this);
}

GcToolBar::~GcToolBar()
{
}

void
GcToolBar::addAction(QAction *a)
{
    GcToolButton *button = new GcToolButton(this, a);
    buttons.append(button);
    layout->addWidget(button);
    button->installEventFilter(this);
}

void
GcToolBar::select(int index)
{
    foreach(GcToolButton *p, buttons) p->selected = false;
    buttons[index]->selected = true;
    repaint();
}

void
GcToolBar::addStretch()
{
    layout->addStretch();
}

void
GcToolBar::addWidget(QWidget *x) // add a widget that doesn't toggle selection
{
    layout->addWidget(x);
    x->installEventFilter(this);
}

void
GcToolBar::paintEvent (QPaintEvent *event)
{
    // paint the darn thing!
    paintBackground(event);
}

void
GcToolBar::paintBackground(QPaintEvent *)
{
    QPainter painter(this);

    // get the widget area
    QRect all(0,0,width(),height());

    // fill with a linear gradient
    QLinearGradient linearGradient(0, 0, 0, height());
    linearGradient.setColorAt(0.0, QColor(130,132,138));
    linearGradient.setColorAt(1.0, QColor(83, 85, 91));
    linearGradient.setSpread(QGradient::PadSpread);
    
    painter.setPen(Qt::NoPen);
    painter.fillRect(all, linearGradient);

    // paint the top line
    QPen tPen;
    tPen.setWidth(1);
    tPen.setColor(QColor(105,105,105));
    painter.setPen(tPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(0, 0, width(), 0);

    tPen.setColor(QColor(165, 167, 171));
    painter.setPen(tPen);
    painter.drawLine(0, 1, width(), 1);

    // paint the botton lines
    QPen bPen;
    bPen.setWidth(1);
    bPen.setColor(QColor(215,216,217));
    painter.setPen(bPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(0, height()-1, width(), height()-1);

    bPen.setColor(QColor(33,33,33));
    painter.setPen(bPen);
    painter.drawLine(0, height()-2, width(), height()-2);

    bPen.setColor(QColor(96,98,104));
    painter.setPen(bPen);
    painter.drawLine(0, height()-3, width(), height()-3);
}

bool
GcToolBar::eventFilter(QObject *obj, QEvent *e)
{
    if (e->type() == QEvent::LayoutRequest) {
        layout->activate();
        repaint();
    }

    if (e->type() == QEvent::MouseButtonRelease) {
        // is one of our widgets unfer the mouse
        GcToolButton *activate = NULL;
        foreach(GcToolButton *p, buttons) {
            if (p == obj) {
                activate = p;
                break;
            }
        }

        if (activate) {
            foreach(GcToolButton *p, buttons) p->selected = false;
            activate->selected = true;
            activate->action->activate(QAction::Trigger);
            repaint();
        }
        return true;
    }
    return false;
}

GcToolButton::GcToolButton(QWidget *parent, QAction *action) : QWidget(parent), action(action)
{
    setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *mlayout = new QHBoxLayout(this);
    mlayout->setSpacing(5);
    mlayout->setContentsMargins(10,0,10,0); // spacing either side
    setLayout(mlayout);

    QLabel *iconlabel = new QLabel(action->iconText(), this);
    iconlabel->setPixmap(action->icon().pixmap(18,18)); // titchy
    mlayout->addWidget(iconlabel);

    label = new QLabel(action->iconText(), this);
    mlayout->addWidget(label);

    // quick hack -- default when open is always analysis
    if (action->iconText() == "Analysis") selected = true;
    else selected = false;
}

void
GcToolButton::paintEvent(QPaintEvent *event)
{
    // paint a background first
    paintBackground(event);

    // call parent
    QWidget::paintEvent(event);
}

void
GcToolButton::paintBackground(QPaintEvent * /* event */)
{
    QPainter painter(this);

    if (selected) {

        // get the widget area
        QRect all(0,0,width(),height());

        // fill with a linear gradient
        QLinearGradient linearGradient(0, 0, 0, height());
        linearGradient.setColorAt(0.0, QColor(0x7A,0x7e,0x84));
        linearGradient.setColorAt(0.3, QColor(0xA0,0xA2,0xA5));
        linearGradient.setColorAt(1.0, QColor(0xB3,0xB4,0xB6));
        linearGradient.setSpread(QGradient::PadSpread);
    
        painter.setPen(Qt::NoPen);
        painter.fillRect(all, linearGradient);

        // top line
        QPen tPen;
        tPen.setWidth(1);
        tPen.setColor(QColor(105,105,105));
        painter.setPen(tPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(0, 0, width(), 0);

        tPen.setColor(QColor(88, 91, 95));
        painter.setPen(tPen);
        painter.drawLine(0, 1, width(), 1);

        // left and right lines
        QLinearGradient linear1(0, 0, 0, height());
        linear1.setColorAt(0.0, QColor(0x62,0x65,0x69));
        linear1.setColorAt(1.0, QColor(0x8f,0x90,0x92));
        linear1.setSpread(QGradient::RepeatSpread);
        painter.setPen(Qt::NoPen);
        painter.fillRect(QRect(2, 1, 2, height()-3), linear1);
        painter.fillRect(QRect(width()-3, 1, width()-2, height()-3), linear1);

        QLinearGradient linear2(0, 0, 0, height());
        linear2.setColorAt(0.0, QColor(0x49,0x4b,0x4e));
        linear2.setColorAt(1.0, QColor(0x6b,0x6c,0x6d));
        linear2.setSpread(QGradient::RepeatSpread);
        painter.setPen(Qt::NoPen);
        painter.fillRect(QRect(1, 1, 1, height()-2), linear2);
        painter.fillRect(QRect(width()-2, 1, width()-1, height()-2), linear2);

        tPen.setColor(QColor(159,160,164));
        painter.setPen(tPen);
        painter.drawLine(0, 1, 0, height()-3);
        painter.drawLine(width()-1, 1, width()-1, height()-3);

        QPalette pal;
        pal.setColor(QPalette::WindowText, QColor(0,0,0,200));
        label->setPalette(pal);
    } else {

        QPalette pal;
        pal.setColor(QPalette::WindowText, QColor(255,255,255,200));
        label->setPalette(pal);
    }
}
