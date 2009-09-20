#include <qpainter.h>
#include <qwt_dial_needle.h>
#include "speedo_meter.h"

SpeedoMeter::SpeedoMeter(QWidget *parent):
    QwtDial(parent),
    d_label("km/h")
{
    setWrapping(false);
    setReadOnly(true);

    setOrigin(135.0);
    setScaleArc(0.0, 270.0);
    scaleDraw()->setSpacing(8);

    QwtDialSimpleNeedle *needle = new QwtDialSimpleNeedle(
            QwtDialSimpleNeedle::Arrow, true, Qt::red, 
            QColor(Qt::gray).light(130));
    setNeedle(needle);

    setScaleOptions(ScaleTicks | ScaleLabel);
    setScaleTicks(0, 4, 8);
}

void SpeedoMeter::setLabel(const QString &label)
{
    d_label = label;
    update();
}

QString SpeedoMeter::label() const
{
    return d_label;
}

void SpeedoMeter::drawScaleContents(QPainter *painter,
    const QPoint &center, int radius) const
{
    QRect rect(0, 0, 2 * radius, 2 * radius - 10);
    rect.moveCenter(center);

    const QColor color =
#if QT_VERSION < 0x040000
        colorGroup().text();
#else
        palette().color(QPalette::Text);
#endif
    painter->setPen(color);

    const int flags = Qt::AlignBottom | Qt::AlignHCenter;
    painter->drawText(rect, flags, d_label);
}
