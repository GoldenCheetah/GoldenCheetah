#include <qapplication.h>
#include <qlayout.h>
#include "tunerfrm.h"
#include "ampfrm.h"
#include "radio.h"

MainWin::MainWin(): 
    QWidget()
{
    TunerFrame *frmTuner = new TunerFrame(this);
    frmTuner->setFrameStyle(QFrame::Panel|QFrame::Raised);

    AmpFrame *frmAmp = new AmpFrame(this);
    frmAmp->setFrameStyle(QFrame::Panel|QFrame::Raised);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(frmTuner);
    layout->addWidget(frmAmp);
    
    connect(frmTuner, SIGNAL(fieldChanged(double)), 
        frmAmp, SLOT(setMaster(double)));

    frmTuner->setFreq(90.0);    
}

int main (int argc, char **argv)
{
    QApplication a(argc, argv);

    MainWin w;

#if QT_VERSION < 0x040000
    a.setMainWidget(&w);
#endif
    w.show();

    return a.exec();
}
