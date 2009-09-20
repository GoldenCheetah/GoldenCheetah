#include <qwt_plot.h>
#include <qwt_double_rect.h>

class QwtPlotSvgItem;

class Plot: public QwtPlot
{
    Q_OBJECT

public:
    Plot(QWidget * = NULL);

public slots:
    void loadSVG();

private:
    void rescale();

    QwtPlotSvgItem *d_mapItem;
    const QwtDoubleRect d_mapRect;
};
