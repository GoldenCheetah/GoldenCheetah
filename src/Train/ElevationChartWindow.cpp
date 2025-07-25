#include <QFormLayout>
#include <QVBoxLayout>

#include "ElevationChartWindow.h"
#include "Context.h"
#include "Colors.h"
#include "HelpWhatsThis.h"
#include <array>


namespace elevationChart {


//Set bubble color based on % grade

// With cxx17 the class compiles to readonly memory and no template parameters are needed.
// Someday...
#if defined(CXX17)
#define CONSTEXPR      constexpr
#define CONSTEXPR_FUNC constexpr
#else constexpr
#define CONSTEXPR      static const
#define CONSTEXPR_FUNC
#endif

struct RangeColorCriteria {
    double m_point;
    QColor m_color;

    CONSTEXPR_FUNC RangeColorCriteria(double p, QColor c) : m_point(p), m_color(c) {}
};

template <typename T, size_t T_size> struct RangeColorMapper {
    std::array<T, T_size> m_colorMap;

    QColor toColor(double m) const {
        if (m <= m_colorMap[0].m_point) return m_colorMap[0].m_color;

        for (size_t i = 1; i < T_size; i++) {
            if (m < m_colorMap[i].m_point) {
                const RangeColorCriteria& start = m_colorMap[i - 1];
                const RangeColorCriteria& end = m_colorMap[i];

                double unit = (m - start.m_point) / (end.m_point - start.m_point);

                int sh, ss, sv;
                start.m_color.getHsv(&sh, &ss, &sv);

                int eh, es, ev;
                end.m_color.getHsv(&eh, &es, &ev);

                return QColor::fromHsv(sh + unit * (eh - sh),  // lerp
                    ss + unit * (es - ss),  // it
                    sv + unit * (ev - sv), // real good
                    128); // 50% transparency
            }
        }

        return m_colorMap[T_size - 1].m_color;
    }
};

#ifdef CXX17
// Template deduction guide for RangeColorMapper
template <typename First, typename... Rest> struct EnforceSame {
    static_assert(std::conjunction_v<std::is_same<First, Rest>...>);
    using type = First;
};
template <typename First, typename... Rest> RangeColorMapper(First, Rest...)
->RangeColorMapper2<typename EnforceSame<First, Rest...>::type, 1 + sizeof...(Rest)>;
#endif

CONSTEXPR RangeColorMapper<RangeColorCriteria, 5> s_gradientToColorMapper{
    RangeColorCriteria(-10., Qt::black),
    RangeColorCriteria(0.,   Qt::white),
    RangeColorCriteria(1.,   Qt::yellow),
    RangeColorCriteria(3.,  QColor(255, 140, 0, 255)), // orange
    RangeColorCriteria(4.,  Qt::red)
};


void BubbleWidget::paintEvent(QPaintEvent *event)
{
    if (!m_rtData || !m_ergFileAdapter || !m_ergFileAdapter->hasGradient())
        return;

    QWidget::paintEvent(event);

    QPainter bubblePainter(this);
    bubblePainter.setRenderHint(QPainter::Antialiasing);
    bubblePainter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // Set bubble painter pen and brush
    QPen bubblePen;
    bubblePen.setColor(Qt::black);
    bubblePen.setWidth(3);
    bubblePen.setStyle(Qt::SolidLine);
    bubblePainter.setPen(bubblePen);

    // Average slope in deltaSeconds seconds (taking into account current speed)

    int lap;
    geolocation geoloc;
    double diffSlope;
    double dummy_gradient;
    double speed = m_rtData->getSpeed();
    double gradientValue = m_rtData->getSlope();
    double distDeltaseconds = speed / 3.6 * deltaSeconds;
    double currDist = m_rtData->getRouteDistance() * 1000.0;
    m_ergFileAdapter->locationAt(currDist + distDeltaseconds, lap, geoloc, dummy_gradient);
    double alt2 = geoloc.Alt();
    m_ergFileAdapter->locationAt(currDist, lap, geoloc, dummy_gradient);
    double alt = geoloc.Alt();
    double averSlope = (alt2 - alt) / distDeltaseconds * 100.0;

    diffSlope = averSlope - gradientValue;

    QColor bubbleColor = s_gradientToColorMapper.toColor(diffSlope);
    bubblePainter.setBrush(bubbleColor);

    double bubbleRadius = std::min(width(), height()) / 5.0;
    double maxDiffSlope = 5.0;
    double bubbleX = width() - bubbleRadius;
    double bubbleY = - height() / maxDiffSlope * std::max(std::min(diffSlope, maxDiffSlope), 0.0) + height() - bubbleRadius;

    bubblePainter.drawEllipse(QPointF(bubbleX, bubbleY), (qreal)bubbleRadius, (qreal)bubbleRadius);

    QString diffSlopeString = (diffSlope < 0.0 ? QString("-") : QString("+")) + QString::number(abs((int)diffSlope)) +
        QString(".") + QString::number(abs((int)(diffSlope * 10.0)) % 10) + QString("%");

    // Display diff gradient text in the bubble
    bubblePainter.drawText(bubbleX - 15 , bubbleY + 5, diffSlopeString);

}


void SlopeWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // Set pen and brush for the plot
    QPen pen(QColor(255,0,0,180));
    pen.setWidth(2);
    pen.setStyle(Qt::SolidLine);
    painter.setPen(pen);
    QBrush brush(QColor(153, 76, 0, 128)); // Blue color with transparency
    painter.setBrush(brush);

