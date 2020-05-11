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
#include "ErgFile.h"
#include "Context.h"

#include <QtWebEngineWidgets/QWebEngineView>

#define GOOGLE_KEY "ABQIAAAAS9Z2oFR8vUfLGYSzz40VwRQ69UCJw2HkJgivzGoninIyL8-QPBTtnR-6pM84ljHLEk3PDql0e2nJmg"

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
    m_BackgroundColor = QColor(96, 96, 96, 0);
    m_RangeMin = 0;
    m_RangeMax = 100;
    m_Angle = 180.0;
    m_SubRange = 10;
    boundingRectVisibility = false;
    forceSquareRatio = true;
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
    if (forceSquareRatio)
    {
        m_Width = m_Height = (m_container->width() * m_RelativeWidth + m_container->height() * m_RelativeHeight) / 2;
    }
    else
    {
        m_Width = m_container->width() * m_RelativeWidth;
        m_Height =  m_container->height() * m_RelativeHeight;
    }
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
    forceSquareRatio = false;
}

void TextMeterWidget::paintEvent(QPaintEvent* paintevent)
{
    MeterWidget::paintEvent(paintevent);

    m_MainBrush = QBrush(m_MainColor);
    m_BackgroundBrush = QBrush(m_BackgroundColor);
    m_OutlinePen = QPen(m_OutlineColor);
    m_OutlinePen.setWidth(1);
    m_OutlinePen.setStyle(Qt::SolidLine);

    //painter
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //draw background
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_BackgroundBrush);
    if (Text!=QString(""))
        painter.drawRect (0, 0, m_Width, m_Height);

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

ElevationMeterWidget::ElevationMeterWidget(QString Name, QWidget *parent, QString Source, Context *context) : MeterWidget(Name, parent, Source), context(context)
{
    forceSquareRatio = false;
    gradientValue = 0.0;
}

void ElevationMeterWidget::paintEvent(QPaintEvent* paintevent)
{
    // TODO : show Power when not in slope simulation mode
    if (!context || !context->currentErgFile() || context->currentErgFile()->Points.size()<=1)
        return;

    MeterWidget::paintEvent(paintevent);

    m_MainBrush = QBrush(m_MainColor);
    m_BackgroundBrush = QBrush(m_BackgroundColor);
    m_OutlinePen = QPen(m_OutlineColor);
    m_OutlinePen.setWidth(1);
    m_OutlinePen.setStyle(Qt::SolidLine);

    //painter
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Find min/max
    double minX, minY, maxX, maxY, cyclistX=0.0;
    // (based on ErgFilePlot.cpp)
    minX=maxX=context->currentErgFile()->Points[0].x; // meters
    minY=maxY=context->currentErgFile()->Points[0].y; // meters or altitude???
    foreach(ErgFilePoint x, context->currentErgFile()->Points)
    {
        if (x.y > maxY) maxY = x.y;
        if (x.x > maxX) maxX = x.x;
        if (x.y < minY) minY = x.y;
        if (x.x < minX) minX = x.x;
    }
    // check if slope shown will not be too inconsistent (based on widget's width/height ratio)
    // we accept 20 times i.e. 5% gradient will be shown as 45°
    if ( m_Width!=0 && (maxY-minY) / 0.05 < (double)m_Height * 0.80 * (maxX-minX) / (double)m_Width)
        maxY = minY + (double)m_Height * 0.80 * (maxX-minX) / (double)m_Width * 0.05;
    double bubbleSize = (double)m_Height*0.010f;
    minY -= (maxY-minY) * 0.20f; // add 20% as bottom headroom (slope gradient will be shown there in a bubble)

    // this->Value should hold the current distance in meters. 
    cyclistX = (this->Value * 1000.0 - minX) * (double)m_Width / (maxX-minX);

    //Get point to create the polygon
    QPolygon polygon;
    polygon << QPoint(0.0, (double)m_Height);
    double x, y, pt=0;
    double nextX = 1;
    for( pt=0; pt < context->currentErgFile()->Points.size(); pt++)
    {
        for ( ; x < nextX && pt < context->currentErgFile()->Points.size(); pt++)
        {
            x = (context->currentErgFile()->Points[pt].x - minX) * (double)m_Width / (maxX-minX);
            y = (context->currentErgFile()->Points[pt].y - minY) * (double)m_Height / (maxY-minY);
        }
        // Add points to polygon only once every time the x coordinate integer part changes.
        polygon << QPoint(x, (double)m_Height - y);
        nextX = floor(x) + 1.0;
    }
    polygon << QPoint((double) m_Width, (double)m_Height);
    polygon << QPoint(fmin((double) m_Width,cyclistX+bubbleSize), (double)m_Height);
    polygon << QPoint(cyclistX, (double)m_Height-bubbleSize);
    polygon << QPoint(fmax(0.0, cyclistX-bubbleSize), (double)m_Height);

    painter.setPen(m_OutlinePen);
    painter.setBrush(m_BackgroundBrush);
    painter.drawPolygon(polygon);

    m_OutlinePen = QPen(m_MainColor);
    m_OutlinePen.setWidth(1);
    m_OutlinePen.setStyle(Qt::SolidLine);
    painter.setPen(m_OutlinePen);
    painter.drawLine(cyclistX, 0.0, cyclistX, (double)m_Height-bubbleSize);

    //Cosmetic enhancment: Display grade as #.#% 
    std::string sGrad;
    QString s_grad ="";
    s_grad = ((-1.0 < this->gradientValue && this->gradientValue < 0.0)?QString("-"):QString("")) + QString::number((int) this->gradientValue);
    s_grad += QString(".") + QString::number(abs((int)(this->gradientValue * 10.0) % 10)) + QString("%");

    // Display gradient text to the right of the line until the middle, then display to the left of the line
    if (cyclistX < m_Width*0.5) {
        painter.drawText((double)cyclistX+5, ((double)m_Height * 0.95), s_grad);
    } else {
        painter.drawText((double)cyclistX-45, ((double)m_Height * 0.95), s_grad);
    }
} 


