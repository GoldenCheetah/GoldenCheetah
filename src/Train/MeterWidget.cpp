/*
 * Copyright (c) 2015 Vianney Boyer (vlcvboyer@gmail.com)
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

#include <QtGui>
#include <QGraphicsPathItem>
#include "MeterWidget.h"

MeterWidget::MeterWidget(QString Name, QWidget *parent, QString Source) : QWidget(parent), m_Name(Name), m_container(parent), m_Source(Source)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_PaintOnScreen);

    setAttribute(Qt::WA_TransparentForMouseEvents);

    //default settings
    m_MainColor = QColor(255,0,0,180);
    m_ScaleColor = QColor(200,200,200,200);
    m_OutlineColor = QColor(128,128,128,180);
    m_MainFont = QFont(this->font().family(), 64);
    m_AltFont = QFont(this->font().family(), 48);
    m_BackgroundColor = QColor(96, 96, 96, 200);
    m_RangeMin = 0;
    m_RangeMax = 100;
    m_Angle = 180.0;
    m_SubRange = 10;
    boundingRectVisibility = false;
}

void MeterWidget::SetRelativeSize(float RelativeWidth, float RelativeHeight)
{
    m_RelativeWidth = RelativeWidth / 100.0;
    m_RelativeHeight = RelativeHeight / 100.0;
    AdjustSizePos();
}

void MeterWidget::SetRelativePos(float RelativePosX, float RelativePosY)
{
    m_RelativePosX = RelativePosX / 100.0;
    m_RelativePosY = RelativePosY / 100.0;
    AdjustSizePos();
}

void MeterWidget::AdjustSizePos()
{
    QPoint p;
    if (m_container->windowFlags() & Qt::Window)
        p = m_container->pos();
    else
        p = m_container->mapToGlobal(m_container->pos());
    ComputeSize();
    m_PosX = p.x() + m_container->width() * m_RelativePosX - m_Width/2;
    m_PosY = p.y() + m_container->height() * m_RelativePosY - m_Height/2;
    move(m_PosX, m_PosY);
    adjustSize();
}

void MeterWidget::ComputeSize()
{
    m_Width = m_Height = (m_container->width() * m_RelativeWidth + m_container->height() * m_RelativeHeight) / 2;
}

QSize MeterWidget::sizeHint() const
{
    return QSize(m_Width, m_Height);
}

QSize MeterWidget::minimumSize() const
{
    return QSize(m_Width, m_Height);
}

void MeterWidget::paintEvent(QPaintEvent* paintevent)
{
    Q_UNUSED(paintevent);
    if (boundingRectVisibility)
    {
        m_OutlinePen = QPen(boundingRectColor);
        m_OutlinePen.setWidth(2);
        m_OutlinePen.setStyle(Qt::SolidLine);

        //painter
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        painter.setPen(m_OutlinePen);
        painter.drawRect (1, 1, m_Width-2, m_Height-2);
    }
}

void  MeterWidget::setColor(QColor  mainColor)
{
    m_MainColor = mainColor;
}

void MeterWidget::setBoundingRectVisibility(bool show, QColor  boundingRectColor)
{
    this->boundingRectVisibility=show;
    this->boundingRectColor = boundingRectColor;
}

TextMeterWidget::TextMeterWidget(QString Name, QWidget *parent, QString Source) : MeterWidget(Name, parent, Source)
{
}

void TextMeterWidget::ComputeSize()
{
    m_Width = m_container->width() * m_RelativeWidth;
    m_Height =  m_container->height() * m_RelativeHeight;
}

void TextMeterWidget::paintEvent(QPaintEvent* paintevent)
{
    MeterWidget::paintEvent(paintevent);

    m_MainBrush = QBrush(m_MainColor);
    m_OutlinePen = QPen(m_OutlineColor);
    m_OutlinePen.setWidth(1);
    m_OutlinePen.setStyle(Qt::SolidLine);

    //painter
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath my_painterPath;
    my_painterPath.addText(QPointF(0,0),m_MainFont,Text);
    my_painterPath.addText(QPointF(QFontMetrics(m_MainFont).width(Text), 0),m_AltFont,AltText);
    QRectF ValueBoundingRct = my_painterPath.boundingRect();

    // define scale
    float fontscale = qMin(m_Width / ValueBoundingRct.width(), m_Height / ValueBoundingRct.height());
    painter.scale(fontscale, fontscale);
    painter.translate(-ValueBoundingRct.x()+(m_Width/fontscale - ValueBoundingRct.width())/2, -ValueBoundingRct.y()+(m_Height/fontscale - ValueBoundingRct.height())/2);

    // Write Value
    painter.setPen(m_OutlinePen);
    painter.setBrush(m_MainBrush);
    painter.drawPath(my_painterPath);
}

CircularIndicatorMeterWidget::CircularIndicatorMeterWidget(QString Name, QWidget *parent, QString Source) : MeterWidget(Name, parent, Source)
{
    //defaut settings
    IndicatorGradient = QConicalGradient(0, 0, 150);
    IndicatorGradient.setColorAt(0.0, QColor(255,0,0,180));
    IndicatorGradient.setColorAt(0.3, QColor(255,255,0,180));
    IndicatorGradient.setColorAt(0.7, QColor(0,255,0,180));
    IndicatorGradient.setColorAt(1.0, QColor(0,255,0,180));
}

void CircularIndicatorMeterWidget::paintEvent(QPaintEvent* paintevent)
{
    MeterWidget::paintEvent(paintevent);

    m_MainBrush = QBrush(m_MainColor);
    m_OutlinePen = QPen(m_OutlineColor);
    m_OutlinePen.setWidth(1);
    m_OutlinePen.setStyle(Qt::SolidLine);

    //painter
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    // define scale and location
    painter.translate(m_Width / 2, m_Height / 2);
    painter.scale(m_Width / 200.0, m_Height / 200.0);

    // Draw circular indicator
    // Define coordinates:
    static const QPoint CP_pt1(0, -70);
    static const QPoint CP_pt2(0, -90);
    static const QRectF CP_extRect(-90,-90,180,180);
    static const QRectF CP_intRect(-70,-70,140,140);
    // rotate painter
    painter.save();
    painter.rotate(-m_Angle/2);

    double CPAngle = qBound((float) -1.0, (Value-m_RangeMin) / (m_RangeMax-m_RangeMin), (float) 1.0) * m_Angle;
    QPainterPath CPEmptyPath;
    CPEmptyPath.moveTo(CP_pt1);
    CPEmptyPath.arcMoveTo(CP_intRect, 90 - CPAngle);
    QPainterPath CPIndicatorPath;
    CPIndicatorPath.moveTo(CP_pt1);
    CPIndicatorPath.lineTo(CP_pt2);
    CPIndicatorPath.arcTo(CP_extRect, 90, -1 * CPAngle);
    CPIndicatorPath.lineTo(CPEmptyPath.currentPosition());
    CPIndicatorPath.arcTo(CP_intRect, 90 - CPAngle, CPAngle);
    painter.setBrush(IndicatorGradient);
    painter.setPen(Qt::NoPen);
    painter.drawPath(CPIndicatorPath);
    painter.restore();
}

CircularBargraphMeterWidget::CircularBargraphMeterWidget(QString Name, QWidget *parent, QString Source) : MeterWidget(Name, parent, Source)
{
}

void CircularBargraphMeterWidget::paintEvent(QPaintEvent* paintevent)
{
    MeterWidget::paintEvent(paintevent);

    m_MainBrush = QBrush(m_MainColor);
    m_OutlinePen = QPen(m_MainColor);
    m_OutlinePen.setWidth(3);
    m_OutlinePen.setStyle(Qt::SolidLine);

    //painter
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //draw bargraph
    painter.setPen(m_OutlinePen);
    painter.setBrush(m_MainBrush);
    painter.save();
    painter.translate(m_Width / 2, m_Height / 2);
    painter.rotate((360.0-m_Angle)/2.0);
    for (int i=0; i<=(int)(m_SubRange*qMin(1.0,(double)(Value-m_RangeMin)/(m_RangeMax-m_RangeMin))); i++)
    {
        painter.drawLine (0, m_Height*2/10, 0, m_Height*4/10);
        painter.rotate(m_Angle/m_SubRange);
    }
    painter.restore();
}

NeedleMeterWidget::NeedleMeterWidget(QString Name, QWidget *parent, QString Source) : MeterWidget(Name, parent, Source)
{
}

void NeedleMeterWidget::paintEvent(QPaintEvent* paintevent)
{
    MeterWidget::paintEvent(paintevent);

    m_MainBrush = QBrush(m_MainColor);
    m_BackgroundBrush = QBrush(m_BackgroundColor);
    m_OutlinePen = QPen(m_OutlineColor);
    m_OutlinePen.setWidth(1);
    m_OutlinePen.setStyle(Qt::SolidLine);
    m_ScalePen = QPen(m_ScaleColor);
    m_ScalePen.setWidth(2);
    m_ScalePen.setStyle(Qt::SolidLine);

    //painter
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //draw background
    painter.setPen(m_OutlinePen);
    painter.setBrush(m_BackgroundBrush);
    painter.drawEllipse (0, 0, m_Width, m_Height);

    //draw scale
    painter.setPen(m_ScalePen);
    painter.setBrush(Qt::NoBrush);
    painter.save();
    painter.translate(m_Width / 2, m_Height / 2);
    painter.rotate(m_Angle/2.0-90.0);
    painter.drawArc (-m_Width*4/10, -m_Height*4/10, m_Width*8/10, m_Height*8/10, 0, (int) (16.0*m_Angle));
    painter.restore();
    painter.save();
    painter.translate(m_Width / 2, m_Height / 2);
    painter.rotate((360.0-m_Angle)/2.0);
    for (int i=0; i<=m_SubRange; i++)
    {
        painter.drawLine (0, m_Height*3/10, 0, m_Height*4/10);
        painter.rotate(m_Angle/m_SubRange);
    }
    painter.restore();

    //draw needle
    painter.setPen(m_OutlinePen);
    painter.setBrush(m_MainBrush);
    QPainterPath my_painterPath;
    painter.save();
    painter.translate(m_Width / 2, m_Height / 2);
    painter.rotate(-m_Angle/2+(qBound((float) -1.0, (Value-m_RangeMin)/(m_RangeMax-m_RangeMin), (float) 1.0)*m_Angle));
    my_painterPath.moveTo(-2, 0);
    my_painterPath.lineTo(0, -m_Height*4/10);
    my_painterPath.lineTo(+2, 0);
    my_painterPath.lineTo(-2, 0);
    painter.drawPath(my_painterPath);
    painter.restore();
}
