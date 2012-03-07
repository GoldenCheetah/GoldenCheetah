/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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


#include "ErgFilePlot.h"

// Bridge between QwtPlot and ErgFile to avoid having to
// create a separate array for the ergfile data, we plot
// directly from the ErgFile points array
double ErgFileData::x(size_t i) const { 
    if (main->currentErgFile()) return main->currentErgFile()->Points.at(i).x;
    else return 0;
}

double ErgFileData::y(size_t i) const {
    if (main->currentErgFile()) return main->currentErgFile()->Points.at(i).y;
    else return 0;
}

size_t ErgFileData::size() const {
    if (main->currentErgFile()) return main->currentErgFile()->Points.count();
    else return 0;
}

QPointF ErgFileData::sample(size_t i) const
{
    return QPointF(x(i), y(i));
}

QRectF ErgFileData::boundingRect() const
{
    if (main->currentErgFile()) {
        double minX, minY, maxX, maxY;
        minX=minY=maxX=maxY=0.0f;
        foreach(ErgFilePoint x, main->currentErgFile()->Points) {
            if (x.y > maxY) maxY = x.y;
            if (x.x > maxX) maxX = x.x;
            if (x.y < minY) minY = x.y;
            if (x.x < minX) minX = x.x;
        }
        maxY *= 1.3f; // always need a bit of headroom
        return QRectF(minX, minY, maxX, maxY);
    }
    return QRectF(0,0,0,0);
}


// Now bar
double NowData::x(size_t) const { return main->getNow(); }
double NowData::y(size_t i) const {
    if (i) {
        if (main->currentErgFile()) return main->currentErgFile()->maxY;
        else return 0;
    } else return 0;
}
size_t NowData::size() const { return 2; }


QPointF NowData::sample(size_t i) const
{
    return QPointF(x(i), y(i));
}

/*QRectF NowData::boundingRect() const
{
    // TODO dgr
    return QRectF(0, 0, 0, 0);
}*/

