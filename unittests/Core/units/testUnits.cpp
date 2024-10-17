#include "Core/Units.h"

#include <QTest>


class TestUnits: public QObject
{
    Q_OBJECT

private slots:
    void kphToPaceTimeRunTrash() {
        QCOMPARE(kphToPaceTime(0.01, true), QTime(0, 0, 0));
        QCOMPARE(kphToPaceTime(100, true), QTime());
        QCOMPARE(kphToPaceTime(0.01, false), QTime(0, 0, 0));
        QCOMPARE(kphToPaceTime(160, false), QTime());
    }

    void kphToPaceTimeRunMetric() {
        QCOMPARE(kphToPaceTime(30, true), QTime(0, 2, 0));
        QCOMPARE(kphToPaceTime(24, true), QTime(0, 2, 30));
        QCOMPARE(kphToPaceTime(15, true), QTime(0, 4, 0));
        QCOMPARE(kphToPaceTime(13.8, true), QTime(0, 4, 21));
    }

    void kphToPaceTimeRunImperial() {
        QCOMPARE(kphToPaceTime(30, false), QTime(0, 3, 13));
        QCOMPARE(kphToPaceTime(24, false), QTime(0, 4, 1));
        QCOMPARE(kphToPaceTime(15, false), QTime(0, 6, 26));
        QCOMPARE(kphToPaceTime(13.8, false), QTime(0, 7, 0));
    }

    void kphToPaceStringRunTrash() {
        QCOMPARE(kphToPace(0.01, true), "00:00");
        QCOMPARE(kphToPace(100, true), "xx:xx");
        QCOMPARE(kphToPace(0.01, false), "00:00");
        QCOMPARE(kphToPace(160, false), "xx:xx");
    }

    void kphToPaceStringRunMetric() {
        QCOMPARE(kphToPace(30, true), "02:00");
        QCOMPARE(kphToPace(24, true), "02:30");
        QCOMPARE(kphToPace(15, true), "04:00");
        QCOMPARE(kphToPace(13.8, true), "04:21");
    }

    void kphToPaceStringRunImperial() {
        QCOMPARE(kphToPace(30, false), "03:13");
        QCOMPARE(kphToPace(24, false), "04:01");
        QCOMPARE(kphToPace(15, false), "06:26");
        QCOMPARE(kphToPace(13.8, false), "07:00");
    }
};


QTEST_MAIN(TestUnits)
#include "testUnits.moc"
