#ifndef _GC_GcChartWindow_h
#define _GC_GcChartWindow_h

// #include "GoldenCheetah.h"
#include "GcWindow.h"
#include "GcOverlayWidget.h"
#include <QStackedLayout>
#include <QPropertyAnimation>
#include <QTimer>
#include <QTextStream>

#ifdef GC_HAS_CLOUD_DB
#include "CloudDBChart.h"
#include "CloudDBUserMetric.h"
#endif

class Context;

class GcChartWindow : public GcWindow
{
private:

    Q_OBJECT

    QStackedLayout *_layout;
    QStackedLayout *_mainLayout;
    QVBoxLayout *_defaultBlankLayout;

    QLayout *_chartLayout,
            *_revealLayout,
            *_blankLayout;

    QWidget *_mainWidget;
    QWidget *_blank;
    QWidget *_chart;

    // reveal controls
    QWidget *_revealControls;
    QPropertyAnimation *_revealAnim,
                       *_unrevealAnim;
    QTimer *_unrevealTimer;
    Context *context;

public:

    // reveal
    bool virtual hasReveal() { return false; }
    void reveal();
    void unreveal();

    // overlay widget
    GcOverlayWidget *overlayWidget;
    bool wantOverlay;

    // handle Chart Serialissation
    void serializeChartToQTextStream(QTextStream& out);


    GcChartWindow(Context *context);

    // parse a .gchart file / or string and return a list of charts expressed
    // as a property list in a QMap1
    static QList<QMap<QString,QString> > chartPropertiesFromFile(QString filename);
    static QList<QMap<QString,QString> > chartPropertiesFromString(QString contents);

    QWidget *mainWidget() { return _mainWidget; }
    GcOverlayWidget *helperWidget() { return overlayWidget; }

    void setChartLayout(QLayout *layout);
    void setRevealLayout(QLayout *layout);
    void setBlankLayout(QLayout *layout);
    void setIsBlank(bool value);
    void setControls(QWidget *x);
    void addHelper(QString name, QWidget *widget); // add to the overlay widget

public Q_SLOTS:
    void hideRevealControls();
    void saveImage();
    void saveChart();
#ifdef GC_HAS_CLOUD_DB
    void exportChartToCloudDB();
    bool chartHasUserMetrics();
#endif
    void colorChanged(QColor);
};

#endif
