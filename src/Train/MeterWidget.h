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

#ifndef _MeterWidget_h
#define _MeterWidget_h 1

#include <QWidget>
#include "Context.h"
#include <QtWebChannel>
#include <QWebEnginePage>
#include <QWebEngineView>
#include <qwt_compass.h>

class MeterWidget : public QWidget
{
    Q_OBJECT

  public:
    explicit MeterWidget(QString name, QWidget *parent = 0, QString Source = QString("None"));
    QString Source() const { return m_Source; };
    QString Name() const { return m_Name; };
    int     Width() const { return m_Width; };
    int     Height() const { return m_Height; };
    int     PosX() const { return m_PosX; };
    int     PosY() const { return m_PosY; };
    float   RelativeWidth() const { return m_RelativeWidth; };
    float   RelativeHeight() const { return m_RelativeHeight; };
    float   RelativePosX() const { return m_RelativePosX; };
    float   RelativePosY() const { return m_RelativePosY; };
    QWidget* container() const { return m_container; };
    virtual void SetRelativeSize(float RelativeWidth = 100.0, float RelativeHeight = 100.0);
    virtual void SetRelativePos(float RelativePosX = 50.0, float RelativePosY = 50.0);
    virtual void AdjustSizePos();
    virtual void ComputeSize();
    virtual void paintEvent(QPaintEvent* paintevent);
    virtual void startPlayback(Context* context);
    virtual void stopPlayback();
    virtual QSize sizeHint() const;
    virtual QSize minimumSize() const;
    void    setColor(QColor  mainColor);
    void    setBoundingRectVisibility(bool show, QColor  boundingRectColor = QColor(255,0,0,255));

    float Value, ValueMin, ValueMax;
    QString Text, AltText, AltTextSuffix;
    Qt::Alignment alignment;
    int textWidth;

  protected:
    QString  m_Name;
    QWidget* m_container;
    QString  m_Source;
    QWidget* m_VideoContainer;
    QRegion  videoContainerRegion;
    float    m_RelativeWidth, m_RelativeHeight;
    float    m_RelativePosX, m_RelativePosY;
    int      m_PosX, m_PosY, m_Width, m_Height;
    float    m_RangeMin, m_RangeMax;
    float    m_Angle;
    int      m_SubRange;
    bool     forceSquareRatio;

    QColor  m_MainColor;
    QColor  m_ScaleColor;
    QColor  m_OutlineColor;
    QColor  m_BackgroundColor;
    QColor  m_BoundingRectColor;
    QFont   m_MainFont;
    QFont   m_AltFont;

    QBrush m_MainBrush;
    QBrush m_BackgroundBrush;
    QBrush m_AltBrush;
    QPen   m_OutlinePen;
    QPen   m_ScalePen;

    bool    backgroundVisibility;
    bool    boundingRectVisibility;

    friend class VideoLayoutParser;
};

class TextMeterWidget : public MeterWidget
{
  public:
    explicit TextMeterWidget(QString name, QWidget *parent = 0, QString Source = QString("None"));
    virtual void paintEvent(QPaintEvent* paintevent);
};

class CircularIndicatorMeterWidget : public MeterWidget
{
  public:
    explicit CircularIndicatorMeterWidget(QString name, QWidget *parent = 0, QString Source = QString("None"));
    virtual void paintEvent(QPaintEvent* paintevent);
    QConicalGradient IndicatorGradient;
};

class CircularBargraphMeterWidget : public MeterWidget
{
  public:
    explicit CircularBargraphMeterWidget(QString name, QWidget *parent = 0, QString Source = QString("None"));
    virtual void paintEvent(QPaintEvent* paintevent);
};

class NeedleMeterWidget : public MeterWidget
{
  public:
    explicit NeedleMeterWidget(QString name, QWidget *parent = 0, QString Source = QString("None"));
    virtual void paintEvent(QPaintEvent* paintevent);
};

class CompassWidget : public MeterWidget
{
  private:
    QwtCompass  m_Compass;
  public:
    explicit CompassWidget(QString name, QWidget *parent = 0, QString Source = QString("None"));
    virtual void paintEvent(QPaintEvent* paintevent);
};



class ElevationMeterWidget : public MeterWidget
{
    Q_OBJECT

    Context* context;
    QPolygon m_elevationPolygon;
    double m_minX, m_maxX;
    int m_savedWidth, m_savedHeight, m_savedMinY, m_savedMaxY;

  protected:
    virtual void paintEvent(QPaintEvent* paintevent);

    void lazySetup(void);

  public:
    explicit ElevationMeterWidget(QString name, QWidget *parent = 0, QString Source = QString("None"), Context *context = NULL);
    void setContext(Context* context) { this->context = context; }
    float gradientValue;
};

// Connect JS with C++
class WebClass : public QObject
{
    Q_OBJECT
    //Q_PROPERTY(QString jsFuncName MEMBER m_jsFuncName)
public slots:
    Q_INVOKABLE void jscallme(const QString &datafromjs);
    //Q_INVOKABLE void jscallme();

public:
    QString jsFuncReturn;
    bool jsFuncinitMapLoaded, jsFuncmoveMarkerLoaded, jsFuncshowMyMarkerLoaded, jsFunccenterMapLoaded, jsFuncshowRouteLoaded = false;
};

// Display live map as overlay on video
class LiveMapWidget : public MeterWidget
{
    Q_OBJECT
    Context* context;

public:
    explicit LiveMapWidget(QString name, QWidget* parent = 0, QString Source = QString("None"), Context* context = NULL);
    ~LiveMapWidget();
    virtual void startPlayback(Context* context);
    virtual void stopPlayback();
    void setContext(Context* context) { this->context = context; }
    void plotNewLatLng(double newLat, double newLong);
    void initLiveMap(Context* context);
    void loadingLiveMap();
    int  m_Zoom;
    QString  m_osmURL;

private slots:

protected:
    void createHtml(QString sBaseUrl, QString autoRunJS);
    void buildRouteArrayLatLngs(Context* context);
    void resizeEvent(QResizeEvent*);
    QWebEngineView *liveMapView;
    QWebEnginePage *webPage;
    WebClass *webobj;
    QWebChannel *channel;

    QString routeLatLngs;
    QString currentPage;
    bool routeInitialized, mapInitialized;
   
};

#endif // _MeterWidget_h
