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

#ifndef _GC_GcChartAxis_h
#define _GC_GcChartAxis_h 1

#include <QQuickPaintedItem>

// QPainter-drawn axis for a GcLineCurve plot: ticks, labels and axis
// line. Cheap because repaint is triggered only on range changes, not
// per-frame. Horizontal draws ticks below a top-edge line; Vertical
// draws ticks to the left of a right-edge line.
class GcChartAxis : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(int orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
    Q_PROPERTY(double valueMin READ valueMin WRITE setValueMin NOTIFY rangeChanged)
    Q_PROPERTY(double valueMax READ valueMax WRITE setValueMax NOTIFY rangeChanged)
    Q_PROPERTY(int tickCount READ tickCount WRITE setTickCount NOTIFY tickCountChanged)
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)

public:
    explicit GcChartAxis(QQuickItem *parent = nullptr);

    int orientation() const { return orientation_; }
    void setOrientation(int o);

    double valueMin() const { return valueMin_; }
    double valueMax() const { return valueMax_; }
    void setValueMin(double v);
    void setValueMax(double v);

    int tickCount() const { return tickCount_; }
    void setTickCount(int n);

    QString label() const { return label_; }
    void setLabel(const QString &s);

    void paint(QPainter *p) override;

signals:
    void orientationChanged();
    void rangeChanged();
    void tickCountChanged();
    void labelChanged();

private:
    int orientation_ = Qt::Horizontal;
    double valueMin_ = 0, valueMax_ = 1;
    int tickCount_ = 5;
    QString label_;
};

#endif
