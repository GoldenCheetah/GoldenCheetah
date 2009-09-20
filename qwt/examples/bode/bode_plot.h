#include <qwt_plot.h>

class QwtPlotCurve;
class QwtPlotMarker;

class BodePlot: public QwtPlot
{
    Q_OBJECT
public:
    BodePlot(QWidget *parent);

public slots:
    void setDamp(double damping);

private:
    void showData(double *frequency, double *amplitude, 
        double *phase, int count);
    void showPeak(double freq, double amplitude);
    void show3dB(double freq);

    QwtPlotCurve *d_crv1;
    QwtPlotCurve *d_crv2;
    QwtPlotMarker *d_mrk1;
    QwtPlotMarker *d_mrk2;
};
