#include <QApplication>
#include <QQuickWidget>
#include <QQmlEngine>
#include <QQuickItem>
#include <QWidget>
#include <QHBoxLayout>
#include <QVector>
#include <QPointF>
#include <QtMath>

#include "LineCurveItem.h"

static QVector<QPointF> makeSine(int n = 2000)
{
    QVector<QPointF> v;
    v.reserve(n);
    for (int i = 0; i < n; ++i) {
        const double x = double(i);
        const double y = 200.0 + 150.0 * std::sin(x * 0.02)
                                + 40.0 * std::sin(x * 0.13);
        v.append({x, y});
    }
    return v;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qmlRegisterType<LineCurveItem>("GC.Proto", 1, 0, "LineCurveItem");

    auto *w = new QQuickWidget;
    w->setResizeMode(QQuickWidget::SizeRootObjectToView);
    w->setSource(QUrl("qrc:/main.qml"));

    QWidget window;
    auto *lay = new QHBoxLayout(&window);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(w);
    window.resize(1000, 400);
    window.setWindowTitle("QSG LineCurve prototype");
    window.show();

    auto *curve = w->rootObject()->findChild<LineCurveItem *>("curve");
    if (curve) {
        const auto samples = makeSine();
        curve->setSamples(samples, 0, samples.size() - 1, 0, 400);
    }

    return app.exec();
}
