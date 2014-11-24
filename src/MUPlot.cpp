/*
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
 * Copyright (c) 2014 Andy Froncioni
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

#include "MUPlot.h"
#include "MUWidget.h"
#include "Context.h"
#include "Athlete.h"
#include "Zones.h"
#include "Colors.h"

#include <unistd.h>
#include <QDebug>
#include <qwt_series_data.h>
#include <qwt_legend.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_marker.h>
#include <qwt_symbol.h>
#include <qwt_scale_engine.h>
#include <qwt_scale_widget.h>
#include <qwt_color_map.h>
#include <algorithm> // for std::lower_bound

#include "CriticalPowerWindow.h"
#include "CPPlot.h"
#include "GcOverlayWidget.h"
#include "Settings.h"
#include "TimeUtils.h"

double PI = 3.14159265359f;

MUPlot::MUPlot(MUWidget *muw, CriticalPowerWindow *parent, Context *context) 
       : QwtPlot(parent), QwtSyntheticPointData(MUN), 
         context(context), muw(muw), parent(parent), modelCurve(NULL), slowCurve(NULL), fastCurve(NULL), mmpCurve(NULL)
{

    // initalise all the model stufff
    // probably abstract this out later
    muSet = NULL;
    slowHandle = NULL;
    slowLine = NULL;
    slowDrag = false;
    fastHandle = NULL;
    fastLine = NULL;
    fastDrag = false;

    setAutoDelete(false);
    setAutoFillBackground(true);
    static_cast<QwtPlotCanvas*>(canvas())->setFrameStyle(QFrame::NoFrame);

    // left yAxis scale prettify
    QwtScaleDraw *sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    sd->enableComponent(QwtScaleDraw::Ticks, false);
    sd->enableComponent(QwtScaleDraw::Backbone, false);
    setAxisScaleDraw(yLeft, sd);
    setAxisTitle(yLeft, tr("w(x)"));
    setAxisMaxMinor(yLeft, 0);
    plotLayout()->setAlignCanvasToScales(true);

    // 0.5 increments on bottom axis
    QwtValueList ytick[QwtScaleDiv::NTickTypes];
    for (double i=0.0f; i<=10.0f; i+= 1.0)
        ytick[QwtScaleDiv::MajorTick]<<i;
    setAxisScaleDiv(yLeft,QwtScaleDiv(0.0f,10.0f,ytick));

    // bottom xAxis scale prettify
    sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    sd->enableComponent(QwtScaleDraw::Ticks, false);
    sd->enableComponent(QwtScaleDraw::Backbone, false);
    setAxisScaleDraw(xBottom, sd);
    setAxisTitle(xBottom, tr("Motor Unit, x"));
    setAxisMaxMinor(xBottom, 0);

    // 0.2 increments on bottom axis
    QwtValueList xtick[QwtScaleDiv::NTickTypes];
    for (double i=0.0f; i<=1.0f; i+= 0.2)
        xtick[QwtScaleDiv::MajorTick]<<i;
    setAxisScaleDiv(xBottom,QwtScaleDiv(0.0f,1.0f,xtick));

    // now color everything we created
    configChanged();

    // set to a 2 Normal Model
    setModel(2);

    // set mouse tracker
    parent->setMouseTracking(true);
    installEventFilter(parent);
    new muMouseTracker(this);
}

// set colours mostly
void
MUPlot::configChanged()
{
    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    setPalette(palette);

    axisWidget(QwtPlot::xBottom)->setPalette(palette);
    axisWidget(QwtPlot::yLeft)->setPalette(palette);

    setCanvasBackground(GColor(CPLOTBACKGROUND));
}

// get the fonts and colors right for the axis scales
void
MUPlot::setAxisTitle(int axis, QString label)
{
    // setup the default fonts
    QFont stGiles; // hoho - Chart Font St. Giles ... ok you have to be British to get this joke
    stGiles.fromString(appsettings->value(this, GC_FONT_CHARTLABELS, QFont().toString()).toString());
    stGiles.setPointSize(appsettings->value(NULL, GC_FONT_CHARTLABELS_SIZE, 8).toInt());

    QwtText title(label);
    title.setColor(GColor(CPLOTMARKER));
    title.setFont(stGiles);
    QwtPlot::setAxisFont(axis, stGiles);
    QwtPlot::setAxisTitle(axis, title);
}


double 
MUPlot::y(double x) const
{
    switch(model) {
        case 0 : return 0;
        case 1 : return slowNormal->y(x);
        default:
        case 2 : return slowNormal->y(x) +  fastNormal->y(x); // additive
    }
}

double 
MUPlot::totalY()
{
    double tot = 0;

    // sum all points -- only really needed for 2 normal model
    for (int x=0; x<MUN; x++) {
        tot += y(double(x)/MUN);
    }
    return tot;
}

// We are going to support multiple models; normal, normal-skew, linear
// So for now lets make it a paramter for the model curve creation
// The model may end up being comprised of 'sub models' for each fibre type
void
MUPlot::setModel(int model)
{
    this->model = model;

    // first lets clear any curves we shouldn't be displaying
    // no model curve if not power !
    if (modelCurve) {
        modelCurve->detach();
        delete modelCurve;
        modelCurve = NULL;
    }

    if (slowCurve) {
        slowCurve->detach();
        delete slowCurve;
        slowCurve = NULL;
    }

    if (slowHandle) {
        slowHandle->detach();
        delete slowHandle;
        slowHandle = NULL;
    }

    if (slowLine) {
        slowLine->detach();
        delete slowLine;
        slowLine = NULL;
    }

    if (fastCurve) {
        fastCurve->detach();
        delete fastCurve;
        fastCurve = NULL;
    }

    if (fastHandle) {
        fastHandle->detach();
        delete fastHandle;
        fastHandle = NULL;
    }

    if (fastLine) {
        fastLine->detach();
        delete fastLine;
        fastLine = NULL;
    }

    if (mmpCurve) {
        mmpCurve->detach();
        delete mmpCurve;
        mmpCurve = NULL;
    }

    switch (model) {

    case 0 : // no model - do nothing
        {
        }
        break;

    case 2 : // 2 Normal distribution -- drops through to case 1 below
        {
            fastCurve = new QwtPlotCurve( "MU Distribution" );
            fastCurve->setData(fastNormal = new MUNormal(MU_FASTMEAN, 0.05f));
            fastCurve->attach(this);

            QColor handleColor = QColor(Qt::magenta); // customise (?)
            handleColor.darker(30);
            handleColor.setAlpha(64);

            // now a mean line
            fastLine = new QwtPlotMarker();
            fastLine->setLineStyle(QwtPlotMarker::VLine);
            fastLine->setLinePen(QPen(QColor(handleColor), 0, Qt::SolidLine));
            fastLine->setValue(MU_FASTMEAN, 0); // mean
            fastLine->setZ(50);
            fastLine->attach(this);

            // now add a handle
            QColor color = GColor(CPLOTBACKGROUND);
            double x = MU_FASTMEAN; // mean is ok
            double y = 0.05f * 40.0f; // variance scaled to w(x)

            QwtSymbol *sym = new QwtSymbol;
            sym->setStyle(QwtSymbol::Rect);
            sym->setSize(8);
            sym->setPen(QPen(handleColor)); // customise ?
            sym->setBrush(QBrush(color));

            fastHandle = new QwtPlotMarker();
            fastHandle->setValue(x, y);
            fastHandle->setYAxis(yLeft);
            fastHandle->setZ(100);
            fastHandle->setSymbol(sym);
            fastHandle->attach(this);

        }

    case 1 : // Normal distribution
        {
            slowCurve = new QwtPlotCurve( "MU Distribution" );
            slowCurve->setData(slowNormal = new MUNormal(MU_SLOWMEAN, 0.05f));
            slowCurve->attach(this);

            QColor handleColor = GColor(CCP);
            handleColor.darker(30);
            handleColor.setAlpha(64);

            // now a mean line
            slowLine = new QwtPlotMarker();
            slowLine->setLineStyle(QwtPlotMarker::VLine);
            slowLine->setLinePen(QPen(QColor(handleColor), 0, Qt::SolidLine));
            slowLine->setValue(MU_SLOWMEAN, 0); // mean
            slowLine->setZ(50);
            slowLine->attach(this);

            // now add a handle
            QColor color = GColor(CPLOTBACKGROUND);
            double x = MU_SLOWMEAN; // mean is ok
            double y = 0.05f * 40.0f; // variance scaled to w(x)

            QwtSymbol *sym = new QwtSymbol;
            sym->setStyle(QwtSymbol::Rect);
            sym->setSize(8);
            sym->setPen(QPen(handleColor)); // customise ?
            sym->setBrush(QBrush(color));

            slowHandle = new QwtPlotMarker();
            slowHandle->setValue(x, y);
            slowHandle->setYAxis(yLeft);
            slowHandle->setZ(100);
            slowHandle->setSymbol(sym);
            slowHandle->attach(this);

            // mmp curve
            mmpCurve = new QwtPlotCurve( "MU Distribution" );
            mmpCurve->attach(parent->cpPlot);
        }
        break;
    }

    // set the colors etc for our curve(s)
    if (fastCurve) {

        // antialias
        bool antialias = appsettings->value(this, GC_ANTIALIAS, false).toBool();
        if (antialias) fastCurve->setRenderHint(QwtPlotItem::RenderAntialiased);

        // color and brush
        QColor color = QColor(Qt::magenta); //XXX customise
        QPen pen(color);
        pen.setWidth(1.0);

        color.setAlpha(80);
        QBrush brush(color);

        fastCurve->setPen(pen);
        fastCurve->setBrush(brush);

        QwtSymbol *sym = new QwtSymbol;
        sym->setStyle(QwtSymbol::Ellipse);
        sym->setSize(4);
        sym->setPen(QPen(Qt::yellow)); // customise ?
        sym->setBrush(QBrush(Qt::yellow));
    }

    if (slowCurve) {

        // antialias
        bool antialias = appsettings->value(this, GC_ANTIALIAS, false).toBool();
        if (antialias) slowCurve->setRenderHint(QwtPlotItem::RenderAntialiased);

        // color and brush
        QColor color = GColor(CCP); // customise ?
        QPen pen(color);
        pen.setWidth(1.0);

        color.setAlpha(80);
        QBrush brush(color);

        slowCurve->setPen(pen);
        slowCurve->setBrush(brush);

        QwtSymbol *sym = new QwtSymbol;
        sym->setStyle(QwtSymbol::Ellipse);
        sym->setSize(4);
        sym->setPen(QPen(Qt::yellow)); // customise ?
        sym->setBrush(QBrush(Qt::yellow));

        mmpCurve->setPen(QPen(Qt::yellow));
        mmpCurve->setSymbol(sym);
        mmpCurve->setBrush(Qt::NoBrush);

        // and refresh the MMP
        setMUSet();

    }

    if (model) {
        modelCurve = new QwtPlotCurve( "Total Distribution" );
        modelCurve->setData(this); // we provide the data too!

        QColor color = QColor(Qt::yellow); //XXX customise
        QPen pen(color);
        pen.setWidth(1.0);
        color.setAlpha(80);
        modelCurve->setPen(pen);
        modelCurve->setBrush(Qt::NoBrush);

        modelCurve->attach(this);
    }


    replot();
}

bool
MUPlot::eventFilter(QObject *, QEvent *e)
{
    switch (e->type()) {

    case QEvent::MouseButtonPress:
        {
            if (!slowDrag && !fastDrag) {

                // where is the mouse ?
                QPoint pos = QCursor::pos();

                // slow handle ?
                QPoint mpos = canvas()->mapToGlobal(QPoint(transform(xBottom, slowHandle->value().x()), 
                                                    transform(yLeft, slowHandle->value().y())));

                int dx = mpos.x() - pos.x();
                int dy = mpos.y() - pos.y();

                if ((dx > -6 && dx < 6) && (dy > -6 && dy < 6)) {

                    // close enough
                    slowDrag = true;
                    return false;
                }

                // fast handle ?
                mpos = canvas()->mapToGlobal(QPoint(transform(xBottom, fastHandle->value().x()), 
                                                    transform(yLeft, fastHandle->value().y())));

                dx = mpos.x() - pos.x();
                dy = mpos.y() - pos.y();

                if ((dx > -6 && dx < 6) && (dy > -6 && dy < 6)) {

                    // close enough
                    fastDrag = true;
                    return false;
                }
            }
        }
        break;

    case QEvent::MouseButtonRelease:
        {
            if (fastDrag) {
                fastDrag = false;
            }
            if (slowDrag) {
                slowDrag = false;
            }
        }
        break;

    default:
        break;
    }
    return false;
}

void
MUPlot::mouseMoved()
{
    if ((!slowDrag && !fastDrag)) return;

    // where is the mouse ?
    QPoint pos = QCursor::pos();

    // cursor position
    QPoint cpos = canvas()->mapFromGlobal(pos);

    double mean = invTransform(xBottom,cpos.x());
    double variance = invTransform(yLeft, cpos.y()) / 40.0f;

    // dragging slow
    if (fastDrag) {

        fastNormal->setMean(mean);
        fastNormal->setVariance(variance);

        // move the handle
        fastHandle->setValue(mean, variance * 40.0f);

        // move the line
        fastLine->setValue(mean, 0);
    }

    // dragging slow
    if (slowDrag) {

        slowNormal->setMean(mean);
        slowNormal->setVariance(variance);

        // move the handle
        slowHandle->setValue(mean, variance * 40.0f);

        // move the line
        slowLine->setValue(mean, 0);
    }

    // redo MMP
    setMUSet();

    replot();
}

void
MUPlot::setMUSet()
{
    // set the MU distribution, starting conditions
    // prior to determining the MMP etc from those
    // starting conditions.

    // this is a TEMPLATE and is used in assignment and never
    // changed -- this is to speed up creation of worker threads
    // all using the same starting conditions

    if (muSet == NULL) {
        // allocate the array
        muSet = new QVector<MUPool>(MUN); // MUN is usually set to 1000
    }

    // we do this every time as the parameters are editable
    // by the user, so they may change from the UI

    double totaly = totalY(); // add up distribution
    for (int x=0; x<MUN; x++) {

        (*muSet)[x].twitch = double(x) / double(MUN);
        (*muSet)[x].pmax = muw->pmax0->value() + ((muw->pmax1->value() - muw->pmax0->value()) * (double(x) / double(MUN)));
        (*muSet)[x].pmin = muw->pmin0->value() + ((muw->pmin1->value() - muw->pmin0->value()) * (double(x) / double(MUN)));
        (*muSet)[x].tau = muw->tau0->value() + ((muw->tau1->value() - muw->tau0->value()) * (double(x) / double(MUN)));
        (*muSet)[x].alpha = muw->alpha->value();

        // get mass from the model
        (*muSet)[x].mass = y(double(x)/double(MUN)) / totaly * muw->mass->value();

    }

    // now refresh MMP Curve using the new MU set
    QVector<double> xseries;
    QVector<double> yseries;

    // for now we use the same worker in serial
    // we might reiplement this as an OpenCL kernel or similar
    // to perform in parallel for performance
    MUSet worker(muSet); 

    xseries << 1.0/60.00f;
    yseries << worker.pMax;

    int inc = (worker.pMax-worker.pMin) / MUSAMPLES;

    for (int i=worker.pMax - inc; i > worker.pMin; i -= inc) {

        double duration = worker.applyLoad(i);
        if (duration) {
            xseries << double(duration) / 60.0f;
            yseries << double(i);
        } 
    }

    xseries << double(3 * 3600)/60.00f;
    yseries << worker.pMin;

    mmpCurve->setSamples(xseries, yseries);

    parent->cpPlot->replot();
}

//
// Working with a set of motor units
//
MUSet::MUSet(QVector<MUPool>* set) : muSet(*set)
{
    setMinMax();
}

// reset the pool
void 
MUSet::reset(QVector<MUPool>* set)
{
    muSet = *set;
    setMinMax();
}

void 
MUSet::setMinMax()
{
    pMax = 0; // whats the max power this distribution can muster?
    pMin = 0; // whats the max power this distribution can muster?
    for (int x=0; x<muSet.size(); x++) {
       pMax += muSet[x].mass * muSet[x].pmax;
       pMin += muSet[x].mass * muSet[x].pmin;
    }
}

// apply load and return maximum duration
// this load can be applied
int 
MUSet::applyLoad(int watts)
{
    // lets not waste our time with out of bound questions
    if (watts > pMax || watts < pMin) return 0;

    // initialise starting conditions
    QVector<double> pMu(MUN);
    QVector<double> act(MUN);
    for(int x=0; x<MUN; x++) {
        pMu[x] = muSet[x].pmax;
        act[x] = -1;
    }

    int duration = 0;
    while (duration < (3 * 3600)) { // anything more than 3 hours is too long dude

        // lets get a second of that power
        double ptot = 0; // how much power did we generate ?
        int x = 0; // how far across the pool did we get ?

        // lets recruite some fibers!
        while (ptot < watts && x < MUN) {

            // is this bin fired yet ?
            if (act[x] < 0) { // first time!
                act[x] = duration; // we just fired it!
            }

            // how long since we fired this sucker ?
            int wasfired = duration - act[x];

            // if we've passed tau seconds we need to apply some fatigue
            if (wasfired > muSet[x].tau) {

                // apply fatigue to pmax
                pMu[x] = (pMu[x] - muSet[x].pmin) * exp(-1.0 * muSet[x].alpha * (double(x)/MUN) * (wasfired - muSet[x].tau)) + muSet[x].pmin;
            }

            // take the power
            ptot += muSet[x].mass * pMu[x];

            // next bin
            x ++;

        }

        // we exhausted fibers
        if (x == MUN) return duration; // we're cooked

        // well we managed to hang in there for another second !
        duration++;
    }

    // too long .. return 3 hours
    return duration;
}
