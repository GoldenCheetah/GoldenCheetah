#include <qwt_plot.h>

class ColorBar;
class QwtWheel;

class Plot: public QwtPlot
{
    Q_OBJECT
public:
    Plot(QWidget *parent = NULL);
    virtual bool eventFilter(QObject *, QEvent *);

public slots:
    void setCanvasColor(const QColor &);
    void insertCurve(int axis, double base);

private slots:
    void scrollLeftAxis(double);

private:
    void insertCurve(Qt::Orientation, const QColor &, double base);

    ColorBar *d_colorBar;
    QwtWheel *d_wheel;
};
