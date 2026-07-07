#ifndef _GC_ElevationChartWindow_h
#define _GC_ElevationChartWindow_h 1

#include <QObject>
#include "GoldenCheetah.h"


#include "ErgFile.h"
#include "RealtimeData.h"

namespace elevationChart {

    //   Two classes in this namespace, each one to show a figure in the chart window:
    //   - BubbleWidget: to show the elevation gradient in a bubble, whose color and position reflects the gradient.
    //   - SlopeWidget: to show current position and slope, and the profile for the short term
    //  They both overlap the other, so they are shown in the same space

    class BubbleWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit BubbleWidget(double deltaSeconds_, QWidget *parent = nullptr) :
            QWidget(parent), deltaSeconds(deltaSeconds_), m_rtData(nullptr), m_ergFileAdapter(nullptr)
        {
            // Set a size policy to allow resizing
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        }

        void setRealtimeData(RealtimeData *rtData) { m_rtData = rtData; }

        void setErgFileAdapter(ErgFileQueryAdapter *ergFileAdapter) { m_ergFileAdapter = ergFileAdapter; }

    private:
        double deltaSeconds;
        RealtimeData *m_rtData;
        ErgFileQueryAdapter *m_ergFileAdapter;

    protected:
        void paintEvent(QPaintEvent *event) override;
    };


    class SlopeWidget : public QWidget
    {
        Q_OBJECT
    public:
        //explicit SlopeWidget(QWidget *parent = nullptr) : QWidget(parent), m_rtData(nullptr), m_ergFileAdapter(nullptr)
        explicit SlopeWidget(QWidget *parent = nullptr) : QWidget(parent)
        {
            // Set a size policy to allow resizing
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        }

        QQueue<QPointF>& getPlotQ() { return plotQ; }
        void setSlope(double slope) { m_slope = slope; }
        void setCurrPos(double currPos) { m_currPos = currPos; }
        void setyPlotScale(int yPlotScale) { m_yPlotScale = yPlotScale; }

        protected:
        void paintEvent(QPaintEvent *event) override;

    private:
        QQueue<QPointF> plotQ;
        double m_slope;
        double m_currPos;   //  Current position
        int m_yPlotScale;   //  Elevation window size
    };

} // namespace elevationChart

class Context;

class ElevationChartWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    // properties can be saved/restored/set by the layout manager
    Q_PROPERTY(int deltaSlopeSeconds READ deltaSlopeSeconds WRITE setDeltaSlopeSeconds USER true)
    Q_PROPERTY(int plotDistance READ plotDistance WRITE setPlotDistance USER true)
    Q_PROPERTY(int yPlotScale READ yPlotScale WRITE setyPlotScale USER true)

    public:

        ElevationChartWindow(Context *context);

        // set/get properties
        int deltaSlopeSeconds() const { return customDeltaSlopeSeconds->value(); }
        void setDeltaSlopeSeconds(int x) { customDeltaSlopeSeconds->setValue(x); }
        int plotDistance() const { return customPlotDistance->value(); }
        void setPlotDistance(int x) { customPlotDistance->setValue(x); }
        int yPlotScale() const { return customyPlotScale->value(); }
        void setyPlotScale(int x) { customyPlotScale->setValue(x); }


    public slots:
        void ergFileSelected(ErgFile*);
        void telemetryUpdate(const RealtimeData &rtData); // got new data

    protected:
        void paintEvent(QPaintEvent *event) override;

    private:
        // Settings data
        QLabel* customPlotDistanceLabel;
        QLabel* customDeltaSlopeSecondsLabel;
        QLabel* customyPlotScaleLabel;
        QSpinBox* customPlotDistance;
        QSpinBox* customDeltaSlopeSeconds;
        QSpinBox* customyPlotScale;

        // Chart widgets
        elevationChart::BubbleWidget *bubbleWidget;
        elevationChart::SlopeWidget *slopeWidget;

        // Configuration data
        ErgFileQueryAdapter m_ergFileAdapter;
        RealtimeData m_rtData;

};

#endif // _GC_ElevationChartWindow_h
