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

#include "RatingWidget.h"

#include <QDebug>
#include <QFontMetrics>

#define STAR_FILLED 0x2605
#define STAR_EMPTY 0x2606


RatingWidget::RatingWidget
(QWidget *parent)
: QLabel(parent), rating(0), mouseOverRating(0)
{
    setMouseTracking(true);
}


RatingWidget::~RatingWidget()
{
}


void
RatingWidget::setRating
(int rating)
{
    this->rating = rating;
    showRating(rating);
}


void
RatingWidget::leaveEvent
(QEvent *event)
{
    mouseOverRating = 0;
    showRating(rating);

    event->accept();
}


void
RatingWidget::mouseMoveEvent
(QMouseEvent *event)
{
    QMargins cm = contentsMargins();
    Qt::Alignment a = alignment();
    int leftOffset = 0;
    static const QString fullS(5, QChar(STAR_FILLED));
    QFontMetrics fontMetrics(font(), this);
    QRect fullRect = fontMetrics.boundingRect(fullS);
    if (a & Qt::AlignHCenter) {
        leftOffset = cm.left() + (width() - cm.left() - cm.right() - fullRect.width()) / 2;
    } else if (a & Qt::AlignRight) {
        leftOffset = width() - cm.right() - fullRect.width();
    } else {
        leftOffset = cm.left();
    }
    QString s;
    mouseOverRating = 5;
    for (int i = 1; i <= 5; ++i) {
        s.append(QChar(STAR_FILLED));
        QRect starRect = fontMetrics.boundingRect(s);
        int ratingWidth = starRect.width() + leftOffset;
        if (event->position().x() < ratingWidth) {
            mouseOverRating = i;
            break;
        }
    }
    showRating(mouseOverRating);

    event->accept();
}


void
RatingWidget::mouseReleaseEvent
(QMouseEvent *event)
{
    setRating(mouseOverRating);
    mouseOverRating = 0;
    emit rated(rating);

    event->accept();
}


void
RatingWidget::showRating
(int r)
{
    QString ratingStr;
    ratingStr.fill(QChar(STAR_FILLED), r);
    ratingStr.resize(5, QChar(STAR_EMPTY));
    setText(ratingStr);
}
