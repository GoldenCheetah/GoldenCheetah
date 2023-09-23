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

#include "PowerInfoWidget.h"

#include <QDebug>
#include <QPainter>
#include <QFontMetrics>
#include <QRect>

#include "Colors.h"


PowerInfoWidget::PowerInfoWidget
(QWidget *parent)
: QWidget(parent), minPower(0), maxPower(0), avgPower(0), isoPower(0), xPower(0), skiba(false)
{
    setContentsMargins(0, 0, 0, 0);
    setCursor(Qt::PointingHandCursor);
}


PowerInfoWidget::~PowerInfoWidget()
{
}


void
PowerInfoWidget::setPower
(int minPower, int maxPower, int avgPower, int isoPower, int xPower)
{
    this->minPower = minPower;
    this->maxPower = maxPower;
    this->avgPower = avgPower;
    this->isoPower = isoPower;
    this->xPower = xPower;

    powerRange = maxPower - minPower;
    avgPercent = (avgPower - minPower) / double(powerRange);
    isoPercent = (isoPower - minPower) / double(powerRange);
    xPercent = (xPower - minPower) / double(powerRange);

    repaint();
}


void
PowerInfoWidget::setContentsMargins
(int left, int top, int right, int bottom)
{
    QWidget::setContentsMargins(left, top, right, bottom);
    adjustHeight();
}


void
PowerInfoWidget::setContentsMargins
(const QMargins &margins)
{
    setContentsMargins(margins.left(), margins.top(), margins.right(), margins.bottom());
}


void
PowerInfoWidget::paintEvent
(QPaintEvent *event)
{
    if (minPower >= 0 && maxPower > 0) {
        QColor bgColor = GColor(CTRAINPLOTBACKGROUND);
        QColor fgColor = GCColor::invertColor(bgColor);
        QColor barColor = GColor(CPOWER);
        QFontMetrics fontMetrics(font(), this);
        int rowHeight = fontMetrics.height();

        QRect drawArea = contentsRect();

        QRect minMaxPowerRect = fontMetrics.boundingRect(QString("%1 W").arg(2000));
        int minMaxPowerLabelWidth = minMaxPowerRect.width();
        int barWidth = drawArea.width();

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setClipRect(drawArea);
        painter.translate(drawArea.topLeft());

        // Bar
        QRect powerBarRect(0, rowTop(rowHeight, 2), barWidth, rowHeight);
        painter.fillRect(powerBarRect, barColor);


        // Markers
        int markerWidth = 4;

        int avgMarkerX = barWidth * avgPercent;
        painter.fillRect(avgMarkerX - markerWidth / 2, rowTop(rowHeight, 1), 4, rowHeight, barColor);

        int weightedMarkerX = barWidth * isoPercent;
        if (skiba) {
            weightedMarkerX = barWidth * xPercent;
        }
        painter.fillRect(weightedMarkerX - markerWidth / 2, rowTop(rowHeight, 3), 4, rowHeight, barColor);


        // Min-Max-Labels
        painter.setPen(QPen(GCColor::invertColor(barColor)));

        QRect minPowerLabelRect(0, rowTop(rowHeight, 2), minMaxPowerLabelWidth, rowHeight);
        painter.drawText(minPowerLabelRect, Qt::AlignCenter, QString("%1 W").arg(minPower));

        QRect maxPowerLabelRect(barWidth - minMaxPowerLabelWidth, rowTop(rowHeight, 2), minMaxPowerLabelWidth, rowHeight);
        painter.drawText(maxPowerLabelRect, Qt::AlignCenter, QString("%1 W").arg(maxPower));


        // Labels
        painter.setPen(fgColor);
        int labelOffset = 5;

        QString avgLabelText = QString("%1 %2 W").arg(tr("Average Power")).arg(avgPower);
        QRect avgLabelRect = fontMetrics.boundingRect(avgLabelText);
        avgLabelRect.setWidth(avgLabelRect.width() + 10); // Workaround for boundingRect reporting too small
        if (avgLabelRect.width() < drawArea.width()) {
            avgLabelRect.moveTo(barWidth * avgPercent + labelOffset, rowTop(rowHeight, 1));
            if (avgLabelRect.left() + avgLabelRect.width() > drawArea.width()) {
                avgLabelRect.translate(drawArea.width() - (avgLabelRect.left() + avgLabelRect.width()), 0);
            }
            painter.drawText(avgLabelRect, Qt::AlignLeft | Qt::AlignVCenter, avgLabelText);
        }

        QString weightedLabel;
        if (skiba) {
            weightedLabel = QString("%1 %2 W").arg(tr("xPower")).arg(xPower);
        } else {
            weightedLabel = QString("%1 %2 W").arg(tr("Iso Power")).arg(isoPower);
        }
        QRect weightedLabelRect = fontMetrics.boundingRect(weightedLabel);
        weightedLabelRect.setWidth(weightedLabelRect.width() + 10); // Workaround for boundingRect reporting too small
        if (weightedLabelRect.width() < drawArea.width()) {
            weightedLabelRect.moveTo(weightedMarkerX + labelOffset, rowTop(rowHeight, 3));
            if (weightedLabelRect.left() + weightedLabelRect.width() > drawArea.width()) {
                weightedLabelRect.translate(drawArea.width() - (weightedLabelRect.left() + weightedLabelRect.width()), 0);
            }
            painter.drawText(weightedLabelRect, Qt::AlignLeft | Qt::AlignVCenter, weightedLabel);
        }

#if 0
        painter.setPen(QColor("black"));
        painter.drawRect(0, 0, width() - 1, height() - 1);
#endif
    }
    event->accept();
}


void
PowerInfoWidget::mouseReleaseEvent
(QMouseEvent*)
{
    skiba = ! skiba;
    repaint();
}


void
PowerInfoWidget::changeEvent
(QEvent *event)
{
    if (event->type() == QEvent::FontChange) {
        adjustHeight();
    }
}


int
PowerInfoWidget::rowTop
(int rowHeight, int row) const
{
    return (row - 1) * rowHeight;
}


void
PowerInfoWidget::adjustHeight
()
{
    QMargins cm = contentsMargins();
    QFontMetrics fontMetrics(font(), this);
    int rowHeight = fontMetrics.height();

    setMinimumSize(100 + cm.left() + cm.right(), rowHeight * 3 + cm.top() + cm.bottom());
    setMaximumSize(100000, rowHeight * 3 + cm.top() + cm.bottom());
}
