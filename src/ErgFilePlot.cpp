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

// static locals for now
static ErgFile *ergFile;
static QList<ErgFilePoint> *courseData;
static long Now;
static int MaxWatts;

// Load history
double ErgFileData::x(size_t i) const { return courseData ? courseData->at(i).x : 0; }
double ErgFileData::y(size_t i) const { return courseData ? courseData->at(i).y : 0; }
size_t ErgFileData::size() const { return courseData ? courseData->count() : 0; }
QwtData *ErgFileData::copy() const { return new ErgFileData(); }
//void ErgFileData::init() { }

// Now bar
double NowData::x(size_t) const { return Now; }
double NowData::y(size_t i) const { return (i ? MaxWatts : 0); }
size_t NowData::size() const { return 2; }
QwtData *NowData::copy() const { return new NowData(); }

ErgFilePlot::ErgFilePlot(QList<ErgFilePoint> *data)
{
    setInstanceName("ErgFile Plot");

    //insertLegend(new QwtLegend(), QwtPlot::BottomLegend);
    setCanvasBackground(Qt::white);
    canvas()->setFrameStyle(QFrame::NoFrame);
    courseData = data;                      // what we plot
    Now = 0;                                // where we are

    // Setup the axis (of evil :-)
    setAxisTitle(yLeft, "Watts");
    //setAxisScale(yLeft, 0, 500); // watts -- Autoscale please!
    enableAxis(yLeft, false);
    enableAxis(xBottom, false); // very little value and some cpu overhead
    enableAxis(yRight, false);

    // Load Curve
    LodCurve = new QwtPlotCurve("Course Load");
    QPen Lodpen = QPen(Qt::blue, 1.0);
    LodCurve->setPen(Lodpen);

    LodCurve->setData(lodData);
    LodCurve->attach(this);
    LodCurve->setYAxis(QwtPlot::yLeft);
    QColor brush_color = QColor(124, 91, 31);
    brush_color.setAlpha(64);
    LodCurve->setBrush(brush_color);   // fill below the line

    // Now pointer
    NowCurve = new QwtPlotCurve("Now");
    QPen Nowpen = QPen(Qt::red, 2.0);
    NowCurve->setPen(Nowpen);
    NowCurve->setData(nowData);
    NowCurve->attach(this);
    NowCurve->setYAxis(QwtPlot::yLeft);

}

void
ErgFilePlot::setData(ErgFile *ergfile)
{
    if (ergfile) {

        // set up again
        ergFile = ergfile;
        //setTitle(ergFile->Name);
        courseData = &ergfile->Points;
        MaxWatts = ergfile->MaxWatts;

        // clear the previous marks (if any)
        for(int i=0; i<Marks.count(); i++) {
            Marks.at(i)->detach();
            delete Marks.at(i);
        }
        Marks.clear();

        for(int i=0; i < ergFile->Laps.count(); i++) {

            // Show Lap Number
            QwtText text(QString::number(ergFile->Laps.at(i).LapNum));
            text.setFont(QFont("Helvetica", 10, QFont::Bold));
            text.setColor(Qt::black);

            // vertical line
            QwtPlotMarker *add = new QwtPlotMarker();
            add->setLineStyle(QwtPlotMarker::VLine);
            add->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
            add->setLinePen(QPen(Qt::black, 0, Qt::DashDotLine));
            add->setValue(ergFile->Laps.at(i).x, 0.0);
            add->setLabel(text);
            add->attach(this);

            Marks.append(add);
        }

        // set the axis so we use all the screen estate
        if ((*courseData).count()) setAxisScale(xBottom, (double)0, (double)(*courseData).last().x);
    }
}

void
ErgFilePlot::setNow(long msecs)
{
    Now = msecs;
    replot(); // and update
}

