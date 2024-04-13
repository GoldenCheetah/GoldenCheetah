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
#include "Units.h"
#include "LocationInterpolation.h"
#include <QWebEngineScriptCollection>
#include <QWebEngineProfile>
#include <array>

MeterWidget::MeterWidget(QString Name, QWidget *parent, QString Source) : QWidget(parent), m_Name(Name), m_container(parent), m_Source(Source)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

#ifdef Q_OS_LINUX
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
#else
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_PaintOnScreen);
#endif

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
    backgroundVisibility = false;
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
    // Compute the size and position relative to its parent
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

    // Translate the Video Container visible region to our coordinate for clipping
    QPoint vp = m_VideoContainer->pos();
    videoContainerRegion = m_VideoContainer->visibleRegion();
    videoContainerRegion.translate(mapFromGlobal(m_VideoContainer->mapToGlobal(vp)) - vp);
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

void MeterWidget::startPlayback(Context*)
{

}

void MeterWidget::stopPlayback()
{

}


void MeterWidget::paintEvent(QPaintEvent* paintevent)
{
    Q_UNUSED(paintevent);
    if(!boundingRectVisibility && !backgroundVisibility) return;

    QPainter painter(this);
    painter.setClipRegion(videoContainerRegion);
    painter.setRenderHint(QPainter::Antialiasing);

    int radius = qMin(m_Width, m_Height) * 0.1;
    if(backgroundVisibility) painter.setBrush(QBrush(m_BackgroundColor));
    else painter.setBrush(Qt::NoBrush);

    if (boundingRectVisibility)
    {
        m_OutlinePen = QPen(m_BoundingRectColor);
        m_OutlinePen.setWidth(2);
        m_OutlinePen.setStyle(Qt::SolidLine);

        painter.setPen(m_OutlinePen);
        painter.drawRoundedRect (1, 1, m_Width-2, m_Height-2, radius, radius);
    }
    else
    {
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect (0, 0, m_Width, m_Height, radius, radius);
    }
}

void  MeterWidget::setColor(QColor  mainColor)
{
    m_MainColor = mainColor;
}

void MeterWidget::setBoundingRectVisibility(bool show, QColor  boundingRectColor)
{
    this->boundingRectVisibility=show;
    this->m_BoundingRectColor = boundingRectColor;
}

