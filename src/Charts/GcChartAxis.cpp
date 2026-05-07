/*
 * Copyright (c) 2026 Cassidy Macdonald
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

#include "GcChartAxis.h"

#include <QPainter>
#include <QFontMetricsF>

GcChartAxis::GcChartAxis(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
}

void GcChartAxis::setOrientation(int o)
{
    if (o == orientation_) return;
    orientation_ = o;
    emit orientationChanged();
    update();
}
void GcChartAxis::setValueMin(double v)
{
    if (v == valueMin_) return;
    valueMin_ = v;
    emit rangeChanged();
    update();
}
void GcChartAxis::setValueMax(double v)
{
    if (v == valueMax_) return;
    valueMax_ = v;
    emit rangeChanged();
    update();
}
void GcChartAxis::setTickCount(int n)
{
    if (n == tickCount_ || n < 2) return;
    tickCount_ = n;
    emit tickCountChanged();
    update();
}
void GcChartAxis::setLabel(const QString &s)
{
    if (s == label_) return;
    label_ = s;
    emit labelChanged();
    update();
}

void
GcChartAxis::paint(QPainter *p)
{
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setRenderHint(QPainter::TextAntialiasing, true);

    QPen pen(Qt::black);
    pen.setWidthF(1.0);
    p->setPen(pen);

    const double w = width();
    const double h = height();
    const double span = (valueMax_ - valueMin_) > 0 ? (valueMax_ - valueMin_) : 1.0;

    QFontMetricsF fm(p->font());

    if (orientation_ == Qt::Horizontal) {
        p->drawLine(QPointF(0, 0), QPointF(w, 0));
        for (int i = 0; i < tickCount_; ++i) {
            const double t = double(i) / double(tickCount_ - 1);
            const double px = t * w;
            const double val = valueMin_ + t * span;
            p->drawLine(QPointF(px, 0), QPointF(px, 6));
            const QString text = QString::number(val, 'f', val >= 10 ? 0 : 1);
            const double tw = fm.horizontalAdvance(text);
            const double tx = qBound(0.0, px - tw / 2.0, w - tw);
            p->drawText(QPointF(tx, 10 + fm.ascent()), text);
        }
        if (!label_.isEmpty()) {
            const double tw = fm.horizontalAdvance(label_);
            p->drawText(QPointF((w - tw) / 2.0, h - fm.descent()), label_);
        }
    } else {
        p->drawLine(QPointF(w, 0), QPointF(w, h));
        for (int i = 0; i < tickCount_; ++i) {
            const double t = double(i) / double(tickCount_ - 1);
            const double py = h - t * h;
            const double val = valueMin_ + t * span;
            p->drawLine(QPointF(w, py), QPointF(w - 6, py));
            const QString text = QString::number(val, 'f', val >= 10 ? 0 : 1);
            const double tw = fm.horizontalAdvance(text);
            p->drawText(QPointF(w - 10 - tw, py + fm.ascent() / 2.0 - 1), text);
        }
        if (!label_.isEmpty()) {
            p->save();
            p->translate(fm.ascent(), h / 2.0);
            p->rotate(-90);
            const double tw = fm.horizontalAdvance(label_);
            p->drawText(QPointF(-tw / 2.0, 0), label_);
            p->restore();
        }
    }
}
