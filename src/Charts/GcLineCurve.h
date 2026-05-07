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

#ifndef _GC_GcLineCurve_h
#define _GC_GcLineCurve_h 1

#include <QQuickItem>
#include <QVector>
#include <QPointF>
#include <QColor>

// QSG-backed line chart item. Takes (x, y) samples in data coordinates
// and a viewport rectangle (xMin/xMax/yMin/yMax), emits a single
// QSGGeometryNode with GL_LINE_STRIP geometry.
//
// xMin/xMax/yMin/yMax are QML-bindable so a MouseArea (or C++) can
// drive pan/zoom by mutating them; the geometry is re-projected each
// updatePaintNode but never reallocated unless the sample set itself
// changed.
//
// See unittests/qsgChart/ for the standalone FPS benchmark harness.
class GcLineCurve : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(double xMin READ xMin WRITE setXMin NOTIFY viewportChanged)
    Q_PROPERTY(double xMax READ xMax WRITE setXMax NOTIFY viewportChanged)
    Q_PROPERTY(double yMin READ yMin WRITE setYMin NOTIFY viewportChanged)
    Q_PROPERTY(double yMax READ yMax WRITE setYMax NOTIFY viewportChanged)
public:
    explicit GcLineCurve(QQuickItem *parent = nullptr);

    void setSamples(const QVector<QPointF> &samples);

    QColor color() const { return color_; }
    void setColor(const QColor &c);

    double xMin() const { return xMin_; }
    double xMax() const { return xMax_; }
    double yMin() const { return yMin_; }
    double yMax() const { return yMax_; }
    void setXMin(double v);
    void setXMax(double v);
    void setYMin(double v);
    void setYMax(double v);

signals:
    void colorChanged();
    void viewportChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *) override;
    void geometryChange(const QRectF &newG, const QRectF &oldG) override;

private:
    QVector<QPointF> samples_;
    double xMin_ = 0, xMax_ = 1, yMin_ = 0, yMax_ = 1;
    QColor color_ = Qt::red;
    bool samplesDirty_ = true;
    bool viewportDirty_ = true;
    bool materialDirty_ = true;
};

#endif
