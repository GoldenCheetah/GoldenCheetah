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

#include "GcLineCurve.h"

#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGFlatColorMaterial>

GcLineCurve::GcLineCurve(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

void
GcLineCurve::setSamples(const QVector<QPointF> &samples)
{
    samples_ = samples;
    samplesDirty_ = true;
    update();
}

void
GcLineCurve::setColor(const QColor &c)
{
    if (c == color_) return;
    color_ = c;
    materialDirty_ = true;
    emit colorChanged();
    update();
}

static void bumpViewport(double &member, double v, bool &dirty, GcLineCurve *self)
{
    if (member == v) return;
    member = v;
    dirty = true;
    emit self->viewportChanged();
    self->update();
}

void GcLineCurve::setXMin(double v) { bumpViewport(xMin_, v, viewportDirty_, this); }
void GcLineCurve::setXMax(double v) { bumpViewport(xMax_, v, viewportDirty_, this); }
void GcLineCurve::setYMin(double v) { bumpViewport(yMin_, v, viewportDirty_, this); }
void GcLineCurve::setYMax(double v) { bumpViewport(yMax_, v, viewportDirty_, this); }

void
GcLineCurve::geometryChange(const QRectF &newG, const QRectF &oldG)
{
    QQuickItem::geometryChange(newG, oldG);
    if (newG.size() != oldG.size()) {
        viewportDirty_ = true;
        update();
    }
}

QSGNode *
GcLineCurve::updatePaintNode(QSGNode *old, UpdatePaintNodeData *)
{
    QSGGeometryNode *node = static_cast<QSGGeometryNode *>(old);

    if (!node) {
        node = new QSGGeometryNode;
        auto *geom = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0);
        geom->setDrawingMode(QSGGeometry::DrawLineStrip);
        geom->setLineWidth(2.0f);
        node->setGeometry(geom);
        node->setFlag(QSGNode::OwnsGeometry);

        auto *mat = new QSGFlatColorMaterial;
        mat->setColor(color_);
        node->setMaterial(mat);
        node->setFlag(QSGNode::OwnsMaterial);
    }

    auto *geom = node->geometry();

    if (samplesDirty_) {
        geom->allocate(samples_.size());
        samplesDirty_ = false;
        viewportDirty_ = true;
    }

    if (viewportDirty_) {
        auto *verts = geom->vertexDataAsPoint2D();
        const double w = width();
        const double h = height();
        const double xSpan = (xMax_ - xMin_) > 0 ? (xMax_ - xMin_) : 1.0;
        const double ySpan = (yMax_ - yMin_) > 0 ? (yMax_ - yMin_) : 1.0;
        for (int i = 0; i < samples_.size(); ++i) {
            const double sx = (samples_[i].x() - xMin_) / xSpan * w;
            const double sy = h - (samples_[i].y() - yMin_) / ySpan * h;
            verts[i].set(float(sx), float(sy));
        }
        node->markDirty(QSGNode::DirtyGeometry);
        viewportDirty_ = false;
    }

    if (materialDirty_) {
        auto *mat = static_cast<QSGFlatColorMaterial *>(node->material());
        mat->setColor(color_);
        node->markDirty(QSGNode::DirtyMaterial);
        materialDirty_ = false;
    }

    return node;
}
