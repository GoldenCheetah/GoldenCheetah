#include <qevent.h>
#include <qpainter.h>
#include <qwt_math.h>
#include <qwt_polygon.h>
#include "attitude_indicator.h"

AttitudeIndicatorNeedle::AttitudeIndicatorNeedle(const QColor &c)
{
    QPalette palette;
    for ( int i = 0; i < QPalette::NColorGroups; i++ )
    {
#if QT_VERSION < 0x040000
        palette.setColor((QPalette::ColorGroup)i,
            QColorGroup::Text, c);
#else
        palette.setColor((QPalette::ColorGroup)i,
            QPalette::Text, c);
#endif
    }
    setPalette(palette);
}

void AttitudeIndicatorNeedle::draw(QPainter *painter, const QPoint &center,
    int length, double direction, QPalette::ColorGroup cg) const
{
    direction *= M_PI / 180.0;
    int triangleSize = qRound(length * 0.1);

    painter->save();

    const QPoint p0(QPoint(center.x() + 1, center.y() + 1));

    const QPoint p1 = qwtPolar2Pos(p0,
        length - 2 * triangleSize - 2, direction);

    QwtPolygon pa(3);
    pa.setPoint(0, qwtPolar2Pos(p1, 2 * triangleSize, direction));
    pa.setPoint(1, qwtPolar2Pos(p1, triangleSize, direction + M_PI_2));
    pa.setPoint(2, qwtPolar2Pos(p1, triangleSize, direction - M_PI_2));

    const QColor color =
#if QT_VERSION < 0x040000
        palette().color(cg, QColorGroup::Text);
#else
        palette().color(cg, QPalette::Text);
#endif
    painter->setBrush(color);
    painter->drawPolygon(pa);

    painter->setPen(QPen(color, 3));
    painter->drawLine(qwtPolar2Pos(p0, length - 2, direction + M_PI_2),
        qwtPolar2Pos(p0, length - 2, direction - M_PI_2));

    painter->restore();
}

AttitudeIndicator::AttitudeIndicator(
        QWidget *parent):
    QwtDial(parent),
    d_gradient(0.0)
{
    setMode(RotateScale);
    setWrapping(true);

    setOrigin(270.0);
    setScaleOptions(ScaleTicks);
    setScale(0, 0, 30.0);

    const QColor color =
#if QT_VERSION < 0x040000
        colorGroup().text();
#else
        palette().color(QPalette::Text);
#endif

    setNeedle(new AttitudeIndicatorNeedle(color));
}

void AttitudeIndicator::setGradient(double gradient)
{
    if ( gradient < -1.0 )
         gradient = -1.0;
    else if ( gradient > 1.0 )
        gradient = 1.0;

    if ( d_gradient != gradient )
    {
        d_gradient = gradient;
        update();
    }
}

void AttitudeIndicator::drawScale(QPainter *painter, const QPoint &center,
    int radius, double origin, double minArc, double maxArc) const
{
    double dir = (360.0 - origin) * M_PI / 180.0; // counter clockwise, radian

    int offset = 4;
    
    const QPoint p0 = qwtPolar2Pos(center, offset, dir + M_PI);

    const int w = contentsRect().width();

    QwtPolygon pa(4);
    pa.setPoint(0, qwtPolar2Pos(p0, w, dir - M_PI_2));
    pa.setPoint(1, qwtPolar2Pos(pa.point(0), 2 * w, dir + M_PI_2));
    pa.setPoint(2, qwtPolar2Pos(pa.point(1), w, dir));
    pa.setPoint(3, qwtPolar2Pos(pa.point(2), 2 * w, dir - M_PI_2));

    painter->save();
    painter->setClipRegion(pa); // swallow 180 - 360 degrees

    QwtDial::drawScale(painter, center, radius, origin,
        minArc, maxArc);

    painter->restore();
}

void AttitudeIndicator::drawScaleContents(QPainter *painter,
    const QPoint &, int) const
{
    int dir = 360 - qRound(origin() - value()); // counter clockwise
    int arc = 90 + qRound(gradient() * 90);

    const QColor skyColor(38, 151, 221);

    painter->save();
    painter->setBrush(skyColor);
    painter->drawChord(scaleContentsRect(), 
        (dir - arc) * 16, 2 * arc * 16 ); 
    painter->restore();
}

void AttitudeIndicator::keyPressEvent(QKeyEvent *e)
{
    switch(e->key())
    {
        case Qt::Key_Plus:
            setGradient(gradient() + 0.05);
            break;
            
        case Qt::Key_Minus:
            setGradient(gradient() - 0.05);
            break;

        default:
            QwtDial::keyPressEvent(e);
    }
}
