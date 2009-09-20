#ifndef _RANDOMPLOT_H_
#define _RANDOMPLOT_H_ 1

#include "incrementalplot.h"

class QTimer;

class RandomPlot: public IncrementalPlot
{
    Q_OBJECT

public:
    RandomPlot(QWidget *parent);

    virtual QSize sizeHint() const;

signals:
    void running(bool);

public slots:
    void clear();
    void stop();
    void append(int timeout, int count);

private slots:
    void appendPoint();

private:
    void initCurve();

    QTimer *d_timer;
    int d_timerCount;
};

#endif // _RANDOMPLOT_H_