ErgFilePlot::ErgFilePlot(MainWindow *main) : main(main)
{
    setInstanceName("ErgFile Plot");

    //insertLegend(new QwtLegend(), QwtPlot::BottomLegend);
    setCanvasBackground(GColor(CRIDEPLOTBACKGROUND));
    canvas()->setFrameStyle(QFrame::NoFrame);
    //courseData = data;                      // what we plot

    // Setup the left axis (Power)
    setAxisTitle(yLeft, "Watts");
    enableAxis(yLeft, true);
    QwtScaleDraw *sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    setAxisMaxMinor(yLeft, 0);
    //setAxisScaleDraw(QwtPlot::yLeft, sd);

    QPalette pal;
    pal.setColor(QPalette::WindowText, GColor(CPOWER));
    pal.setColor(QPalette::Text, GColor(CPOWER));
    axisWidget(QwtPlot::yLeft)->setPalette(pal);

    QFont stGiles;
    stGiles.fromString(appsettings->value(this, GC_FONT_CHARTLABELS, QFont().toString()).toString());
    stGiles.setPointSize(appsettings->value(NULL, GC_FONT_CHARTLABELS_SIZE, 8).toInt());
    QwtText title("Watts");
    title.setFont(stGiles);
    QwtPlot::setAxisFont(yLeft, stGiles);
    QwtPlot::setAxisTitle(yLeft, title);

    enableAxis(xBottom, true);
    distdraw = new DistScaleDraw;
    distdraw->setTickLength(QwtScaleDiv::MajorTick, 3);
    timedraw = new HourTimeScaleDraw;
    timedraw->setTickLength(QwtScaleDiv::MajorTick, 3);
    setAxisMaxMinor(xBottom, 0);
    setAxisScaleDraw(QwtPlot::xBottom, timedraw);

    // set the axis so we default to an hour workout
    setAxisScale(xBottom, (double)0, 1000 * 60  * 60 , 15 * 60 * 1000);

    title.setFont(stGiles);
    title.setText("Time (mins)");
    QwtPlot::setAxisFont(xBottom, stGiles);
    QwtPlot::setAxisTitle(xBottom, title);

    pal.setColor(QPalette::WindowText, Qt::blue);
    pal.setColor(QPalette::Text, Qt::blue);
    axisWidget(QwtPlot::xBottom)->setPalette(pal);


    // set all the orher axes off but scaled
    setAxisScale(yLeft, 0, 300); // max cadence and hr
    enableAxis(yLeft, true);
    setAxisAutoScale(QwtPlot::yLeft, true);// we autoscale, since peaks are so much higher than troughs

    setAxisScale(yRight, 0, 250); // max cadence and hr
    enableAxis(yRight, false);
    setAxisScale(yRight2, 0, 60); // max speed of 60mph/60kmh seems ok to me!
    enableAxis(yRight2, false);

    // data bridge to ergfile
    lodData = new ErgFileData(main);
    // Load Curve
    LodCurve = new QwtPlotCurve("Course Load");
    LodCurve->setData(lodData);
    LodCurve->attach(this);
    LodCurve->setBaseline(-1000);
    LodCurve->setYAxis(QwtPlot::yLeft);

    // load curve is blue for time and grey for gradient
    QColor brush_color = QColor(Qt::blue);
    brush_color.setAlpha(64);
    LodCurve->setBrush(brush_color);   // fill below the line
    QPen Lodpen = QPen(Qt::blue, 1.0);
    LodCurve->setPen(Lodpen);

    // telemetry history
    wattsCurve = new QwtPlotCurve("Power");
    QPen wattspen = QPen(GColor(CPOWER));
    wattsCurve->setPen(wattspen);
    wattsCurve->attach(this);
    wattsCurve->setYAxis(QwtPlot::yLeft);
    // dgr wattsCurve->setPaintAttribute(QwtPlotCurve::PaintFiltered);
    wattsData = new CurveData;
    wattsCurve->setData(wattsData->x(), wattsData->y(), wattsData->count());

    // telemetry history
    hrCurve = new QwtPlotCurve("Heartrate");
    QPen hrpen = QPen(GColor(CHEARTRATE));
    hrCurve->setPen(hrpen);
    hrCurve->attach(this);
    hrCurve->setYAxis(QwtPlot::yRight);
    hrData = new CurveData;
    hrCurve->setData(hrData->x(), hrData->y(), hrData->count());

    // telemetry history
    cadCurve = new QwtPlotCurve("Cadence");
    QPen cadpen = QPen(GColor(CCADENCE));
    cadCurve->setPen(cadpen);
    cadCurve->attach(this);
    cadCurve->setYAxis(QwtPlot::yRight);
    cadData = new CurveData;
    cadCurve->setData(cadData->x(), cadData->y(), cadData->count());

    // telemetry history
    speedCurve = new QwtPlotCurve("Speed");
    QPen speedpen = QPen(GColor(CSPEED));
    speedCurve->setPen(speedpen);
    speedCurve->attach(this);
    speedCurve->setYAxis(QwtPlot::yRight2);
    speedData = new CurveData;
    speedCurve->setData(speedData->x(), speedData->y(), speedData->count());

    // Now data bridge
    nowData = new NowData(main);

    // Now pointer
    NowCurve = new QwtPlotCurve("Now");
    QPen Nowpen = QPen(Qt::red, 2.0);
    NowCurve->setPen(Nowpen);
    NowCurve->setData(nowData);
    NowCurve->attach(this);
    NowCurve->setYAxis(QwtPlot::yLeft);

    bydist = false;
    ergFile = NULL;

    setAutoReplot(false);
}

