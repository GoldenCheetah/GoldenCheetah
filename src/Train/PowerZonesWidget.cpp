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

#include "PowerZonesWidget.h"

#include <QDebug>
#include <QPainter>
#include <QFontMetrics>
#include <QPropertyAnimation>

#include "Colors.h"


PowerZonesWidget::PowerZonesWidget
(QList<QColor> colors, QList<QString> names, QWidget *parent)
: QWidget(parent), colors(colors), names(names), zones(), dominantZone(0), duration(0), collapsed(true)
{
    setContentsMargins(0, 0, 0, 0);
    setCursor(Qt::PointingHandCursor);

    toggleAnimation.addAnimation(new QPropertyAnimation(this, "minimumHeight"));
    toggleAnimation.addAnimation(new QPropertyAnimation(this, "maximumHeight"));

    detailsDoc.setUseDesignMetrics(true);
}


PowerZonesWidget::~PowerZonesWidget()
{
}


void
PowerZonesWidget::setContentsMargins
(int left, int top, int right, int bottom)
{
    QWidget::setContentsMargins(left, top, right, bottom);

    detailsDoc.setTextWidth(contentsRect().width());
    adjustHeight();
}


void
PowerZonesWidget::setContentsMargins
(const QMargins &margins)
{
    setContentsMargins(margins.left(), margins.top(), margins.right(), margins.bottom());
}


void
PowerZonesWidget::setPowerZones
(QList<double> zones, int dominantZone, long duration)
{
    this->zones = zones;
    this->dominantZone = dominantZone;
    this->duration = duration;
    fillDetailsDoc();
    repaint();
}


void
PowerZonesWidget::setColors
(QList<QColor> colors)
{
    this->colors = colors;
    fillDetailsDoc();
    repaint();
}


void
PowerZonesWidget::setNames
(QList<QString> names)
{
    this->names = names;
    fillDetailsDoc();
    repaint();
}


void
PowerZonesWidget::paintEvent
(QPaintEvent *event)
{
    if (dominantZone == 0) {
        event->accept();
        return;
    }

    QColor bgColor = GColor(CTRAINPLOTBACKGROUND);
    QColor fgColor = GCColor::invertColor(bgColor);

    QRect drawArea = contentsRect();
    int barWidth = drawArea.width();
    QFontMetrics fontMetrics(font(), this);
    int rowHeight = fontMetrics.height();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setClipRect(drawArea);
    painter.translate(drawArea.topLeft());

    int markerHeight = 5;
    double sumPercentLeft = 0;
    int lastRight = 0;
    for (int i = 0; i < zones.length(); ++i) {
        int zoneLeft = barWidth * sumPercentLeft / 100.0;
        int zoneWidth = barWidth * zones[i] / 100.0;
        if (lastRight != zoneLeft) {
            zoneLeft = lastRight;
            zoneWidth += (zoneLeft - lastRight);
        }
        QRect zoneBarRect(zoneLeft, rowTop(rowHeight, 1), zoneWidth, rowHeight);
        if (i == dominantZone - 1) {
            painter.fillRect(zoneLeft, rowTop(rowHeight, 2) * 1.2, zoneWidth, markerHeight, fgColor);
        }
        painter.fillRect(zoneBarRect, color(i));
        painter.setPen(QPen(GCColor::invertColor(color(i))));
        painter.drawText(zoneBarRect, Qt::AlignCenter, QString("Z%1").arg(i + 1));
        sumPercentLeft += zones[i];
        lastRight = zoneLeft + zoneWidth;
    }
    if (! collapsed || toggleAnimation.state() == QAbstractAnimation::Running) {
        painter.save();
        int docx = qMax(0, int((drawArea.width() - detailsDoc.idealWidth()) / 2));
        painter.translate(docx, 1.5 * rowHeight);
        detailsDoc.setTextWidth(drawArea.width());
        detailsDoc.drawContents(&painter);
        painter.restore();
    }

#if 0
    painter.setPen(QPen(QColor("red"), 5));
    painter.drawRect(0, 0, drawArea.width() - 1, drawArea.height() - 1);
#endif
    event->accept();
}


void
PowerZonesWidget::resizeEvent
(QResizeEvent *event)
{
    if (event->oldSize().width() != event->size().width()) {
        detailsDoc.setTextWidth(contentsRect().width());
        adjustHeight();
    }
    event->accept();
}


