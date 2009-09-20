#include <qframe.h>

class QwtWheel;
class QwtSlider;
class TuningThermo;

class TunerFrame : public QFrame
{
    Q_OBJECT
public:
    TunerFrame(QWidget *p);

signals:
    void fieldChanged(double f);

public slots:
    void setFreq(double frq);

private slots:
    void adjustFreq(double frq);

private:
    QwtWheel *d_whlFreq;
    TuningThermo *d_thmTune;
    QwtSlider *d_sldFreq;
};