void
ErgFilePlot::setData(ErgFile *ergfile)
{
    reset();

    ergFile = ergfile;
    // clear the previous marks (if any)
    for(int i=0; i<Marks.count(); i++) {
        Marks.at(i)->detach();
        delete Marks.at(i);
    }
    Marks.clear();

    // axis fonts
    QFont stGiles;
    stGiles.fromString(appsettings->value(this, GC_FONT_CHARTLABELS, QFont().toString()).toString());
    stGiles.setPointSize(appsettings->value(NULL, GC_FONT_CHARTLABELS_SIZE, 8).toInt());
    QPalette pal;

    if (ergfile) {

        // is this by distance or time?
        bydist = (ergfile->format == CRS) ? true : false;

        if (bydist == true) {

            QColor brush_color = QColor(Qt::gray);
            brush_color.setAlpha(64);
            LodCurve->setBrush(brush_color);   // fill below the line
            QPen Lodpen = QPen(Qt::gray, 1.0);
            LodCurve->setPen(Lodpen);

        } else {

            QColor brush_color = QColor(Qt::blue);
            brush_color.setAlpha(64);
            LodCurve->setBrush(brush_color);   // fill below the line
            QPen Lodpen = QPen(Qt::blue, 1.0);
            LodCurve->setPen(Lodpen);

        }

        // set up again
        for(int i=0; i < ergFile->Laps.count(); i++) {

            // Show Lap Number
            QwtText text(ergFile->Laps.at(i).name != "" ? ergFile->Laps.at(i).name : QString::number(ergFile->Laps.at(i).LapNum));
            text.setFont(QFont("Helvetica", 10, QFont::Bold));
            text.setColor(GColor(CPLOTMARKER));

            // vertical line
            QwtPlotMarker *add = new QwtPlotMarker();
            add->setLineStyle(QwtPlotMarker::VLine);
            add->setLinePen(QPen(GColor(CPLOTMARKER), 0, Qt::DashDotLine));
            add->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
            add->setValue(ergFile->Laps.at(i).x, 0.0);
            add->setLabel(text);
            add->attach(this);

            Marks.append(add);
        }

        // set the axis so we use all the screen estate
        if (main->currentErgFile() && main->currentErgFile()->Points.count()) {
            double maxX = (double)main->currentErgFile()->Points.last().x;

            if (bydist) {

                // tics every 5 kilometer, if workout shorter tics every 1000m
                double step = 5000;
                if (maxX <= 1000) step = 100;
                else if (maxX < 5000) step = 1000;

                // axis setup for distance
                setAxisScale(xBottom, (double)0, maxX, step);

                QwtText title;
                title.setFont(stGiles);
                title.setText("Distance (km)");
                QwtPlot::setAxisFont(xBottom, stGiles);
                QwtPlot::setAxisTitle(xBottom, title);

                pal.setColor(QPalette::WindowText, Qt::gray);
                pal.setColor(QPalette::Text, Qt::gray);
                axisWidget(QwtPlot::xBottom)->setPalette(pal);

                // only allocate a new one if its not the current (they get freed by Qwt)
                if (axisScaleDraw(xBottom) != distdraw)
                    setAxisScaleDraw(QwtPlot::xBottom, (distdraw=new DistScaleDraw()));

            } else {

                // tics every 15 minutes, if workout shorter tics every minute
                setAxisScale(xBottom, (double)0, maxX, maxX > (15*60*1000) ? 15*60*1000 : 60*1000);

                QwtText title;
                title.setFont(stGiles);
                title.setText("Time (mins)");
                QwtPlot::setAxisFont(xBottom, stGiles);
                QwtPlot::setAxisTitle(xBottom, title);

                pal.setColor(QPalette::WindowText, Qt::blue);
                pal.setColor(QPalette::Text, Qt::blue);
                axisWidget(QwtPlot::xBottom)->setPalette(pal);

                // only allocate a new one if its not the current (they get freed by Qwt)
                if (axisScaleDraw(xBottom) != timedraw)
                    setAxisScaleDraw(QwtPlot::xBottom, (timedraw=new HourTimeScaleDraw()));
            }
        }

    } else {

        // clear the plot we have nothing selected
        bydist = false; // do by time when no workout selected

        QwtText title;
        title.setFont(stGiles);
        title.setText("Time (mins)");
        QwtPlot::setAxisFont(xBottom, stGiles);
        QwtPlot::setAxisTitle(xBottom, title);

        pal.setColor(QPalette::WindowText, Qt::blue);
        pal.setColor(QPalette::Text, Qt::blue);
        axisWidget(QwtPlot::xBottom)->setPalette(pal);

        // set the axis so we default to an hour workout
        if (axisScaleDraw(xBottom) != timedraw)
            setAxisScaleDraw(QwtPlot::xBottom, (timedraw=new HourTimeScaleDraw()));
        setAxisScale(xBottom, (double)0, 1000 * 60 * 60, 15*60*1000);
    }
}

void
ErgFilePlot::setNow(long msecs)
{
    replot(); // and update
}

