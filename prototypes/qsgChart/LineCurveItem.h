#ifndef LINE_CURVE_ITEM_H
#define LINE_CURVE_ITEM_H

#include <QQuickItem>
#include <QVector>
#include <QPointF>
#include <QColor>

// Minimal QSG-backed line chart item. Takes (x,y) samples in data
// coordinates plus an x-range and y-range, emits a single
// QSGGeometryNode with GL_LINE_STRIP geometry.
class LineCurveItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
public:
    explicit LineCurveItem(QQuickItem *parent = nullptr);

    void setSamples(const QVector<QPointF> &samples,
                    double xMin, double xMax,
                    double yMin, double yMax);

    QColor color() const { return color_; }
    void setColor(const QColor &c);

signals:
    void colorChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *) override;

private:
    QVector<QPointF> samples_;
    double xMin_ = 0, xMax_ = 1, yMin_ = 0, yMax_ = 1;
    QColor color_ = Qt::red;
    bool dirty_ = true;
};

#endif