void
PowerZonesWidget::mouseReleaseEvent
(QMouseEvent *event)
{
    auto h = heights();
    for (int i = 0; i < toggleAnimation.animationCount(); ++i) {
        QPropertyAnimation * spoilerAnimation = static_cast<QPropertyAnimation *>(toggleAnimation.animationAt(i));
        spoilerAnimation->setDuration(100);
        spoilerAnimation->setStartValue(h.first);
        spoilerAnimation->setEndValue(h.second);
    }
    toggleAnimation.setDirection(collapsed ? QAbstractAnimation::Forward : QAbstractAnimation::Backward);
    toggleAnimation.start();

    collapsed = ! collapsed;

    event->accept();
}


void
PowerZonesWidget::changeEvent
(QEvent *event)
{
    if (event->type() == QEvent::FontChange) {
        detailsDoc.setDefaultFont(font());
        adjustHeight();
    }
}


int
PowerZonesWidget::rowTop
(int rowHeight, int row) const
{
    return (row - 1) * rowHeight;
}


QColor
PowerZonesWidget::color
(int idx) const
{
    if (idx >= 0 && idx < colors.length()) {
        return colors[idx];
    } else {
        return QColor("red");
    }
}


QString
PowerZonesWidget::name
(int idx) const
{
    if (idx >= 0 && idx < names.length()) {
        return names[idx];
    } else {
        return "_out of range_";
    }
}


void
PowerZonesWidget::fillDetailsDoc
()
{
    QColor bgColor = GColor(CTRAINPLOTBACKGROUND);
    QColor fgColor = GCColor::invertColor(bgColor);
    QColor fgLowColor = GCColor::alternateColor(bgColor);

    QString detailsText = QString("<table>");
    for (int i = 0; i < zones.length(); ++i) {
        int durationHours = (duration * zones[i] / 100000) / 60;
        int durationMins = (static_cast<int>(duration * zones[i] / 100000)) % 60;

        QString cellColor;
        QString textColor;
        if (zones[i] > 0) {
            cellColor = QString(" bgcolor=\"%1\"").arg(color(i).name());
        } else {
            textColor = QString(" style=\"color: %1;\"").arg(fgLowColor.name());
        }
        if (i == dominantZone - 1) {
            detailsText.append(QString("<tr><td%6>&nbsp;</td>"
                                       "    <td><b>Z%1</b></td>"
                                       "    <td><b>%2</b></td>"
                                       "    <td><b>%3:%4</b></td>"
                                       "    <td align=\"right\"><b>%5 %</b></td>"
                                       "</tr>")
                                     .arg(i + 1)
                                     .arg(name(i))
                                     .arg(durationHours, 2, 10, QChar('0'))
                                     .arg(durationMins, 2, 10, QChar('0'))
                                     .arg(zones[i], 0, 'f', 1)
                                     .arg(cellColor));
        } else {
            detailsText.append(QString("<tr><td%6>&nbsp;</td>"
                                       "    <td%7>Z%1</td>"
                                       "    <td%7>%2</td>"
                                       "    <td%7>%3:%4</td>"
                                       "    <td align=\"right\" %7>%5 %</td>"
                                       "</tr>")
                                     .arg(i + 1)
                                     .arg(name(i))
                                     .arg(durationHours, 2, 10, QChar('0'))
                                     .arg(durationMins, 2, 10, QChar('0'))
                                     .arg(zones[i], 0, 'f', 1)
                                     .arg(cellColor, textColor));
        }
    }
    detailsText.append("</table>");
    detailsDoc.setDefaultStyleSheet(QString("table { color: %1; }").arg(fgColor.name()));
    detailsDoc.setTextWidth(contentsRect().width());
    detailsDoc.setHtml(detailsText);
    adjustHeight();
}


void
PowerZonesWidget::adjustHeight
()
{
    auto h = heights();
    setMinimumSize(100, collapsed ? h.first : h.second);
    setMaximumSize(100000, collapsed ? h.first : h.second);
}


std::pair<int, int>
PowerZonesWidget::heights
() const
{
    QMargins cm = contentsMargins();
    QFontMetrics fontMetrics(font(), this);
    int rowHeight = fontMetrics.height();
    int collapsedHeight = rowHeight * 1.6 + cm.top() + cm.bottom();
    return std::make_pair(collapsedHeight, collapsedHeight + detailsDoc.size().height());
}
