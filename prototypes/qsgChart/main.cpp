#include <QApplication>
#include <QQuickWidget>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QWidget>
#include <QHBoxLayout>
#include <QVector>
#include <QPointF>
#include <QtMath>
#include <QRandomGenerator>
#include <QElapsedTimer>
#include <QTimer>
#include <atomic>

#include "LineCurveItem.h"
#include "AxisItem.h"

// Counts QQuickWindow::frameSwapped signals over a 1s window and
// publishes the rate as a Q_PROPERTY so QML can bind a Text to it.
// Cheap: a counter increment per frame on the GUI thread.
class FrameCounter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double fps READ fps NOTIFY fpsChanged)
public:
    explicit FrameCounter(QObject *parent = nullptr) : QObject(parent)
    {
        timer_.start();
        tick_.setInterval(500);
        connect(&tick_, &QTimer::timeout, this, &FrameCounter::flush);
        tick_.start();
    }
    double fps() const { return fps_; }
    void bump() { ++frames_; } // safe from render thread via atomic
signals:
    void fpsChanged();
private:
    void flush()
    {
        const double elapsed = timer_.restart() / 1000.0;
        const int n = frames_.exchange(0);
        fps_ = elapsed > 0 ? n / elapsed : 0.0;
        emit fpsChanged();
    }
    QElapsedTimer timer_;
    QTimer tick_;
    std::atomic<int> frames_{0};
    double fps_ = 0.0;
};

namespace {

// Build a workout-shaped power curve: 5 min warmup ramp, 6x 2-min
// intervals at 300W with 1-min recoveries at 120W, 5 min cooldown.
// Sample every second.
QVector<QPointF>
buildPower(int totalSecs)
{
    QVector<QPointF> out;
    out.reserve(totalSecs);
    auto *rng = QRandomGenerator::global();
    for (int t = 0; t < totalSecs; ++t) {
        double target;
        if (t < 300) {
            target = 100 + (t / 300.0) * 100; // 100 → 200W ramp
        } else if (t > totalSecs - 300) {
            const double p = double(totalSecs - t) / 300.0;
            target = 100 + p * 80; // cooldown ramp down
        } else {
            const int intervalBlock = 180; // 2 min on + 1 min off
            const int phase = (t - 300) % intervalBlock;
            target = phase < 120 ? 300 : 120;
        }
        const double noise = rng->bounded(20) - 10;
        out.append(QPointF(double(t), qBound(0.0, target + noise, 400.0)));
    }
    return out;
}

// HR lags power with a ~30s time constant, tops out ~180.
QVector<QPointF>
buildHr(const QVector<QPointF> &power)
{
    QVector<QPointF> out;
    out.reserve(power.size());
    double hr = 75.0;
    for (const auto &p : power) {
        const double target = 75.0 + p.y() * 0.32; // 0W→75, 400W→203
        hr += (target - hr) / 30.0;
        out.append(QPointF(p.x(), qBound(60.0, hr, 200.0)));
    }
    return out;
}

// Altitude: slow rolling hills.
QVector<QPointF>
buildAlt(int totalSecs)
{
    QVector<QPointF> out;
    out.reserve(totalSecs);
    for (int t = 0; t < totalSecs; ++t) {
        const double base = 150.0 + 60.0 * std::sin(t * 0.0012)
                                  + 25.0 * std::sin(t * 0.0053);
        out.append(QPointF(double(t), base));
    }
    return out;
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    bool bench = false;
    int benchSecs = 5;
    for (int i = 1; i < argc; ++i) {
        const QString a = QString::fromLocal8Bit(argv[i]);
        if (a == "--bench") bench = true;
        else if (a.startsWith("--bench-secs=")) benchSecs = a.mid(13).toInt();
    }

    qmlRegisterType<LineCurveItem>("GC.Proto", 1, 0, "LineCurveItem");
    qmlRegisterType<AxisItem>("GC.Proto", 1, 0, "AxisItem");

    const int secs = 60 * 60 * 10; // 10h workout — long-ride stress test
    const auto power = buildPower(secs);
    const auto hr    = buildHr(power);
    const auto alt   = buildAlt(secs);

    auto *w = new QQuickWidget;
    w->setResizeMode(QQuickWidget::SizeRootObjectToView);

    auto *frameCounter = new FrameCounter(w);
    w->engine()->rootContext()->setContextProperty("frameCounter", frameCounter);

    w->setSource(QUrl("qrc:/main.qml"));
    // afterRendering fires even in QQuickWidget's FBO-backed path;
    // frameSwapped does not. The counter uses std::atomic so it is
    // safe to bump from the render thread directly.
    QObject::connect(w->quickWindow(), &QQuickWindow::afterRendering,
                     frameCounter, [frameCounter]() { frameCounter->bump(); },
                     Qt::DirectConnection);

    auto *root = w->rootObject();
    root->setProperty("dataXMin", 0.0);
    root->setProperty("dataXMax", double(secs - 1));
    // Narrow initial view so auto-pan has room to slide.
    root->setProperty("viewXMin", 0.0);
    root->setProperty("viewXMax", double(secs - 1) / 4.0);
    root->setProperty("sampleCount", secs * 3); // three curves
    for (auto *curve : root->findChildren<LineCurveItem *>()) {
        if (curve->property("color").value<QColor>() == QColor("#c0392b"))
            curve->setSamples(power);
        else if (curve->property("color").value<QColor>() == QColor("#2980b9"))
            curve->setSamples(hr);
        else if (curve->property("color").value<QColor>() == QColor("#27ae60"))
            curve->setSamples(alt);
    }

    QWidget window;
    auto *lay = new QHBoxLayout(&window);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(w);
    window.resize(1100, 500);
    window.setWindowTitle("QSG chart prototype — power / HR / alt");
    window.show();

    if (bench) {
        // Kick off auto-pan, sample FPS for benchSecs, print, and quit.
        QTimer::singleShot(400, [root]() {
            QMetaObject::invokeMethod(root, "startAutoPan");
        });
        auto *log = new QTimer(&app);
        int elapsed = 0;
        QObject::connect(log, &QTimer::timeout, [&, frameCounter, root]() {
            ++elapsed;
            qInfo().noquote() << QString("bench t=%1s fps=%2 viewXMin=%3")
                                     .arg(elapsed)
                                     .arg(frameCounter->fps(), 0, 'f', 1)
                                     .arg(root->property("viewXMin").toDouble(), 0, 'f', 1);
            if (elapsed >= benchSecs) QApplication::quit();
        });
        log->start(1000);
    }

    return app.exec();
}

#include "main.moc"