TextMeterWidget::TextMeterWidget(QString Name, QWidget *parent, QString Source) : MeterWidget(Name, parent, Source)
{
    forceSquareRatio = false;
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
    painter.setClipRegion(videoContainerRegion);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath my_painterPath;
    my_painterPath.addText(QPointF(0,0),m_MainFont,Text);
    my_painterPath.addText(QPointF(QFontMetrics(m_MainFont).horizontalAdvance(Text), 0),m_AltFont,AltText);
    QRectF ValueBoundingRct = my_painterPath.boundingRect();

    // We use leading whitespace for alignment which boundingRect() does not count
    ValueBoundingRct.setLeft(qMin(0.0, ValueBoundingRct.left()));

    // The scale should not change with the string content, we use Font ascent and descent
    ValueBoundingRct.setTop(qMin(ValueBoundingRct.top(), qreal(qMin(-QFontMetrics(m_MainFont).ascent(), 
            -QFontMetrics(m_AltFont).ascent()))));
    ValueBoundingRct.setBottom(qMax(ValueBoundingRct.bottom(), qreal(qMax(QFontMetrics(m_MainFont).descent(),
            QFontMetrics(m_AltFont).descent()))));

    // scale to fit the available space
    float fontscale = qMin(m_Width / ValueBoundingRct.width(), m_Height / ValueBoundingRct.height());
    painter.scale(fontscale, fontscale);

    float translationX = -ValueBoundingRct.x();  // AlignLeft

    if(alignment == Qt::AlignHCenter)
        translationX += (m_Width/fontscale - ValueBoundingRct.width())/2;
    else if(alignment == Qt::AlignRight)
        translationX += (m_Width/fontscale - ValueBoundingRct.width());

    painter.translate(translationX, -ValueBoundingRct.y()+(m_Height/fontscale - ValueBoundingRct.height())/2);

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
    painter.setClipRegion(videoContainerRegion);
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
    painter.setClipRegion(videoContainerRegion);
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
    painter.setClipRegion(videoContainerRegion);
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

ElevationMeterWidget::ElevationMeterWidget(QString Name, QWidget *parent, QString Source, Context *context) : MeterWidget(Name, parent, Source), context(context),
    m_minX(0.), m_maxX(0.), m_savedWidth(0), m_savedHeight(0), m_savedMinY(0), m_savedMaxY(0), gradientValue(0.)
{
    forceSquareRatio = false;
}

// Compute polygon for elevation graph based on widget size.
void ElevationMeterWidget::lazySetup(void)
{
    // Nothing to compute unless there is a valid erg file.
    if (!context)
        return;

    const ErgFile* ergFile = context->currentErgFile();
    if (!ergFile || !ergFile->isValid())
        return;

    // Compute if size has changed. Store truncated values to allow equality comparison.
    double minX = 0.;
    double maxX = floor(ergFile->duration());
    double minY = floor(ergFile->minY());
    double maxY = floor(ergFile->maxY());

    if (m_savedWidth != m_Width || m_savedHeight != m_Height || m_savedMinY != minY || m_savedMaxY != maxY) {

        m_savedMinY = minY;
        m_savedMaxY = maxY;

        if (m_Width != 0 && (maxY - minY) / 0.05 < m_Height * 0.80 * (maxX - minX) / m_Width)
            maxY = minY + m_Height * 0.80 * (maxX - minX) / m_Width * 0.05;
        minY -= (maxY - minY) * 0.20f; // add 20% as bottom headroom (slope gradient will be shown there in a bubble)

        // Populate elevation route polygon
        m_elevationPolygon.clear();
        m_elevationPolygon << QPoint(0.0, m_Height);

        // Scaling Multiples.
        const double xScale = m_Width  / (maxX - minX);
        const double yScale = m_Height / (maxY - minY);

        int lastPixelX = -1; // always draw first point.
        foreach(const ErgFilePoint &p, ergFile->Points) {

            int pixelX = (int)floor((p.x - minX) * xScale);

            // Skip over segment points that are less than a pixel to the right of the last segment we drew.
            if (pixelX == lastPixelX)
                continue;

            int pixelY = (int)(m_Height - floor((p.y - minY) * yScale));

            m_elevationPolygon << QPoint(pixelX, pixelY);

            lastPixelX = pixelX;
        }

        // Complete a final segment from elevation profile to bottom right of display rect.
        m_elevationPolygon << QPoint(m_Width, m_Height);

        // Save distance extent, used to situate rider location within widget display.
        m_minX = minX;
        m_maxX = maxX;

        m_savedWidth = m_Width;
        m_savedHeight = m_Height;
    }
}

void ElevationMeterWidget::paintEvent(QPaintEvent* paintevent)
{
    MeterWidget::paintEvent(paintevent);

    // TODO : show Power when not in slope simulation mode
    if (!context || !context->currentErgFile() || context->currentErgFile()->Points.size()<=1)
        return;

    m_MainBrush = QBrush(m_MainColor);
    m_BackgroundBrush = QBrush(m_BackgroundColor);
    m_OutlinePen = QPen(m_OutlineColor);
    m_OutlinePen.setWidth(1);
    m_OutlinePen.setStyle(Qt::SolidLine);

    //painter
    QPainter painter(this);
    painter.setClipRegion(videoContainerRegion);
    painter.setRenderHint(QPainter::Antialiasing);

    // Lazy compute of min, max and route elevation polygon on init or if dimensions
    // change. Ideally we'd use the resize event but we are computing from ergfile
    // which isnt available when initial resize occurs.
    lazySetup();

    double bubbleSize = (double)m_Height * 0.010f;

    QPolygon polygon;
    polygon << QPoint(fmin((double) m_Width, bubbleSize), (double)m_Height);

    double cyclistX = (this->Value * 1000.0 - m_minX) * (double)m_Width / (m_maxX - m_minX);
    polygon << QPoint(cyclistX, (double)m_Height-bubbleSize);
    polygon << QPoint(fmax(0.0, cyclistX-bubbleSize), (double)m_Height);

    painter.setPen(m_OutlinePen);
    painter.setBrush(m_BackgroundBrush);
    painter.drawPolygon(m_elevationPolygon);
    painter.drawPolygon(polygon);

    m_OutlinePen = QPen(m_MainColor);
    m_OutlinePen.setWidth(1);
    m_OutlinePen.setStyle(Qt::SolidLine);
    painter.setPen(m_OutlinePen);
    painter.drawLine(cyclistX, 0.0, cyclistX, (double)m_Height-bubbleSize);

    // Display grade as #.#%
    QString gradientString = ((-1.0 < this->gradientValue && this->gradientValue < 0.0) ? QString("-") : QString("")) + QString::number((int)this->gradientValue) +
        QString(".") + QString::number(abs((int)(this->gradientValue * 10.0) % 10)) + QString("%");

    // Display gradient text to the right of the line until the middle, then display to the left of the line
    double gradientDrawX = cyclistX;
    double gradientDrawY = m_Height * 0.95;

    if (cyclistX < m_Width * 0.5)
        gradientDrawX += 5.;
    else
        gradientDrawX -= 45.;

    painter.drawText(gradientDrawX, gradientDrawY, gradientString);

    double routeDistance = this->Value;
    if (!GlobalContext::context()->useMetricUnits) routeDistance *= MILES_PER_KM;

    routeDistance = ((int)(routeDistance * 1000.)) / 1000.;

    QString distanceString = QString::number(routeDistance, 'f', 3) +
        ((GlobalContext::context()->useMetricUnits) ? tr("km") : tr("mi"));

    double distanceDrawX = (double)cyclistX;
    double distanceDrawY = ((double)m_Height * 0.75);

    // Display distance text to the right of the line until the middle, then display to the left of the line
    if (cyclistX < m_Width * 0.5)
        distanceDrawX += 5;
    else
        distanceDrawX -= 45;

    painter.drawText(distanceDrawX, distanceDrawY, distanceString);
}

LiveMapWidget::LiveMapWidget(QString Name, QWidget* parent, QString Source, Context* context) : MeterWidget(Name, parent, Source), context(context)
{
    m_Zoom = 16;
    m_osmURL = "http://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png";
    forceSquareRatio = false;
    liveMapView = new QWebEngineView(this);
    webPage = new QWebEnginePage(liveMapView);
    liveMapView->setPage(webPage);
    routeInitialized = false;
    mapInitialized = false;
    // Connect JS to C++
    webobj = new WebClass();
    channel = new QWebChannel(this);
    channel->registerObject("webobj", webobj);
    liveMapView->page()->setWebChannel(channel);
    loadingLiveMap();
}

void LiveMapWidget::startPlayback(Context* context)
{
    MeterWidget::startPlayback(context);
    if (context->currentErgFile()) 
    {
        buildRouteArrayLatLngs(context);
        if (!mapInitialized) {
            initLiveMap(context);
            mapInitialized = true;
        }
    }
    else 
    {
        qDebug() << "Error: LiveMap cannot find Ergfile";
    }
}

LiveMapWidget::~LiveMapWidget()
{
    delete webobj;
    delete channel;
    delete webPage;
    delete liveMapView;
}

void LiveMapWidget::stopPlayback()
{
    MeterWidget::stopPlayback();
    loadingLiveMap();
    mapInitialized = false;
}

void LiveMapWidget::resizeEvent(QResizeEvent*)
{
    liveMapView->resize(m_Width, m_Height);
}

void LiveMapWidget::loadingLiveMap() 
{
    //Set initial page to display loading map. 
    currentPage = QString("<html><head></head>\n"
        "<body><center><h1>Loading map...</h1></center>\n"
        "</body></html>\n");
    liveMapView->page()->setHtml(currentPage);
}

// Initialize map, build route and show it
void LiveMapWidget::initLiveMap(Context* context)
{
    QString startingLat = QVariant(context->currentErgFile()->Points[0].lat).toString();
    QString startingLon = QVariant(context->currentErgFile()->Points[0].lon).toString();

    if (startingLat == "0" && startingLon == "0")
    {
        currentPage = QString("<html><head></head>\n"
            "<body><center><h1>Ride contains invalid data</h1></center>\n"
            "</body></html>\n");
        liveMapView->page()->setHtml(currentPage);
    }
    else
    {
        QString sMapZoom = QString::number(m_Zoom);
        QString js = ("<div><script type=\"text/javascript\">initMap("
            + startingLat + "," + startingLon + "," + sMapZoom + ");"
            "showMyMarker(" + startingLat + "," + startingLon + ");</script></div>\n");
        routeLatLngs = "[";
        QString code = "";

        for (int pt = 0; pt < context->currentErgFile()->Points.size() - 1; pt++) {

            geolocation geoloc(context->currentErgFile()->Points[pt].lat, context->currentErgFile()->Points[pt].lon, context->currentErgFile()->Points[pt].y);
            if (geoloc.IsReasonableGeoLocation()) {
                if (pt == 0) { routeLatLngs += "["; }
                else { routeLatLngs += ",["; }
                routeLatLngs += QVariant(context->currentErgFile()->Points[pt].lat).toString();
                routeLatLngs += ",";
                routeLatLngs += QVariant(context->currentErgFile()->Points[pt].lon).toString();
                routeLatLngs += "]";
            }

        }
        routeLatLngs += "]";
        // We can either setHTML page or runJavaScript but not both. 
        // So we create divs with the 2 methods we need to run when the document loads
        code = QString("showRoute (" + routeLatLngs + ");");
        js += ("<div><script type=\"text/javascript\">" + code + "</script></div>\n");
        createHtml(m_osmURL, js);
        liveMapView->page()->setHtml(currentPage);
        routeInitialized = true;
    }
}

// Show route or move the marker at the next location
void LiveMapWidget::plotNewLatLng(double dLat, double dLon)
{
    QString code = "";
    // these values need extended precision or place marker jumps around.
    QString sLat = QString::number(dLat, 'g', 10);
    QString sLon = QString::number(dLon, 'g', 10);
    QString sMapZoom = QString::number(m_Zoom);

    if (!routeInitialized)
    {
        code = QString("showRoute(" + routeLatLngs + ");");
        liveMapView->page()->runJavaScript(code);
        routeInitialized = true;
    }
    else {
        code += QString("moveMarker(" + sLat + " , " + sLon + ");");
        liveMapView->page()->runJavaScript(code);
    }
}

// Build LatLon array for selected workout
void LiveMapWidget::buildRouteArrayLatLngs(Context* context)
{
    routeLatLngs = "[";
    for (int pt = 0; pt < context->currentErgFile()->Points.size() - 1; pt++) {
        if (pt == 0) { routeLatLngs += "["; }
        else { routeLatLngs += ",["; }
        routeLatLngs += QVariant(context->currentErgFile()->Points[pt].lat).toString();
        routeLatLngs += ",";
        routeLatLngs += QVariant(context->currentErgFile()->Points[pt].lon).toString();
        routeLatLngs += "]";
    }
    routeLatLngs += "]";
}


// Build HTML code with all the javascript functions to be called later
// to update the postion on the map
void LiveMapWidget::createHtml(QString sBaseUrl, QString autoRunJS)
{
    currentPage = "";

    currentPage = QString("<html><head>\n"
        "<meta name=\"viewport\" content=\"initial-scale=1.0, user-scalable=yes\"/> \n"
        "<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\"/>\n"
        "<title>GoldenCheetah LiveMap - TrainView</title>\n"
        "<link rel=\"stylesheet\" href=\"https://unpkg.com/leaflet@1.6.0/dist/leaflet.css\"\n"
        "integrity=\"sha512-xwE/Az9zrjBIphAcBb3F6JVqxf46+CDLwfLMHloNu6KEQCAWi6HcDUbeOfBIptF7tcCzusKFjFw2yuvEpDL9wQ==\" crossorigin=\"\"/>\n"
        "<script src=\"https://unpkg.com/leaflet@1.6.0/dist/leaflet.js\"\n"
        "integrity=\"sha512-gZwIG9x3wUXg2hdXF6+rVkLF/0Vi9U8D2Ntg4Ga5I5BZpVkVxlJWbSQtXPSiUTtC0TjtGOmxa1AJPuV0CPthew==\" crossorigin=\"\"></script>\n"
        "<style>#mapid {height:100%;width:100%}</style></head>\n"
        "<body><div id=\"mapid\"></div>\n"
        "<script type=\"text/javascript\">\n"
        "var mapOptions, mymap, mylayer, mymarker, latlng, myscale, routepolyline\n"
        "function moveMarker(myLat, myLon) {\n"
        "    mymap.panTo(new L.LatLng(myLat, myLon));\n"
        "    mymarker.setLatLng(new L.latLng(myLat, myLon));\n"
        "}\n"
        "function initMap(myLat, myLon, myZoom) {\n"
        "    mapOptions = {\n"
        "    center: [myLat, myLon],\n"
        "    zoom : myZoom,\n"
        "    zoomControl : true,\n"
        "    scrollWheelZoom : false,\n"
        "    dragging : false,\n"
        "    doubleClickZoom : false }\n"
        "    mymap = L.map('mapid', mapOptions);\n"
        "    myscale = L.control.scale().addTo(mymap);\n"
        "    mylayer = new L.tileLayer('" + sBaseUrl + "');\n"
        "    mymap.addLayer(mylayer);\n"
        "}\n"
        "function showMyMarker(myLat, myLon) {\n"
        "    mymarker = new L.marker([myLat, myLon], {\n"
        "    draggable: false,\n"
        "    title : \"GoldenCheetah - Workout LiveMap\",\n"
        "    alt : \"GoldenCheetah - Workout LiveMap\",\n"
        "    riseOnHover : true\n"
        "        }).addTo(mymap);\n"
        "}\n"
        "function centerMap(myLat, myLon, myZoom) {\n"
        "    latlng = L.latLng(myLat, myLon);\n"
        "    mymap.setView(latlng, myZoom)\n"
        "}\n"
        "function showRoute(myRouteLatlngs) {\n"
        "    routepolyline = L.polyline(myRouteLatlngs, { color: 'red' }).addTo(mymap);\n"
        //"    mymap.fitBounds(routepolyline.getBounds());\n"
        "}\n"
        "</script>\n"
        + autoRunJS +
        "</body></html>\n"
    );
}

// This method is called from the HTML page javascript using: webobj.jscallme(#);
void WebClass::jscallme(const QString &datafromjs)
{
    //qDebug() << "INFO: ===> I was called by js function: " << datafromjs;
    switch (datafromjs.toInt()) {
    case 0:
        jsFuncinitMapLoaded = false;
        break;
    case 1:
        jsFuncinitMapLoaded = true;
        break;
    case 2:
        jsFuncmoveMarkerLoaded = true;
        break;
    case 3:
        jsFuncshowMyMarkerLoaded = true;
        break;
    case 4:
        jsFunccenterMapLoaded = true;
        break;
    case 5:
        jsFuncshowRouteLoaded = true;
        break;
    }
    
}

ElevationZoomedMeterWidget::ElevationZoomedMeterWidget(QString Name, QWidget* parent, QString Source, Context* context) : MeterWidget(Name, parent, Source), context(context),
gradientValue(0.)
{
    forceSquareRatio = false;
}

//void ElevationZoomedMeterWidget::stopPlayback()
//{
//
//}

void ElevationZoomedMeterWidget::startPlayback(Context* context) {

    // Nothing to compute unless there is a valid erg file.
    if (!context)
        return;

    ErgFile* ergFile = context->currentErgFile();
    if (!ergFile || !ergFile->isValid())
        return;

    windowWidthMeters = 300; // We are ploting ### meters
    windowHeightMeters = 96; // fixed height shown in graph
    totalRideDistance = floor(ergFile->Duration);
    xScale = -1;
    yScale = -1;
    translateX = 0;
    translateY = 0;
    qSize = windowWidthMeters;
    savedErgFile = ergFile;
    plotQ.clear();

    m_ergFileAdapter.setErgFile(ergFile);
}

void ElevationZoomedMeterWidget::updateRidePointsQ(int tDist) {

    double windowEdge = plotQ.empty() ? 0 : plotQ.last().x();

    // if update is before last element in q then we must reset queue.
    double queryPoint = windowEdge;
    if (plotQ.empty() || (tDist < windowEdge)) {
        m_ergFileAdapter.resetQueryState();
        plotQ.clear();
        queryPoint = tDist - windowWidthMeters;
    }

    int qStep = (int)(windowWidthMeters / qSize);

    queryPoint += qStep;
    while (queryPoint < tDist) {
        int lap;
        geolocation geoloc;
        double slope;
        double tempQP = std::max<double>(0, std::min<double>(totalRideDistance, queryPoint));
        if (m_ergFileAdapter.locationAt(tempQP, lap, geoloc, slope)) {
            if (plotQ.size() == qSize) plotQ.dequeue();
            plotQ.enqueue(QPointF(queryPoint, geoloc.Alt()));
            queryPoint += qStep;
            continue;
        }

        break;
    }
}

void ElevationZoomedMeterWidget::scalePointsToPlot() {

    double pixelX = -1;
    double pixelY = -1;
    m_zoomedElevationPolygon.clear();
    foreach(QPointF p, plotQ) {
        pixelX = ((p.x() - minX) * xScale);
        pixelY = (m_Height - ((p.y() - minY) * yScale));
        m_zoomedElevationPolygon.push_back(QPointF(pixelX, pixelY));
    }
    //Point (0,0) of the graph for the poly left edge
    m_zoomedElevationPolygon.push_front(QPointF((plotQ.front().x() - minX) * xScale, m_Height));
    // Last point of the graph for the poly right edge
    m_zoomedElevationPolygon.push_back(QPointF((plotQ.last().x() - minX) * xScale, m_Height));
    // Translate poly based on the x value of the first point to keep it visable
    if (m_zoomedElevationPolygon.front().x() > 0) {
        translateX = m_zoomedElevationPolygon.front().x() * -1;
    }
}

void ElevationZoomedMeterWidget::calcScaleMultipliers() {
    // Scaling Multiples.
    xScale = m_Width / (maxX - minX);
    yScale = m_Height / windowHeightMeters;
}

void ElevationZoomedMeterWidget::calcMinAndMax()
{
    if (plotQ.empty()) {
        minX = maxX = minY = maxY = 0;
        return;
    }

    minX = plotQ.first().x();
    maxX = plotQ.last().x();
    minY = plotQ[qSize / 3].y() - (windowHeightMeters / 2);
    maxY = minY + windowHeightMeters;
}

//Set bubble color based on % grade

// With cxx17 the class compiles to readonly memory and no template parameters are needed.
// Someday...
#if defined(CXX17)
#define CONSTEXPR      constexpr
#define CONSTEXPR_FUNC constexpr
#else constexpr
#define CONSTEXPR      static const
#define CONSTEXPR_FUNC
#endif

struct RangeColorCriteria {
    double m_point;
    QColor m_color;

    CONSTEXPR_FUNC RangeColorCriteria(double p, QColor c) : m_point(p), m_color(c) {}
};

template <typename T, size_t T_size> struct RangeColorMapper {
    std::array<T, T_size> m_colorMap;

    QColor toColor(double m) const {
        if (m <= m_colorMap[0].m_point) return m_colorMap[0].m_color;

        for (size_t i = 1; i < T_size; i++) {
            if (m < m_colorMap[i].m_point) {
                const RangeColorCriteria& start = m_colorMap[i - 1];
                const RangeColorCriteria& end = m_colorMap[i];

                double unit = (m - start.m_point) / (end.m_point - start.m_point);

                int sh, ss, sv;
                start.m_color.getHsv(&sh, &ss, &sv);

                int eh, es, ev;
                end.m_color.getHsv(&eh, &es, &ev);

                return QColor::fromHsv(sh + unit * (eh - sh),  // lerp
                    ss + unit * (es - ss),  // it
                    sv + unit * (ev - sv)); // real good
            }
        }

        return m_colorMap[T_size - 1].m_color;
    }
};

#ifdef CXX17
// Template deduction guide for RangeColorMapper
template <typename First, typename... Rest> struct EnforceSame {
    static_assert(std::conjunction_v<std::is_same<First, Rest>...>);
    using type = First;
};
template <typename First, typename... Rest> RangeColorMapper(First, Rest...)
->RangeColorMapper2<typename EnforceSame<First, Rest...>::type, 1 + sizeof...(Rest)>;
#endif

CONSTEXPR RangeColorMapper<RangeColorCriteria, 5> s_gradientToColorMapper{
    RangeColorCriteria(-40., Qt::black),
    RangeColorCriteria(0.,   Qt::white),
    RangeColorCriteria(3.,   Qt::yellow),
    RangeColorCriteria(8.,  QColor(255, 140, 0, 255)), // orange
    RangeColorCriteria(40.,  Qt::red)
};

void ElevationZoomedMeterWidget::paintEvent(QPaintEvent* paintevent)
{
    MeterWidget::paintEvent(paintevent);

    if (!context || !context->currentErgFile() || context->currentErgFile()->Points.size() <= 1)
        return;

    m_MainBrush = QBrush(m_MainColor);
    m_BackgroundBrush = QBrush(m_BackgroundColor);
    m_OutlinePen = QPen(m_OutlineColor);
    m_OutlinePen.setWidth(1);
    m_OutlinePen.setStyle(Qt::SolidLine);
    currDist = this->Value * 1000.0;

    // Add more points to the queue
    int targetDist = currDist + (windowWidthMeters *2  / 3);
    updateRidePointsQ(targetDist);
    calcMinAndMax();
    calcScaleMultipliers();
    scalePointsToPlot();

    // Cyclist position 
    cyclistX = (currDist - minX) * xScale;

    //painter
    QPainter painter(this);
    painter.setClipRegion(videoContainerRegion);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(translateX, translateY);
    painter.setPen(m_OutlinePen);
    painter.setBrush(m_BackgroundBrush);
    painter.drawPolygon(m_zoomedElevationPolygon);
    m_OutlinePen = QPen(m_MainColor);
    m_OutlinePen.setWidth(1);
    m_OutlinePen.setStyle(Qt::SolidLine);
    painter.setPen(m_OutlinePen);
    painter.drawLine(cyclistX, 0.0, cyclistX, (double)m_Height);
    
    //Create Bubble painter to show the grade and change color
    QPainter bubblePainter(this);
    bubblePainter.setClipRegion(videoContainerRegion);
    bubblePainter.setRenderHint(QPainter::Antialiasing);

    // Set bubble painter pen and brush
    QPen bubblePen;
    bubblePen.setColor(Qt::black);
    bubblePen.setWidth(3);
    bubblePen.setStyle(Qt::SolidLine);
    bubblePainter.setPen(bubblePen);

    double gradient = std::min<double>(40, std::max<double>(-40, this->gradientValue));
    QColor bubbleColor = s_gradientToColorMapper.toColor(gradient);
    bubblePainter.setBrush(bubbleColor);

    // Draw bubble
    bubbleSize = (double)m_Height * 0.010f;
    double cyclistY = 0.0;
    cyclistY = (double)m_Height;

    double cyclistCircleRadius = (double)m_Height / 4;
    // Find bubble size that intersects altitude line and rider.
    for (int idx = (m_zoomedElevationPolygon.size() + 1) / 2;
        idx < m_zoomedElevationPolygon.size();
        idx++) {
        double searchDist = m_zoomedElevationPolygon[idx].x() - cyclistX;
        double radius = (m_Height - m_zoomedElevationPolygon[idx].y()) / 2.;

        // we wish a circle where radius is ~= distance.
        if (searchDist >= radius) {
            cyclistCircleRadius = radius;
            break;
        }
    }
    double bubbleX = cyclistX + cyclistCircleRadius;
    bubblePainter.drawEllipse(QPointF(bubbleX, cyclistY - cyclistCircleRadius), (qreal)cyclistCircleRadius, (qreal)cyclistCircleRadius);

    // Display grade as #.#%
    QString gradientString = ((-1.0 < this->gradientValue && this->gradientValue < 0.0) ? QString("-") : QString("")) + QString::number((int)this->gradientValue) +
        QString(".") + QString::number(abs((int)(this->gradientValue * 10.0) % 10)) + QString("%");

    // Display gradient text in the bubble
    bubblePainter.drawText(bubbleX - 15 , (cyclistY - (cyclistCircleRadius - 5)), gradientString);
}
