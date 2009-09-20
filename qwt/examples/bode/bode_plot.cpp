#include <qwt_math.h>
#include <qwt_scale_engine.h>
#include <qwt_symbol.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_curve.h>
#include <qwt_legend.h>
#include <qwt_text.h>
#include "cplx.h"
#include "bode_plot.h"

static void logSpace(double *array, int size, double xmin, double xmax)
{ 
    if ((xmin <= 0.0) || (xmax <= 0.0) || (size <= 0))
       return;

    const int imax = size -1;

    array[0] = xmin;
    array[imax] = xmax;

    const double lxmin = log(xmin);
    const double lxmax = log(xmax);
    const double lstep = (lxmax - lxmin) / double(imax);

    for (int i = 1; i < imax; i++)
       array[i] = exp(lxmin + double(i) * lstep);
}

BodePlot::BodePlot(QWidget *parent):
    QwtPlot(parent)
{
    setAutoReplot(false);

    setTitle("Frequency Response of a Second-Order System");

    setCanvasBackground(QColor(Qt::darkBlue));

    // legend
    QwtLegend *legend = new QwtLegend;
    legend->setFrameStyle(QFrame::Box|QFrame::Sunken);
    insertLegend(legend, QwtPlot::BottomLegend);

    // grid 
    QwtPlotGrid *grid = new QwtPlotGrid;
    grid->enableXMin(true);
    grid->setMajPen(QPen(Qt::white, 0, Qt::DotLine));
    grid->setMinPen(QPen(Qt::gray, 0 , Qt::DotLine));
    grid->attach(this);

    // axes
    enableAxis(QwtPlot::yRight);
    setAxisTitle(QwtPlot::xBottom, "Normalized Frequency");
    setAxisTitle(QwtPlot::yLeft, "Amplitude [dB]");
    setAxisTitle(QwtPlot::yRight, "Phase [deg]");

    setAxisMaxMajor(QwtPlot::xBottom, 6);
    setAxisMaxMinor(QwtPlot::xBottom, 10);
    setAxisScaleEngine(QwtPlot::xBottom, new QwtLog10ScaleEngine);
  
    // curves
    d_crv1 = new QwtPlotCurve("Amplitude");
#if QT_VERSION >= 0x040000
    d_crv1->setRenderHint(QwtPlotItem::RenderAntialiased);
#endif
    d_crv1->setPen(QPen(Qt::yellow));
    d_crv1->setYAxis(QwtPlot::yLeft);
    d_crv1->attach(this);

    d_crv2 = new QwtPlotCurve("Phase");
#if QT_VERSION >= 0x040000
    d_crv2->setRenderHint(QwtPlotItem::RenderAntialiased);
#endif
    d_crv2->setPen(QPen(Qt::cyan));
    d_crv2->setYAxis(QwtPlot::yRight);
    d_crv2->attach(this);
    
    // marker
    d_mrk1 = new QwtPlotMarker();
    d_mrk1->setValue(0.0, 0.0);
    d_mrk1->setLineStyle(QwtPlotMarker::VLine);
    d_mrk1->setLabelAlignment(Qt::AlignRight | Qt::AlignBottom);
    d_mrk1->setLinePen(QPen(Qt::green, 0, Qt::DashDotLine));
    d_mrk1->attach(this);

    d_mrk2 = new QwtPlotMarker();
    d_mrk2->setLineStyle(QwtPlotMarker::HLine);
    d_mrk2->setLabelAlignment(Qt::AlignRight | Qt::AlignBottom);
    d_mrk2->setLinePen(QPen(QColor(200,150,0), 0, Qt::DashDotLine));
    d_mrk2->setSymbol( QwtSymbol(QwtSymbol::Diamond, 
        QColor(Qt::yellow), QColor(Qt::green), QSize(7,7)));
    d_mrk2->attach(this);

    setDamp(0.0);

    setAutoReplot(true);
}

void BodePlot::showData(double *frequency, double *amplitude,
    double *phase, int count)
{
    d_crv1->setData(frequency, amplitude, count);
    d_crv2->setData(frequency, phase, count);
}

void BodePlot::showPeak(double freq, double amplitude)
{
    QString label;
    label.sprintf("Peak: %.3g dB", amplitude);

    QwtText text(label);
    text.setFont(QFont("Helvetica", 10, QFont::Bold));
    text.setColor(QColor(200,150,0));

    d_mrk2->setValue(freq, amplitude);
    d_mrk2->setLabel(text);
}

void BodePlot::show3dB(double freq)
{
    QString label;
    label.sprintf("-3 dB at f = %.3g", freq);

    QwtText text(label);
    text.setFont(QFont("Helvetica", 10, QFont::Bold));
    text.setColor(Qt::green);

    d_mrk1->setValue(freq, 0.0);
    d_mrk1->setLabel(text);
}

//
// re-calculate frequency response
//
void BodePlot::setDamp(double damping)
{
    const bool doReplot = autoReplot();
    setAutoReplot(false);

    const int ArraySize = 200;

    double frequency[ArraySize];
    double amplitude[ArraySize];
    double phase[ArraySize];

    // build frequency vector with logarithmic division
    logSpace(frequency, ArraySize, 0.01, 100);

    int i3 = 1;
    double fmax = 1;
    double amax = -1000.0;
    
    for (int i = 0; i < ArraySize; i++)
    {
        double f = frequency[i];
        cplx g = cplx(1.0) / cplx(1.0 - f * f, 2.0 * damping * f);
        amplitude[i] = 20.0 * log10(sqrt( g.real()*g.real() + g.imag()*g.imag()));
        phase[i] = atan2(g.imag(), g.real()) * (180.0 / M_PI);

        if ((i3 <= 1) && (amplitude[i] < -3.0)) 
           i3 = i;
        if (amplitude[i] > amax)
        {
            amax = amplitude[i];
            fmax = frequency[i];
        }
        
    }
    
    double f3 = frequency[i3] - 
       (frequency[i3] - frequency[i3 - 1]) 
          / (amplitude[i3] - amplitude[i3 -1]) * (amplitude[i3] + 3);
    
    showPeak(fmax, amax);
    show3dB(f3);
    showData(frequency, amplitude, phase, ArraySize);

    setAutoReplot(doReplot);

    replot();
}
