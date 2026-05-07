#include "AxisItem.h"

#include <QPainter>
#include <QFontMetricsF>

AxisItem::AxisItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
}

void AxisItem::setOrientation(int o)
{
    if (o == orientation_) return;
    orientation_ = o;
    emit orientationChanged();
    update();
}
void AxisItem::setValueMin(double v)
{
    if (v == valueMin_) return;
    valueMin_ = v;
    emit rangeChanged();
    update();
}
void AxisItem::setValueMax(double v)
{
    if (v == valueMax_) return;
    valueMax_ = v;
    emit rangeChanged();
    update();
}
void AxisItem::setTickCount(int n)
{
    if (n == tickCount_ || n < 2) return;
    tickCount_ = n;
    emit tickCountChanged();
    update();
}
void AxisItem::setLabel(const QString &s)
{
    if (s == label_) return;
    label_ = s;
    emit labelChanged();
    update();
}

void
AxisItem::paint(QPainter *p)
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
        // axis line along the top of this item's rect
        p->drawLine(QPointF(0, 0), QPointF(w, 0));
        for (int i = 0; i < tickCount_; ++i) {
            const double t = double(i) / double(tickCount_ - 1);
            const double px = t * w;
            const double val = valueMin_ + t * span;
            p->drawLine(QPointF(px, 0), QPointF(px, 6));
            const QString text = QString::number(val, 'f', val >= 10 ? 0 : 1);
            const double tw = fm.horizontalAdvance(text);
            // clamp so first/last labels don't clip off edges
            const double tx = qBound(0.0, px - tw / 2.0, w - tw);
            p->drawText(QPointF(tx, 10 + fm.ascent()), text);
        }
        if (!label_.isEmpty()) {
            const double tw = fm.horizontalAdvance(label_);
            p->drawText(QPointF((w - tw) / 2.0, h - fm.descent()), label_);
        }
    } else {
        // axis line along the right edge
        p->drawLine(QPointF(w, 0), QPointF(w, h));
        for (int i = 0; i < tickCount_; ++i) {
            const double t = double(i) / double(tickCount_ - 1);
            const double py = h - t * h; // value increases upward
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