    if (plotQ.isEmpty())
        return;

    // Calculate scaling factors
    double xScale = width() / (plotQ.last().x() - plotQ.first().x());
    double yMin = std::numeric_limits<double>::max();
    double yMax;

    for (const QPointF &point : plotQ)
    {
        if (point.y() < yMin) yMin = point.y();
    }

        // yMax is a % of the distance over yMin
    yMax = yMin + (plotQ.last().x() - plotQ.first().x()) * m_yPlotScale / 100.0;
    double yScale = height() / (yMax - yMin);

    // Create a QPolygonF to hold the points
    QPolygonF polygon;
    for (const QPointF &point : plotQ)
    {
        QPointF scaledPoint((point.x() - plotQ.first().x()) * xScale, height() - (point.y() - yMin) * yScale);
        polygon << scaledPoint;
    }

    // Add points to close the polygon at the bottom
    polygon << QPointF((plotQ.last().x() - plotQ.first().x()) * xScale, height());
    polygon << QPointF(0, height());

    // Draw the polygon
    painter.drawPolygon(polygon);

    // Draw a vertical line at current position
    QPen linePen(GColor(CPLOTMARKER));
    linePen.setWidth(2);
    painter.setPen(linePen);
    int currPosX = (m_currPos - plotQ.first().x()) * xScale;
    painter.drawLine(currPosX, 0, currPosX, height());
    QFont font = painter.font();
    font.setPointSize(14);
    painter.setFont(font);
    QString textSlope = QString("%1%").arg(m_slope, 0, 'f', 1);
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(textSlope);
    if (currPosX > textWidth + 5)
        painter.drawText(currPosX - textWidth - 5, 15, textSlope);
    else
        painter.drawText(currPosX + 5, 15, textSlope);

}

} // namespace elevationChart


