#ifndef AXIS_ITEM_H
#define AXIS_ITEM_H

#include <QQuickPaintedItem>

// QPainter-drawn axis: ticks + labels + axis line. Cheap and gives us
// fast iteration on tick math without writing glyph geometry; the
// axis is called maybe once per interaction event, not per frame.
//
// orientation = Qt::Horizontal draws a bottom axis (ticks going down
// from the top edge, labels below). Qt::Vertical draws a left axis
// (ticks going right from the right edge, labels to the left).
class AxisItem : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(int orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
    Q_PROPERTY(double valueMin READ valueMin WRITE setValueMin NOTIFY rangeChanged)
    Q_PROPERTY(double valueMax READ valueMax WRITE setValueMax NOTIFY rangeChanged)
    Q_PROPERTY(int tickCount READ tickCount WRITE setTickCount NOTIFY tickCountChanged)
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)

public:
    explicit AxisItem(QQuickItem *parent = nullptr);

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
