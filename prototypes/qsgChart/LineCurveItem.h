#ifndef LINE_CURVE_ITEM_H
#define LINE_CURVE_ITEM_H

#include <QQuickItem>
#include <QVector>
#include <QPointF>
#include <QColor>

// Minimal QSG-backed line chart item. Takes (x,y) samples in data
// coordinates and a viewport rectangle (xMin/xMax/yMin/yMax), emits a
// single QSGGeometryNode with GL_LINE_STRIP geometry.
//
// xMin/xMax/yMin/yMax are QML-bindable so a MouseArea can drive pan
// and zoom by updating them; the geometry is re-projected each
// updatePaintNode() but never reallocated unless the sample set
// itself changed.
class LineCurveItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(double xMin READ xMin WRITE setXMin NOTIFY viewportChanged)
    Q_PROPERTY(double xMax READ xMax WRITE setXMax NOTIFY viewportChanged)
    Q_PROPERTY(double yMin READ yMin WRITE setYMin NOTIFY viewportChanged)
    Q_PROPERTY(double yMax READ yMax WRITE setYMax NOTIFY viewportChanged)
public:
    explicit LineCurveItem(QQuickItem *parent = nullptr);

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
