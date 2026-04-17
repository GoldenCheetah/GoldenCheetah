#include "LineCurveItem.h"

#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGFlatColorMaterial>

LineCurveItem::LineCurveItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

void
LineCurveItem::setSamples(const QVector<QPointF> &samples,
                          double xMin, double xMax,
                          double yMin, double yMax)
{
    samples_ = samples;
    xMin_ = xMin; xMax_ = xMax;
    yMin_ = yMin; yMax_ = yMax;
    dirty_ = true;
    update();
}

void
LineCurveItem::setColor(const QColor &c)
{
    if (c == color_) return;
    color_ = c;
    dirty_ = true;
    emit colorChanged();
    update();
}

QSGNode *
LineCurveItem::updatePaintNode(QSGNode *old, UpdatePaintNodeData *)
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

    if (dirty_) {
        auto *geom = node->geometry();
        geom->allocate(samples_.size());
        auto *verts = geom->vertexDataAsPoint2D();

        const double w = width();
        const double h = height();
        const double xSpan = (xMax_ - xMin_) > 0 ? (xMax_ - xMin_) : 1.0;
        const double ySpan = (yMax_ - yMin_) > 0 ? (yMax_ - yMin_) : 1.0;

        for (int i = 0; i < samples_.size(); ++i) {
            const double sx = (samples_[i].x() - xMin_) / xSpan * w;
            // y is flipped: QSG 0 is top, data 0 is bottom
            const double sy = h - (samples_[i].y() - yMin_) / ySpan * h;
            verts[i].set(float(sx), float(sy));
        }
        node->markDirty(QSGNode::DirtyGeometry);

        auto *mat = static_cast<QSGFlatColorMaterial *>(node->material());
        if (mat->color() != color_) {
            mat->setColor(color_);
            node->markDirty(QSGNode::DirtyMaterial);
        }
        dirty_ = false;
    }

    return node;
}