LiveMapWidget::LiveMapWidget(QString Name, QWidget *parent, QString Source, Context *context) : MeterWidget(Name, parent, Source), context(context) 
{
    forceSquareRatio = false;
    //gradientValue = 0.0;
    curr_lon = 0.0;
    curr_lat = 0.0;
    liveMapView = new QWebEngineView(this);
    liveMapInitialized = false;
 }

void LiveMapWidget::paintEvent(QPaintEvent* paintevent)
{
    MeterWidget::paintEvent(paintevent);

    m_MainBrush = QBrush(m_MainColor);
    m_BackgroundBrush = QBrush(m_BackgroundColor);
    m_OutlinePen = QPen(m_OutlineColor);
    m_OutlinePen.setWidth(1);
    m_OutlinePen.setStyle(Qt::SolidLine);

    //painter
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //draw background
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_BackgroundBrush);
    painter.drawRect (0, 0, m_Width, m_Height);
 
    //Set pen for text
    m_OutlinePen = QPen(m_MainColor);
    m_OutlinePen.setWidth(1);
    m_OutlinePen.setStyle(Qt::SolidLine);
    painter.setPen(m_OutlinePen);

    //Print Coordinates if map is not displayed
    painter.drawText (20.0 ,((double)(m_Height/2)-20), QVariant(this->curr_lon).toString());
    painter.drawText (20.0 ,((double)m_Height/2), QVariant(this->curr_lat).toString());
}

//***************************************************************************************************************
void LiveMapWidget::initLiveMap ()
{
    if ( ! this->liveMapInitialized ) {
        //liveMapView->resize(400,400);
        liveMapView->resize(m_Width, m_Height);
        createHtml(this->curr_lat, this->curr_lon, 16);
        liveMapView->page()->setHtml(currentPage);
        liveMapView->show();
        this->liveMapInitialized = true;
    }
}

void LiveMapWidget::plotNewLatLng(double dLat, double dLon)
{
    qDebug("==> Ready to receive new LatLng") ;
    QString code;
    QString sLat = QVariant(dLat).toString();
    QString sLon = QVariant(dLon).toString();
    code = QString("moveMarker(" + sLat + " , "  + sLon + ")");
    liveMapView->page()->runJavaScript(code);
}

void LiveMapWidget::createHtml(double dLat, double dLon, int iMapZoom)
{
    QString sLat = QVariant(dLat).toString();
    QString sLon = QVariant(dLon).toString();
    QString sWidth = QVariant(m_Width).toString();
    QString sHeight = QVariant(m_Height).toString();
    QString sMapZoom = QVariant(iMapZoom).toString();
    currentPage = "";

    currentPage = QString("<html><head>\n"
    "<meta name=\"viewport\" content=\"initial-scale=1.0, user-scalable=yes\"/> \n"
    "<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\"/>\n"
    "<title>GoldenCheetah LiveMap - TrainView</title>\n");
    //Leaflet CSS and JS
    currentPage += QString("<link rel=\"stylesheet\" href=\"https://unpkg.com/leaflet@1.6.0/dist/leaflet.css\"\n"
    "integrity=\"sha512-xwE/Az9zrjBIphAcBb3F6JVqxf46+CDLwfLMHloNu6KEQCAWi6HcDUbeOfBIptF7tcCzusKFjFw2yuvEpDL9wQ==\" crossorigin=\"\"/>\n"
	"<script src=\"https://unpkg.com/leaflet@1.6.0/dist/leaflet.js\"\n"
    "integrity=\"sha512-gZwIG9x3wUXg2hdXF6+rVkLF/0Vi9U8D2Ntg4Ga5I5BZpVkVxlJWbSQtXPSiUTtC0TjtGOmxa1AJPuV0CPthew==\" crossorigin=\"\"></script>\n"
	"<style>#mapid {height:300px;}</style></head>\n");

    // local functions
    currentPage += QString("<body><div id=\"mapid\"></div>\n"
    "<script type=\"text/javascript\">\n");
    // Create Map options
    currentPage += QString("var mapOptions = {\n"
        "    center: [" + sLat + ", " + sLon + "] ,\n"
        "    zoom : " + sMapZoom + ",\n"
        "    zoomControl : false,\n"
        "    scrollWheelZoom : false,\n"
        "    dragging : false,\n"
        "    doubleClickZoom : false}\n");
    // Create map object
    currentPage += QString("var mymap = L.map('mapid', mapOptions);\n");
    // Create layer object
    currentPage += QString("var layer = new L.tileLayer('http://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png');\n");
    // Add layer to the map
    currentPage += QString("mymap.addLayer(layer);\n");
    // mymarker var
    currentPage += QString("var mymarker = new L.marker([" + sLat + ", " + sLon + "], {\n"
    "    draggable: false,\n"
    "    title: \"GoldenCheetah - Workout LiveMap\",\n"
    "    alt: \"GoldenCheetah - Workout LiveMap\",\n"
    "    riseOnHover: true\n"
    "}).addTo(mymap)\n");
    // Move marker function
    currentPage += QString(    "function moveMarker(myLat, myLon) { \n"
    "   mymap.panTo(new L.LatLng(myLat, myLon));\n"
    "    mymarker.setLatLng(new L.latLng(myLat, myLon));}\n"
    "</script>\n"
    "</body></html>\n");
}
