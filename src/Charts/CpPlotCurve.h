#ifndef CpPlotCurve_H
#define CpPlotCurve_H

#include "qwt_global.h"
#include "qwt_plot_seriesitem.h"
#include "qwt_series_data.h"

class QwtSymbol;
class QwtColorMap;

class CpPlotCurve: public QwtPlotSeriesItem, QwtSeriesStore<QwtPoint3D>
{
public:
    //! Paint attributes
    enum PaintAttribute
    {
        //! Clip points outside the canvas rectangle
        ClipPoints = 0x01,

        /*!
          Clip polygons before painting them. In situations, where points
          are far outside the visible area (f.e when zooming deep) this
          might be a substantial improvement for the painting performance
         */
        ClipPolygons = 0x02,

        /*!
          Tries to reduce the data that has to be painted, by sorting out
          duplicates, or paintings outside the visible area. Might have a
          notable impact on curves with many close points.
          Only a couple of very basic filtering algorithms are implemented.
         */
        FilterPoints = 0x04
    };

    //! Paint attributes
    typedef QFlags<PaintAttribute> PaintAttributes;

    explicit CpPlotCurve( const QString &title = QString() );
    explicit CpPlotCurve( const QwtText &title );

    virtual ~CpPlotCurve();

    virtual int rtti() const;

    void setPaintAttribute( PaintAttribute, bool on = true );
    bool testPaintAttribute( PaintAttribute ) const;

    void setSamples( const QVector<QwtPoint3D> & );
    void setSamples( QwtSeriesData<QwtPoint3D> * );


    void setColorMap( QwtColorMap * );
    const QwtColorMap *colorMap() const;

    void setColorRange( const QwtInterval & );
    QwtInterval & colorRange() const;

    virtual void drawSeries( QPainter *,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        const QRectF &canvasRect, int from, int to ) const;

    void setPenWidth(double width);
    double penWidth() const;

protected:
    virtual void drawDots( QPainter *,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        const QRectF &canvasRect, int from, int to ) const;

    virtual void drawLines( QPainter *,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        const QRectF &canvasRect, int from, int to ) const;

private:
    void init();

    class PrivateData;
    PrivateData *d_data;
};

#endif
