/*
 * Copyright (c) 2014 Joern Rischmueller (joern.rm@gmail.com)
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



#include "AllPlotSlopeCurve.h"
#include "Colors.h"

#include "qwt_point_data.h"
#include "qwt_painter.h"
#include "qwt_plot.h"
#include "qwt_symbol.h"


class AllPlotSlopeCurve::PrivateData
{
public:
    PrivateData():
        style( AllPlotSlopeCurve::SlopeDist1 ),
        baseline( 0.0 )

    {     brushes.resize(10);
          brushes[0].setColor (QColor(0,255,0));
          brushes[0].setStyle(Qt::SolidPattern);
          brushes[1].setColor (QColor(0,0,255));
          brushes[1].setStyle(Qt::SolidPattern);
          brushes[2].setColor (QColor(255,255,0));
          brushes[2].setStyle(Qt::SolidPattern);
          brushes[3].setColor (QColor(255,0,0));
          brushes[3].setStyle(Qt::SolidPattern);
          brushes[4].setColor (QColor(255,0,0));
          brushes[4].setStyle(Qt::SolidPattern);
          brushes[5].setColor (QColor(127,127,127));
          brushes[5].setStyle(Qt::SolidPattern);
          brushes[6].setColor (QColor(127,127,127));
          brushes[6].setStyle(Qt::SolidPattern);
          brushes[7].setColor (QColor(127,127,127));
          brushes[7].setStyle(Qt::SolidPattern);
          brushes[8].setColor (QColor(127,127,127));
          brushes[8].setStyle(Qt::SolidPattern);
          brushes[9].setColor (QColor(127,127,127));
          brushes[9].setStyle(Qt::SolidPattern);
}

    ~PrivateData()
    {
      brushes.clear();
    }

    AllPlotSlopeCurve::CurveStyle style;
    double baseline;
    QVector<QBrush> brushes;

};


// create new AllPlotSlopeCurve
AllPlotSlopeCurve::AllPlotSlopeCurve( const QString &title ):
    QwtPlotCurve( QwtText( title ) )
{
    init();
}

// Destructor
AllPlotSlopeCurve::~AllPlotSlopeCurve()
{
    delete d_data;
}

// Initialize internal members
void AllPlotSlopeCurve::init()
{

    d_data = new PrivateData;
    setData( new QwtPointSeriesData() );

    setZ( 25.0 );
}

int AllPlotSlopeCurve::rtti() const
{
    return Rtti_PlotCurve ; // same as plot curve
}

void AllPlotSlopeCurve::setStyle( CurveStyle style )
{
    if ( style != d_data->style )
    {
        d_data->style = style;

        legendChanged();
        itemChanged();
    }
}

AllPlotSlopeCurve::CurveStyle AllPlotSlopeCurve::style() const
{
    return d_data->style;
}

// draw the ALT/SLOPE curve
void AllPlotSlopeCurve::drawCurve( QPainter *painter, int,
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    const QRectF &, int from, int to ) const
{

    const QwtSeriesData<QPointF> *series = data();

    // parameter (will move to data

    // use sensible defaults
    double section_delta = 0.1;
    bool byDistance = true;

    switch (d_data->style) {

    // time-section is defined in minutes, distance-section in km
    case SlopeTime1 : { section_delta = 1.0; byDistance = false; break; }
    case SlopeTime2 : { section_delta = 5.0; byDistance = false; break; }
    case SlopeTime3 : { section_delta = 10.0; byDistance = false; break; }
    case SlopeDist1 : { section_delta = 0.1; byDistance = true; break; }
    case SlopeDist2 : { section_delta = 0.5; byDistance = true; break; }
    case SlopeDist3 : { section_delta = 1; byDistance = true; break; }
    }

    // create single polygons to be painted (in different colors depending on slope)
    QList<QPolygonF*> polygons;
    // store the polygon edge points (original coordinates) for slope/distance & m/time calculation
    QList<QPointF> calcPoints;

    // prepare Y-Axis baseline info for painting the polygon
    double baseline = d_data->baseline;
    if ( yMap.transformation() )
        baseline = yMap.transformation()->bounded( baseline );
    double refY = yMap.transform( baseline );
    double sectionStart = 0.0;
    QPolygonF *polygon;
    QPointF *points = NULL;
    for (int i = from; i <= to; i++ ) {
        const QPointF sample = series->sample( i );
        if (i == from) {
            // first polygon
            polygon = new QPolygonF (4);
            points = polygon->data();
            sectionStart = sample.x();
            double xi = xMap.transform( sample.x() );
            double yi = yMap.transform( sample.y() );
            points[0].rx() = xi;
            points[0].ry() = refY;
            points[1].rx() = xi;
            points[1].ry() = yi;
            // first point for slope/mperh calculation
            QPointF calcPoint;
            calcPoint.rx() = sample.x();
            calcPoint.ry() = sample.y();
            calcPoints.append(calcPoint);
        };

        // we are in a section - so search for the end and if found close polygon
        if (points && sample.x() >= (sectionStart+section_delta)) {
            // we are at the end - close and create polygon and go to next
            double xi = xMap.transform( sample.x() );
            double yi = yMap.transform( sample.y() );
            points[2].rx() = xi;
            points[2].ry() = yi;
            points[3].rx() = xi;
            points[3].ry() = refY;
            // append to list
            polygons.append(polygon);
            // next point for slope/mperh calculation
            QPointF calcPoint;
            calcPoint.rx() = sample.x();
            calcPoint.ry() = sample.y();
            calcPoints.append(calcPoint);

            // start the next polygon with the SAME point than the previous one to have a step-free graph
            polygon = new QPolygonF (4);
            points = polygon->data();
            sectionStart = sample.x();
            double xi2 = xMap.transform( sample.x() );
            double yi2 = yMap.transform( sample.y() );
            points[0].rx() = xi2;
            points[0].ry() = refY;
            points[1].rx() = xi2;
            points[1].ry() = yi2;
        }
        // last started polygon is not closed and painted by intent since it would be smaller than then the others
    }

    // paint the polygons & text per polygon
    int i = 0;
    foreach (QPolygonF *p, polygons) {

        double slope=0.0f; // slope of a section (byDistance = true)
        double mperh=0.0f; // meter per hour (climb or descent) (byDistance = false)
        QPointF point1 = calcPoints.at(i);
        QPointF point2 = calcPoints.at(i+1);

        QBrush brush;
        if (byDistance) {
            // if Y-Axis did not change, no calculation
            // distance - X-Axis is in KM, Y-Axis in m ! and at the end *100 to get %value
            if (point2.ry() != point1.ry()) {
                slope = 100 * ((point2.ry() - point1.ry()) / ((point2.rx() - point1.rx())*1000));
            } else {
                slope = 0.0;
            }
            // set the brush
            if (slope >= 0 && slope < 5) brush = d_data->brushes[0];
            if (slope >= 4 && slope < 7) brush = d_data->brushes[1];
            if (slope >= 7 && slope < 10) brush = d_data->brushes[2];
            if (slope >= 10 && slope < 15) brush = d_data->brushes[3];
            if (slope >= 15) brush = d_data->brushes[4];
            if (slope < 0 && slope > -2) brush = d_data->brushes[5];
            if (slope <= -2 && slope > -5) brush = d_data->brushes[6];
            if (slope <= -5 && slope > -9) brush = d_data->brushes[7];
            if (slope <= -9 && slope > -15) brush = d_data->brushes[8];
            if (slope <= -15) brush = d_data->brushes[5];
        } else {
            // if Y-Axis did not change, no calculation
            // distance - X-Axis is in min, Y-Axis in m !
            if (point2.ry() != point1.ry()) {
                mperh = 60 * ((point2.ry() - point1.ry()) / (point2.rx() - point1.rx()));
            } else {
                mperh = 0.0;
            }
            // set the brush
            if (mperh >= 0 && mperh < 100) brush = d_data->brushes[0];
            if (mperh >= 100 && mperh < 200) brush = d_data->brushes[1];
            if (mperh >= 200 && mperh < 300) brush = d_data->brushes[2];
            if (mperh >= 300 && mperh < 500) brush = d_data->brushes[3];
            if (mperh >= 500) brush = d_data->brushes[4];
            if (mperh < 0 && mperh > -100) brush = d_data->brushes[5];
            if (mperh <= -100 && mperh > -200) brush = d_data->brushes[6];
            if (mperh <= -200 && mperh > -300) brush = d_data->brushes[7];
            if (mperh <= -300 && mperh > -500) brush = d_data->brushes[8];
            if (mperh <= -500) brush = d_data->brushes[5];

        };
        painter->setPen(QColor(127,127,127));
        painter->setBrush( brush );
        // paint the polygon
        QwtPainter::drawPolygon( painter, *p );

        // determine Y-Width of polygon / don't show text if too small
        if (p->at(3).x() - p->at(0).x() > 25) {

            // draw the text (find the point, draw the text)
            QPointF pText = p->at(1);
            if (p->at(1).y() >= p->at(2).y()) pText.setY(p->at(2).y()); else pText.setY(p->at(1).y());
            pText.rx() +=5.0;
            pText.ry() -=30.0;
            QString text;
            if (byDistance) {
                text.setNum(slope, 'f', 2);
            } else {
                text.setNum(mperh, 'f', 0);
            }
            painter->setPen(GCColor::invertColor(GColor(CPLOTBACKGROUND)));
            painter->setFont(QFont("Helvetica",8));
            QwtPainter::drawText(painter, pText, text );
        }

        i++;
    }

}
