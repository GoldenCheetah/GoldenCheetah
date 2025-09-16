#include "Gui/CalendarData.h"

#include <QTest>


class TestCalendarData: public QObject
{
    Q_OBJECT

private slots:

    void layoutCalendarEntry() {
#if QT_VERSION < 0x060000
        QSKIP("Skipping test with Qt5");
#else
        QList<CalendarEntry> entries = {
            { "", "", "", "", Qt::red, "", QTime(9, 0), 3600 },
            { "", "", "", "", Qt::red, "", QTime(9, 30), 3600 },
            { "", "", "", "", Qt::red, "", QTime(10, 30), 1800 },
            { "", "", "", "", Qt::red, "", QTime(11, 0), 1800 },
            { "", "", "", "", Qt::red, "", QTime(11, 15), 3600 },
        };
        CalendarEntryLayouter layouter;
        QList<CalendarEntryLayout> layout = layouter.layout(entries);

        QCOMPARE(layout.count(), entries.count());
        QCOMPARE(layout[0].columnIndex, 0);
        QCOMPARE(layout[0].columnCount, 2);
        QCOMPARE(layout[1].columnIndex, 1);
        QCOMPARE(layout[1].columnCount, 2);
        QCOMPARE(layout[2].columnIndex, 0);
        QCOMPARE(layout[2].columnCount, 1);
        QCOMPARE(layout[3].columnIndex, 0);
        QCOMPARE(layout[3].columnCount, 2);
        QCOMPARE(layout[4].columnIndex, 1);
        QCOMPARE(layout[4].columnCount, 2);
#endif
    }
};


QTEST_MAIN(TestCalendarData)
#include "testCalendarData.moc"
