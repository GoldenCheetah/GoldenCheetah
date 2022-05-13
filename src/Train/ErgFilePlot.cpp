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
#include "WPrime.h"
#include "Context.h"
#include "Units.h"

#include <unordered_map>

// Bridge between QwtPlot and ErgFile to avoid having to
// create a separate array for the ergfile data, we plot
// directly from the ErgFile points array
double ErgFileData::x(size_t i) const {
    if (context->currentErgFile()) return context->currentErgFile()->Points.at(i).x;
    else return 0;
}

double ErgFileData::y(size_t i) const {
    if (context->currentErgFile()) return context->currentErgFile()->Points.at(i).y;
    else return 0;
}

size_t ErgFileData::size() const {
    if (context->currentErgFile()) return context->currentErgFile()->Points.count();
    else return 0;
}

QPointF ErgFileData::sample(size_t i) const
{
    return QPointF(x(i), y(i));
}

QRectF ErgFileData::boundingRect() const
{
    if (context->currentErgFile()) {
        double minX, minY, maxX, maxY;
        minX=minY=maxX=maxY=0.0f;
        foreach(ErgFilePoint x, context->currentErgFile()->Points) {
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
double NowData::x(size_t) const { return context->getNow(); }
double NowData::y(size_t i) const {
    if (i) {
        if (context->currentErgFile()) return context->currentErgFile()->maxY;
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

ErgFilePlot::ErgFilePlot(Context *context) : context(context)
{
    //insertLegend(new QwtLegend(), QwtPlot::BottomLegend);
    setCanvasBackground(GColor(CTRAINPLOTBACKGROUND));
    static_cast<QwtPlotCanvas*>(canvas())->setFrameStyle(QFrame::NoFrame);
    //courseData = data;                      // what we plot
    setAutoDelete(false);
    setAxesCount(QwtAxis::yRight, 4);

    // Setup the left axis (Power)
    setAxisTitle(yLeft, "Watts");
    enableAxis(yLeft, true);
    QwtScaleDraw *sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    setAxisMaxMinor(yLeft, 0);
    //setAxisScaleDraw(QwtPlot::yLeft, sd);

    QPalette pal;
    pal.setColor(QPalette::WindowText, GColor(CRIDEPLOTYAXIS));
    pal.setColor(QPalette::Text, GColor(CRIDEPLOTYAXIS));
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

    pal.setColor(QPalette::WindowText, GColor(CRIDEPLOTXAXIS));
    pal.setColor(QPalette::Text, GColor(CRIDEPLOTXAXIS));
    axisWidget(QwtPlot::xBottom)->setPalette(pal);

    // axis 1 not currently used
    setAxisVisible(QwtAxisId(QwtAxis::yRight,1), false); // max speed of 60mph/60kmh seems ok to me!
    enableAxis(QwtAxisId(QwtAxis::yRight,1).id, false);

    // set all the orher axes off but scaled
    setAxisScale(yLeft, 0, 300); // max cadence and hr
    enableAxis(yLeft, true);
    setAxisAutoScale(QwtPlot::yLeft, true);// we autoscale, since peaks are so much higher than troughs

    setAxisScale(yRight, 0, 250); // max cadence and hr
    enableAxis(yRight, false);
    setAxisScale(QwtAxisId(QwtAxis::yRight,2), 0, 60); // max speed of 60mph/60kmh seems ok to me!
    setAxisVisible(QwtAxisId(QwtAxis::yRight,2), false); // max speed of 60mph/60kmh seems ok to me!
    enableAxis(QwtAxisId(QwtAxis::yRight,2).id, false);

    // data bridge to ergfile
    lodData = new ErgFileData(context);
    // Load Curve
    LodCurve = new QwtPlotCurve("Course Load");
    LodCurve->setSamples(lodData);
    LodCurve->attach(this);
    LodCurve->setBaseline(-1000);
    LodCurve->setYAxis(QwtPlot::yLeft);

    // load curve is blue for time and grey for gradient
    QColor brush_color = QColor(GColor(CTPOWER));
    brush_color.setAlpha(64);
    LodCurve->setBrush(brush_color);   // fill below the line
    QPen Lodpen = QPen(GColor(CTPOWER), 1.0);
    LodCurve->setPen(Lodpen);

    wbalCurvePredict = new QwtPlotCurve("W'bal Predict");
    wbalCurvePredict->attach(this);
    wbalCurvePredict->setYAxis(QwtAxisId(QwtAxis::yRight, 3));
    QColor predict = GColor(CWBAL).darker();
    predict.setAlpha(200);
    QPen wbalPen = QPen(predict, 2.0); // predict darker...
    wbalCurvePredict->setPen(wbalPen);
    wbalCurvePredict->setVisible(true);

    wbalCurve = new QwtPlotCurve("W'bal Actual");
    wbalCurve->attach(this);
    wbalCurve->setYAxis(QwtAxisId(QwtAxis::yRight, 3));
    QPen wbalPenA = QPen(GColor(CWBAL), 1.0); // actual lighter
    wbalCurve->setPen(wbalPenA);
    wbalData = new CurveData;
    wbalCurve->setSamples(wbalData->x(), wbalData->y(), wbalData->count());

    sd = new QwtScaleDraw;
    sd->enableComponent(QwtScaleDraw::Ticks, false);
    sd->enableComponent(QwtScaleDraw::Backbone, false);
    sd->setLabelRotation(90);// in the 000s
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    setAxisScaleDraw(QwtAxisId(QwtAxis::yRight, 3), sd);
    pal.setColor(QPalette::WindowText, GColor(CWBAL));
    pal.setColor(QPalette::Text, GColor(CWBAL));
    axisWidget(QwtAxisId(QwtAxis::yRight, 3))->setPalette(pal);
    QwtPlot::setAxisFont(QwtAxisId(QwtAxis::yRight, 3), stGiles);
    QwtText title2("W'bal");
    title2.setFont(stGiles);
    QwtPlot::setAxisTitle(QwtAxisId(QwtAxis::yRight,3), title2);

    // telemetry history
    wattsCurve = new QwtPlotCurve("Power");
    QPen wattspen = QPen(GColor(CPOWER));
    wattsCurve->setPen(wattspen);
    wattsCurve->attach(this);
    wattsCurve->setYAxis(QwtPlot::yLeft);
    // dgr wattsCurve->setPaintAttribute(QwtPlotCurve::PaintFiltered);
    wattsData = new CurveData;
    wattsCurve->setSamples(wattsData->x(), wattsData->y(), wattsData->count());

    // telemetry history
    hrCurve = new QwtPlotCurve("Heartrate");
    QPen hrpen = QPen(GColor(CHEARTRATE));
    hrCurve->setPen(hrpen);
    hrCurve->attach(this);
    hrCurve->setYAxis(QwtPlot::yRight);
    hrData = new CurveData;
    hrCurve->setSamples(hrData->x(), hrData->y(), hrData->count());

    // telemetry history
    cadCurve = new QwtPlotCurve("Cadence");
    QPen cadpen = QPen(GColor(CCADENCE));
    cadCurve->setPen(cadpen);
    cadCurve->attach(this);
    cadCurve->setYAxis(QwtPlot::yRight);
    cadData = new CurveData;
    cadCurve->setSamples(cadData->x(), cadData->y(), cadData->count());

    // telemetry history
    speedCurve = new QwtPlotCurve("Speed");
    QPen speedpen = QPen(GColor(CSPEED));
    speedCurve->setPen(speedpen);
    speedCurve->attach(this);
    speedCurve->setYAxis(QwtAxisId(QwtAxis::yRight,2).id);
    speedData = new CurveData;
    speedCurve->setSamples(speedData->x(), speedData->y(), speedData->count());

    // Now data bridge
    nowData = new NowData(context);

    // CP marker
    QwtText CPText(QString(tr("CP")));
    CPText.setColor(GColor(CPLOTMARKER));
    CPMarker = new QwtPlotMarker(CPText);
    CPMarker->setLineStyle(QwtPlotMarker::HLine);
    CPMarker->setLinePen(GColor(CPLOTMARKER), 1, Qt::DotLine);
    CPMarker->setLabel(CPText);
    CPMarker->setLabelAlignment(Qt::AlignLeft | Qt::AlignTop);
    CPMarker->setYAxis(QwtPlot::yLeft);
    CPMarker->setYValue(274);
    CPMarker->attach(this);


    // Now pointer
    NowCurve = new QwtPlotCurve("Now");
    QPen Nowpen = QPen(Qt::red, 2.0);
    NowCurve->setPen(Nowpen);
    NowCurve->setSamples(nowData);
    NowCurve->attach(this);
    NowCurve->setYAxis(QwtPlot::yLeft);

    bydist = false;
    ergFile = NULL;

    setAutoReplot(false);
	setData(ergFile);

    configChanged(CONFIG_ZONES);

    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
}

void
ErgFilePlot::configChanged(qint32)
{
    setCanvasBackground(GColor(CTRAINPLOTBACKGROUND));
    CPMarker->setLinePen(GColor(CPLOTMARKER), 1, Qt::DotLine);

    // set CP Marker
    double CP = 0; // default
    if (context->athlete->zones("Bike")) {
        int zoneRange = context->athlete->zones("Bike")->whichRange(QDate::currentDate());
        if (zoneRange >= 0) CP = context->athlete->zones("Bike")->getCP(zoneRange);
    }
    if (CP) {
        CPMarker->setYValue(CP);
        CPMarker->show();
    } else CPMarker->hide();

    replot();
}


// Distribute segments into rows for non-overlapped display.
//
// This is currently based strictly on segment start and end and ignores
// text length. Ideally text length would be used to raise segment
// size for purposes of display packing, which would allow cause lap
// markers to be shown without their names stomping all over their
// adjacent bretheren.
class LapRowDistributor {

    const QList<ErgFileLap>& laps;
    std::unordered_map<int, std::tuple<int, int>> lapRangeIdMap;
    std::vector<int> segmentRowMap;

public:

    enum ResultEnum { Failed = 0, StartOfRange, EndOfRange, InternalRange, SimpleLap };

    ResultEnum GetInfo(int i, int& row) {

        if (i < 0 || i > laps.count())
            return Failed;

        int lapRangeId = laps.at(i).lapRangeId;

        row = segmentRowMap[std::get<0>(lapRangeIdMap[lapRangeId])];

        if (lapRangeId) {
            auto range = lapRangeIdMap.find(lapRangeId);
            if (range != lapRangeIdMap.end()) {
                if (std::get<0>(range->second) == i) return StartOfRange;
                if (std::get<1>(range->second) == i) return EndOfRange;
            }
            return InternalRange;
        }

        return SimpleLap;
    }

    LapRowDistributor(const QList<ErgFileLap> &laps) : laps(laps), segmentRowMap(laps.count(), -1) {

        // Part 1:
        //
        // Build mapping from lapRangeId to index of first/last lap markers in the
        // group.
        // 
        // Map provides instant access to start and end of rangeid.
        int lapCount = laps.count();

        for (int i = 0; i < lapCount; i++) {
            const ErgFileLap& lap = laps.at(i);

            int startIdx = i, endIdx = i;

            auto e = lapRangeIdMap.find(lap.lapRangeId);
            if (e != lapRangeIdMap.end()) {
                std::tie(startIdx, endIdx) = e->second;
                if (lap.x < laps.at(startIdx).x)
                    startIdx = i;

                if (lap.x > laps.at(endIdx).x)
                    endIdx = i;
            }

            lapRangeIdMap[lap.lapRangeId] = std::make_tuple(startIdx, endIdx);
        }

        // Part 2: Generate segmentRowMap, this is a map from lap to what row
        // that lap should be printed upon.

        // Tracks what segments are live at what row during search
        // Grows when a segment is found that can't fit into an existing
        // row.
        //
        // Note: This is a greedy packing, not optimal, but seems to look good
        // because adjacent segments tend to appear adjacent on the same row.
        std::vector<int> segmentRowLiveMap;
        for (int i = 0; i < lapCount; i++) {
            const ErgFileLap& lap = laps.at(i);

            // Space is only computed for first lap in range group.
            if (std::get<0>(lapRangeIdMap[lap.lapRangeId]) != i) {
                continue;
            }

            double startM = lap.x;

            // Age-out all rows of segments that end at or before startKM
            // Assign first available that is available
            int row = -1;
            for (int r = 0; r < segmentRowLiveMap.size(); r++) {
                int v = segmentRowLiveMap[r];
                if (v >= 0) {
                    double endM = laps.at(std::get<1>(lapRangeIdMap[laps.at(v).lapRangeId])).x;
                    if (endM <= startM) {
                        v = -1;
                        segmentRowLiveMap[r] = v;
                    }
                }

                // Take first free row we encounter.
                if (row < 0 && v < 0) {
                    segmentRowLiveMap[r] = i;
                    row = r;
                }
            }

            // If no free rows then push a new one on the end
            if (row < 0) {
                segmentRowLiveMap.push_back(i);
                row = (int)(segmentRowLiveMap.size() - 1);
            }

            // Record the row that this segment was assigned
            segmentRowMap[i] = row;
        }
    }
};

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

            QColor brush_color1 = QColor(Qt::gray);
            brush_color1.setAlpha(200);
            QColor brush_color2 = QColor(Qt::gray);
            brush_color2.setAlpha(64);

            QLinearGradient linearGradient(0, 0, 0, height());
            linearGradient.setColorAt(0.0, brush_color1);
            linearGradient.setColorAt(1.0, brush_color2);
            linearGradient.setSpread(QGradient::PadSpread);

            LodCurve->setBrush(linearGradient);   // fill below the line
            QPen Lodpen = QPen(Qt::gray, 1.0);
            LodCurve->setPen(Lodpen);

        } else {

            QColor brush_color1 = QColor(GColor(CTPOWER));
            brush_color1.setAlpha(200);
            QColor brush_color2 = QColor(GColor(CTPOWER));
            brush_color2.setAlpha(64);

            QLinearGradient linearGradient(0, 0, 0, height());
            linearGradient.setColorAt(0.0, brush_color1);
            linearGradient.setColorAt(1.0, brush_color2);
            linearGradient.setSpread(QGradient::PadSpread);

            LodCurve->setBrush(linearGradient);   // fill below the line
            QPen Lodpen = QPen(GColor(CTPOWER), 1.0);
            LodCurve->setPen(Lodpen);

        }

        LapRowDistributor lapRowDistributor(ergFile->Laps);

        // set up again
        for(int i=0; i < ergFile->Laps.count(); i++) {

            // Show Lap Number
            const ErgFileLap& lap = ergFile->Laps.at(i);

            int row = 0;
            LapRowDistributor::ResultEnum distributionResult = lapRowDistributor.GetInfo(i, row);

            // Danger: ASCII ART. Somebody please replace this with graphics?
            QString decoratedName;
            Qt::Alignment labelAlignment = Qt::AlignRight | Qt::AlignTop;

            switch(distributionResult) {
            case LapRowDistributor::StartOfRange:
                decoratedName = "<" + lap.name;
                break;
            case LapRowDistributor::EndOfRange:
                decoratedName = ">";
                labelAlignment = Qt::AlignLeft | Qt::AlignTop;
                break;
            case LapRowDistributor::SimpleLap:
                decoratedName = QString::number(lap.LapNum) + ":" + lap.name;
                break;
            case LapRowDistributor::InternalRange:
                decoratedName = "o";
                labelAlignment = Qt::AlignHCenter | Qt::AlignTop;
                break;
            case LapRowDistributor::Failed:
            default:
                // Nothing to do.
                break;
            };

            // Literal row translation. We loves ascii art...
            QString prefix = (row > 0) ? QString("\n").repeated(row) : "";
            QwtText text(prefix + decoratedName);
                
            text.setFont(QFont("Helvetica", 10, QFont::Bold));
            text.setColor(GColor(CPLOTMARKER));

            // vertical line
            QwtPlotMarker *add = new QwtPlotMarker();
            add->setLineStyle(QwtPlotMarker::VLine);
            add->setLinePen(QPen(GColor(CPLOTMARKER), 0, Qt::DashDotLine));
            add->setLabelAlignment(labelAlignment);
            add->setValue(lap.x, 0);
            add->setLabel(text);
            add->attach(this);

            Marks.append(add);
        }

        // set the axis so we use all the screen estate
        if (context->currentErgFile() && context->currentErgFile()->Points.count()) {
            double maxX = (double)context->currentErgFile()->Points.last().x;

            if (bydist) {
                double step = 5000;
                if (!GlobalContext::context()->useMetricUnits) maxX = (double)context->currentErgFile()->Points.last().x * MILES_PER_KM;
                // tics every 5 kilometers/miles, if workout shorter tics every 1000m
                if (maxX <= 1000) step = 100;
                else if (maxX < 5000) step = 1000;

                // axis setup for distance
                setAxisScale(xBottom, (double)0, maxX, step);
                QwtText title;
                title.setFont(stGiles);
                title.setText("Distance " + ((GlobalContext::context()->useMetricUnits) ? tr("(Km)") : tr(" Mi")));
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

                pal.setColor(QPalette::WindowText, GColor(CRIDEPLOTXAXIS));
                pal.setColor(QPalette::Text, GColor(CRIDEPLOTXAXIS));
                axisWidget(QwtPlot::xBottom)->setPalette(pal);

                // only allocate a new one if its not the current (they get freed by Qwt)
                if (axisScaleDraw(xBottom) != timedraw)
                    setAxisScaleDraw(QwtPlot::xBottom, (timedraw=new HourTimeScaleDraw()));
            }
        }

        // compute wbal curve for the erg file
        calculator.setErg(ergfile);

        setAxisTitle(QwtAxisId(QwtAxis::yRight, 3), tr("W' Balance (j)"));
        setAxisScale(QwtAxisId(QwtAxis::yRight, 3),qMin(double(calculator.minY-1000),double(0)),calculator.maxY+1000);
        setAxisLabelAlignment(QwtAxisId(QwtAxis::yRight, 3),Qt::AlignVCenter);

        // and the values ... but avoid sharing!
        wbalCurvePredict->setSamples(calculator.xdata(false), calculator.ydata());

    } else {

        // clear the plot we have nothing selected
        bydist = false; // do by time when no workout selected

        QwtText title;
        title.setFont(stGiles);
        title.setText("Time (mins)");
        QwtPlot::setAxisFont(xBottom, stGiles);
        QwtPlot::setAxisTitle(xBottom, title);

        pal.setColor(QPalette::WindowText, GColor(CRIDEPLOTXAXIS));
        pal.setColor(QPalette::Text, GColor(CRIDEPLOTXAXIS));
        axisWidget(QwtPlot::xBottom)->setPalette(pal);

        // set the axis so we default to an hour workout
        if (axisScaleDraw(xBottom) != timedraw)
            setAxisScaleDraw(QwtPlot::xBottom, (timedraw=new HourTimeScaleDraw()));
        setAxisScale(xBottom, (double)0, 1000 * 60 * 60, 15*60*1000);
    }

    // make the xBottom scale visible
    enableAxis(xBottom, true);
    setAxisVisible(xBottom, true);
}

void
ErgFilePlot::setNow(long /*msecs*/)
{
    replot(); // and update
}

void
ErgFilePlot::performancePlot(RealtimeData rtdata)
{
    // don't update this plot if we are not running or are paused
    if ((!context->isRunning) || (context->isPaused)) return;

    // we got some data
    double feetOrMeters = (GlobalContext::context()->useMetricUnits) ? rtdata.getDistance() * 1000 : rtdata.getDistance() * 1000 * FEET_PER_METER;
    // x is plotted in meters/feet or micro-seconds
    double x = bydist ? feetOrMeters : rtdata.getMsecs();

    // when not using a workout we need to extend the axis when we
    // go out of bounds -- we do not use autoscale for x, because we
    // want to control stepping and tick marking add another 30 mins
    if (!ergFile && axisScaleDiv(QwtPlot::xBottom).upperBound() <= x) {
        double maxX = x + ( 30 * 60 * 1000);
        setAxisScale(xBottom, (double)0, maxX, maxX > (15*60*1000) ? 15*60*1000 : 60*1000);
    }

    double watts = rtdata.getWatts();
    double speed = rtdata.getSpeed();
    double cad = rtdata.getCadence();
    double hr = rtdata.getHr();
    double wbal = rtdata.getWbal();

    wattssum += watts;
    hrsum += hr;
    cadsum += cad;
    speedsum += speed;
    wbalsum += wbal;

    if (counter < 25) {
        counter++;
        return;
    } else {
        watts = wattssum / 26;
        hr = hrsum / 26;
        cad = cadsum / 26;
        speed = speedsum / 26;
        wbal = wbalsum / 26;
        counter=0;
        wbalsum = wattssum = hrsum = cadsum = speedsum = 0;
    }

    double zero = 0;

    if (!wattsData->count()) wattsData->append(&zero, &watts, 1);
    wattsData->append(&x, &watts, 1);
    wattsCurve->setSamples(wattsData->x(), wattsData->y(), wattsData->count());

    if (!hrData->count()) hrData->append(&zero, &hr, 1);
    hrData->append(&x, &hr, 1);
    hrCurve->setSamples(hrData->x(), hrData->y(), hrData->count());

    if (!speedData->count()) speedData->append(&zero, &speed, 1);
    speedData->append(&x, &speed, 1);
    speedCurve->setSamples(speedData->x(), speedData->y(), speedData->count());

    if (!cadData->count()) cadData->append(&zero, &cad, 1);
    cadData->append(&x, &cad, 1);
    cadCurve->setSamples(cadData->x(), cadData->y(), cadData->count());

    if (!wbalData->count()) wbalData->append(&zero, &wbal, 1);
    wbalData->append(&x, &wbal, 1);
    wbalCurve->setSamples(wbalData->x(), wbalData->y(), wbalData->count());
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
    counter = wbalsum = hrsum = wattssum = speedsum = cadsum = 0;

    // note the origin of the data is not a point 0, but the first
    // average over 5 seconds. this leads to a small gap on the left
    // which is better than the traces all starting from 0,0 which whilst
    // is factually correct, does not tell us anything useful and look horrid.
    // instead when we place the first points on the plots we add them twice
    // once for time/distance of 0 and once for the current point in time
    wattsData->clear();
    wattsCurve->setSamples(wattsData->x(), wattsData->y(), wattsData->count());
    wbalData->clear();
    wbalCurve->setSamples(wbalData->x(), wbalData->y(), wbalData->count());
    cadData->clear();
    cadCurve->setSamples(cadData->x(), cadData->y(), cadData->count());
    hrData->clear();
    hrCurve->setSamples(hrData->x(), hrData->y(), hrData->count());
    speedData->clear();
    speedCurve->setSamples(speedData->x(), speedData->y(), speedData->count());
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

    for (int i = 0; i < count; i++) {
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
