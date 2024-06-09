/*
 * Copyright (c) 2023 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#include "Shy.h"

#include <QDebug>
#include <QFontMetrics>
#include <QVBoxLayout>


Shy::Shy
(double amount, ShyType type, QWidget *parent)
: QWidget(parent), collapsed(true), amount(amount), type(type), duration(100)
{
    animation = new QPropertyAnimation(this, "maximumHeight", this);

    QVBoxLayout *l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);

    container = new QWidget();
    l->addWidget(container, Qt::AlignCenter);

    QWidget::setLayout(l);
}


Shy::~Shy()
{
}


void
Shy::setAmount
(double amount)
{
    setAmount(amount, type);
}


void
Shy::setAmount
(double amount, ShyType type)
{
    this->amount = amount;
    this->type = type;
}


void
Shy::setAnimationDuration
(int duration)
{
    this->duration = duration;
}


void
Shy::setLayout
(QLayout *layout)
{
    container->setLayout(layout);

    updateHeight();
}


void
Shy::mouseReleaseEvent
(QMouseEvent* /*event - UNUSED*/)
{
    int contHeight = containerHeight();
    int collHeight = collapsedHeight();

    if (contHeight > collHeight) {
        container->setMinimumHeight(contHeight);
        container->setMaximumHeight(contHeight);

        animation->setDuration(duration);
        animation->setStartValue(collapsedHeight());
        animation->setEndValue(contHeight);

        animation->setDirection(collapsed ? QAbstractAnimation::Forward : QAbstractAnimation::Backward);
        animation->start();

        collapsed = ! collapsed;
    }
}


void
Shy::resizeEvent
(QResizeEvent *event)
{
    if (event->oldSize().width() != event->size().width()) {
        updateHeight();
    }
}


bool
Shy::event(QEvent *event)
{
    if (event->type() == QEvent::LayoutRequest) {
        updateHeight();
    }
    return QWidget::event(event);
}


int
Shy::collapsedHeight
() const
{
    int contHeight = containerHeight();
    int collapsed = 0;
    switch (this->type) {
    case ShyType::pixel:
        collapsed = qMin(contHeight, static_cast<int>(amount));
        break;
    case ShyType::lines:
        collapsed = QFontMetrics(font(), this).height() * amount;
        break;
    case ShyType::percent:
        collapsed = contHeight * amount / 100.0;
        break;
    default:
        qWarning() << "Shy::setAmount: Internal error: Unknown ShyType" << static_cast<int>(this->type);
        collapsed = contHeight;
        break;
    }
    return collapsed;
}


int
Shy::containerHeight
() const
{
    int containerHeight = container->sizeHint().height();
    if (container->hasHeightForWidth()) {
        containerHeight = container->heightForWidth(width());
    }
    return containerHeight;
}


void
Shy::updateHeight
()
{
    int contHeight = containerHeight();
    int collHeight = collapsedHeight();
    container->setMinimumHeight(contHeight);
    container->setMaximumHeight(contHeight);
    if (collapsed) {
        setMaximumHeight(collapsedHeight());
    } else {
        setMaximumHeight(contHeight);
    }
    if (contHeight > collHeight) {
        setCursor(Qt::PointingHandCursor);
    } else {
        unsetCursor();
    }
}
