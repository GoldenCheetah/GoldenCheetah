#include <qlayout.h>
#include <qlabel.h>
#include <qwt_wheel.h>
#include <qwt_slider.h>
#include <qwt_thermo.h>
#include <qwt_math.h>
#include "tunerfrm.h"

class TuningThermo: public QWidget
{
public:
    TuningThermo(QWidget *parent):
        QWidget(parent)
    {
        d_thermo = new QwtThermo(this);
        d_thermo->setOrientation(Qt::Horizontal, QwtThermo::NoScale);
        d_thermo->setRange(0.0, 1.0);
        d_thermo->setFillColor(Qt::green);

        QLabel *label = new QLabel("Tuning", this);
        label->setAlignment(Qt::AlignCenter);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setMargin(0);
        layout->addWidget(d_thermo);
        layout->addWidget(label);

        setFixedWidth(3 * label->sizeHint().width());
    }

    void setValue(double value)
    {
        d_thermo->setValue(value);
    }

private:
    QwtThermo *d_thermo;
};

TunerFrame::TunerFrame(QWidget *parent): 
    QFrame(parent)
{
    d_sldFreq = new QwtSlider(this, Qt::Horizontal, QwtSlider::TopScale);
    d_sldFreq->setRange(87.5, 108, 0.01, 10);
    d_sldFreq->setScaleMaxMinor(5);
    d_sldFreq->setScaleMaxMajor(12);
    d_sldFreq->setThumbLength(80);
    d_sldFreq->setBorderWidth(1);

    d_thmTune = new TuningThermo(this);

    d_whlFreq = new QwtWheel(this);
    d_whlFreq->setMass(0.5);
    d_whlFreq->setRange(87.5, 108, 0.01);
    d_whlFreq->setTotalAngle(3600.0);
    d_whlFreq->setFixedHeight(30);


    connect(d_whlFreq, SIGNAL(valueChanged(double)), SLOT(adjustFreq(double)));
    connect(d_sldFreq, SIGNAL(valueChanged(double)), SLOT(adjustFreq(double)));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setMargin(10);
    mainLayout->setSpacing(5);
    mainLayout->addWidget(d_sldFreq);

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->setMargin(0);
    hLayout->addWidget(d_thmTune, 0);
    hLayout->addStretch(5);
    hLayout->addWidget(d_whlFreq, 2);

    mainLayout->addLayout(hLayout);
}

void TunerFrame::adjustFreq(double frq)
{
    const double factor = 13.0 / (108 - 87.5);

    const double x = (frq - 87.5) * factor;
    const double field = qwtSqr(sin(x) * cos(4.0 * x));
    
    d_thmTune->setValue(field);  

    if (d_sldFreq->value() != frq) 
        d_sldFreq->setValue(frq);
    if (d_whlFreq->value() != frq) 
        d_whlFreq->setValue(frq);

    emit fieldChanged(field);   
}

void TunerFrame::setFreq(double frq)
{
    d_whlFreq->setValue(frq);
}
