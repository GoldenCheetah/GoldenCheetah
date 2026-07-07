#include <QTest>
#include <QObject>
#include "Core/SplineLookup.h"

class TestSplineCrash : public QObject
{
    Q_OBJECT

private slots:
    void testEmptySpline() {
        SplineLookup spline;
        // Verify it is empty (default state)
        // Calling valueY(10.0) should return 10.0 and NOT crash.
        double val = spline.valueY(10.0);
        QCOMPARE(val, 10.0);
    }
};

QTEST_MAIN(TestSplineCrash)
#include "testSplineCrash.moc"
