#include <qwt_plot.h>
#include <qwt_plot_spectrogram.h>

class Plot: public QwtPlot
{
    Q_OBJECT

public:
    Plot(QWidget * = NULL);

public slots:
    void showContour(bool on);
    void showSpectrogram(bool on);
    void printPlot();

private:
    QwtPlotSpectrogram *d_spectrogram;
};