ElevationChartWindow::ElevationChartWindow(Context *context) :
    GcChartWindow(context)
{
    HelpWhatsThis *helpContents = new HelpWhatsThis(this);
    this->setWhatsThis(helpContents->getWhatsThisText(HelpWhatsThis::ChartTrain_Elevation));

    QWidget *settingsWidget = new QWidget(this);
    HelpWhatsThis *helpConfig = new HelpWhatsThis(settingsWidget);
    settingsWidget->setWhatsThis(helpConfig->getWhatsThisText(HelpWhatsThis::ChartTrain_Elevation));
    settingsWidget->setContentsMargins(0,0,0,0);
    setProperty("color", GColor(CTRAINPLOTBACKGROUND));

    QFormLayout* commonLayout = new QFormLayout(settingsWidget);

    customPlotDistanceLabel = new QLabel(tr("Profile Distance (m)"));
    customPlotDistance = new QSpinBox(this);
    customPlotDistance->setFixedWidth(60);
    if (customPlotDistance->text().trimmed().isEmpty()) customPlotDistance->setValue(500);
    customPlotDistance->setRange(50, 2000);

    commonLayout->addRow(customPlotDistanceLabel, customPlotDistance);

    customDeltaSlopeSecondsLabel = new QLabel(tr("Seconds for delta slope"));
    customDeltaSlopeSeconds = new QSpinBox(this);
    customDeltaSlopeSeconds->setFixedWidth(60);
    if (customDeltaSlopeSeconds->text().trimmed().isEmpty()) customDeltaSlopeSeconds->setValue(10);
    customDeltaSlopeSeconds->setRange(5, 600);

    commonLayout->addRow(customDeltaSlopeSecondsLabel, customDeltaSlopeSeconds);

    customyPlotScaleLabel = new QLabel(tr("Elevation window size (%) relative to current altitude"));
    customyPlotScale = new QSpinBox(this);
    customyPlotScale->setSuffix("%");
    customyPlotScale->setFixedWidth(60);
    if (customyPlotScale->text().trimmed().isEmpty()) customyPlotScale->setValue(10);
    customyPlotScale->setRange(5, 20.0);

    commonLayout->addRow(customyPlotScaleLabel, customyPlotScale);

    setControls(settingsWidget);
    setContentsMargins(0, 0, 0, 0);

    // Data shown in the chart

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(3,3,3,3);

    // Container widget for the overlaid widgets
    QWidget *containerWidget = new QWidget;
    containerWidget->setContentsMargins(3,3,3,3);

    // Layout for the container that will hold both widgets
    QGridLayout *overlayLayout = new QGridLayout(containerWidget);
    overlayLayout->setSpacing(0);
    overlayLayout->setContentsMargins(0,0,0,0);


    slopeWidget = new elevationChart::SlopeWidget(this);
    slopeWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    slopeWidget->setStyleSheet("background:transparent;");
    overlayLayout->addWidget(slopeWidget, 0, 0);


    bubbleWidget = new elevationChart::BubbleWidget(customDeltaSlopeSeconds->value(), this);
    bubbleWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    bubbleWidget->setStyleSheet("background:transparent;");
    overlayLayout->addWidget(bubbleWidget, 0, 0);

    mainLayout->addWidget(containerWidget);
    setChartLayout(mainLayout);

    // get updates..
    connect(context, SIGNAL(telemetryUpdate(RealtimeData)), this, SLOT(telemetryUpdate(RealtimeData)));
    connect(context, SIGNAL(ergFileSelected(ErgFile*)), this, SLOT(ergFileSelected(ErgFile*)));

    ergFileSelected(context->currentErgFile());
}


void ElevationChartWindow::ergFileSelected(ErgFile* f)
{
    if (!f || f->filename() == "" )
        return;
    m_ergFileAdapter.setErgFile(f);
    bubbleWidget->setErgFileAdapter(&m_ergFileAdapter);
}


void
ElevationChartWindow::paintEvent(QPaintEvent *event)
{
    GcChartWindow::paintEvent(event);
}

void
ElevationChartWindow::telemetryUpdate(const RealtimeData &rtData)
{
    // If it is not visible, it saves time
    if (!isVisible() || !m_ergFileAdapter.hasGradient())
        return;
    m_rtData = rtData;
    bubbleWidget->setRealtimeData(&m_rtData);

    int npoints = 30;

    QQueue<QPointF> &plotQ = slopeWidget->getPlotQ();
    plotQ.clear();
    m_ergFileAdapter.resetQueryState();


    double x0 = m_rtData.getRouteDistance() * 1000.0;   //  Current point
    double x1 = std::min(x0 + customPlotDistance->value(), m_ergFileAdapter.Duration());       //  last point drawn
    double xs = std::max(x0 - customPlotDistance->value() / 4.0, 0.0);  // first point drawn

    double pointLength = (x1 - xs) / npoints;

    for (int i = 0; i < npoints; i++)
    {
        int lap;
        double x = xs + pointLength * i;
        double y = m_ergFileAdapter.altitudeAt(x, lap);
        plotQ.enqueue(QPointF(x, y));
    }

    // Data to show current position, slope and elevation window size
    slopeWidget->setSlope(m_rtData.getSlope());
    slopeWidget->setCurrPos(x0);
    slopeWidget->setyPlotScale(customyPlotScale->value());

    // This forces a call to paintEvent(), since it is only called when the mouse is passed over the chart,
    // or when Update() is invoked and it is visible
    update();
}