void
ErgFilePlot::performancePlot(RealtimeData rtdata)
{
    // we got some data
    // x is plotted in meters or micro-seconds
    double x = bydist ? (rtdata.getDistance() * 1000) : rtdata.getMsecs();

    // when not using a workout we need to extend the axis when we
    // go out of bounds -- we do not use autoscale for x, because we
    // want to control stepping and tick marking add another 30 mins
    if (!ergFile && axisScaleDiv(xBottom)->upperBound() <= x) {
        double maxX = x + ( 30 * 60 * 1000);
        setAxisScale(xBottom, (double)0, maxX, maxX > (15*60*1000) ? 15*60*1000 : 60*1000);
    }

    double watts = rtdata.getWatts();
    double speed = rtdata.getSpeed();
    double cad = rtdata.getCadence();
    double hr = rtdata.getHr();

    wattssum += watts;
    hrsum += hr;
    cadsum += cad;
    speedsum += speed;

    if (counter < 25) {
        counter++;
        return;
    } else {
        watts = wattssum / 26;
        hr = hrsum / 26;
        cad = cadsum / 26;
        speed = speedsum / 26;
        counter=0;
        wattssum = hrsum = cadsum = speedsum = 0;
    }

    double zero = 0;

    if (!wattsData->count()) wattsData->append(&zero, &watts, 1);
    wattsData->append(&x, &watts, 1);
    wattsCurve->setData(wattsData->x(), wattsData->y(), wattsData->count());

    if (!hrData->count()) hrData->append(&zero, &hr, 1);
    hrData->append(&x, &hr, 1);
    hrCurve->setData(hrData->x(), hrData->y(), hrData->count());

    if (!speedData->count()) speedData->append(&zero, &speed, 1);
    speedData->append(&x, &speed, 1);
    speedCurve->setData(speedData->x(), speedData->y(), speedData->count());

    if (!cadData->count()) cadData->append(&zero, &cad, 1);
    cadData->append(&x, &cad, 1);
    cadCurve->setData(cadData->x(), cadData->y(), cadData->count());

    //const bool cacheMode = canvas()->testPaintAttribute(QwtPlotCanvas::PaintCached);

#if 0
#if QT_VERSION >= 0x040000 && defined(Q_WS_X11)
    // Even if not recommended by TrollTech, Qt::WA_PaintOutsidePaintEvent 
    // works on X11. This has an tremendous effect on the performance..

    canvas()->setAttribute(Qt::WA_PaintOutsidePaintEvent, true);
#endif

    canvas()->setPaintAttribute(QwtPlotCanvas::PaintCached, false);
    wattsCurve->draw(0, wattsCurve->dataSize() - 1);
    hrCurve->draw(0, hrCurve->dataSize() - 1);
    cadCurve->draw(0, cadCurve->dataSize() - 1);
    speedCurve->draw(0, speedCurve->dataSize() - 1);
    canvas()->setPaintAttribute(QwtPlotCanvas::PaintCached, cacheMode);

#if QT_VERSION >= 0x040000 && defined(Q_WS_X11)
    canvas()->setAttribute(Qt::WA_PaintOutsidePaintEvent, false);
#endif
#endif

}

void
ErgFilePlot::start()
{
    reset();
}

void
ErgFilePlot::reset()
{
    // reset data
    counter = hrsum = wattssum = speedsum = cadsum = 0;

    // note the origin of the data is not a point 0, but the first
    // average over 5 seconds. this leads to a small gap on the left
    // which is better than the traces all starting from 0,0 which whilst
    // is factually correct, does not tell us anything useful and look horrid.
    // instead when we place the first points on the plots we add them twice
    // once for time/distance of 0 and once for the current point in time
    wattsData->clear();
    wattsCurve->setData(wattsData->x(), wattsData->y(), wattsData->count());
    cadData->clear();
    cadCurve->setData(cadData->x(), cadData->y(), cadData->count());
    hrData->clear();
    hrCurve->setData(hrData->x(), hrData->y(), hrData->count());
    speedData->clear();
    speedCurve->setData(speedData->x(), speedData->y(), speedData->count());
}

// curve data.. code snaffled in from the Qwt example (realtime_plot)
CurveData::CurveData(): d_count(0) { }

void CurveData::append(double *x, double *y, int count)
{
    int newSize = ((d_count + count) / 1000 + 1 ) * 1000;
    if (newSize > size()) {
        d_x.resize(newSize);
        d_y.resize(newSize);
    }

    for (register int i = 0; i < count; i++) {
        d_x[d_count + i] = x[i];
        d_y[d_count + i] = y[i];
    }
    d_count += count;
}

int CurveData::count() const
{
    return d_count;
}

int CurveData::size() const
{
    return d_x.size();
}

const double *CurveData::x() const
{
    return d_x.data();
}

const double *CurveData::y() const
{
    return d_y.data();
}

void CurveData::clear()
{
    d_count = 0;
    d_x.clear();
    d_y.clear();
}
