#include "Gui/CalendarData.h"
#include "Core/Season.h"

#include <QTest>
#include <QUuid>


class TestCalendarData: public QObject
{
    Q_OBJECT

private slots:

    void layoutCalendarEntry() {

        QList<CalendarEntry> entries = {
            { "", "", "", "", "", Qt::red, "", QTime(9, 0), 3600 },
            { "", "", "", "", "", Qt::red, "", QTime(9, 30), 3600 },
            { "", "", "", "", "", Qt::red, "", QTime(10, 30), 1800 },
            { "", "", "", "", "", Qt::red, "", QTime(11, 0), 1800 },
            { "", "", "", "", "", Qt::red, "", QTime(11, 15), 3600 },
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
    }

    void seasonEventReference_prefersExplicitId() {

        Season season;
        season.setId(QUuid("{00000000-0000-0000-0000-000000000321}"));

        SeasonEvent event("Race", QDate(2026, 4, 11), 1, "desc", "event-123");

        QCOMPARE(calendarSeasonEventReference(season, event, 7), QString("event-123"));
    }

    void seasonEventReference_fallsBackToStableSeasonAndIndex() {

        Season season;
        season.setId(QUuid("{00000000-0000-0000-0000-000000000654}"));

        SeasonEvent first("Race", QDate(2026, 4, 11), 1, "same");
        SeasonEvent second("Race", QDate(2026, 4, 11), 1, "same");

        QCOMPARE(calendarSeasonEventReference(season, first, 0),
                 QString("season:{00000000-0000-0000-0000-000000000654}:event:0"));
        QCOMPARE(calendarSeasonEventReference(season, second, 1),
                 QString("season:{00000000-0000-0000-0000-000000000654}:event:1"));
    }
};


QTEST_MAIN(TestCalendarData)
#include "testCalendarData.moc"
