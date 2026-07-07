#include "Core/Season.h"

#include <QTest>


class TestSeason: public QObject
{
    Q_OBJECT

public:
    TestSeason(): reference(2024, 8, 20) {
    }

private slots:
    void builtInAllDates() {
        Season season;
        season.setName("All Dates");
        season.setOffsetAndLength(-50, 1, 1, 100, 0, 0);
        QCOMPARE(season.getStart(reference), QDate(1974, 1, 1));
        QCOMPARE(season.getEnd(reference), QDate(2073, 12, 31));
    }

    void builtInThisYear() {
        Season season;
        season.setName("This Year");
        season.setOffsetAndLength(0, 1, 1, 1, 0, 0);
        QCOMPARE(season.getStart(reference), QDate(2024, 1, 1));
        QCOMPARE(season.getEnd(reference), QDate(2024, 12, 31));
    }

    void builtInThisMonth() {
        Season season;
        season.setName("This Month");
        season.setOffsetAndLength(1, 0, 1, 0, 1, 0);
        QCOMPARE(season.getStart(reference), QDate(2024, 8, 1));
        QCOMPARE(season.getEnd(reference), QDate(2024, 8, 31));
    }

    void builtInLastMonth() {
        Season season;
        season.setName("Last Month");
        season.setOffsetAndLength(1, -1, 1, 0, 1, 0);
        QCOMPARE(season.getStart(reference), QDate(2024, 7, 1));
        QCOMPARE(season.getEnd(reference), QDate(2024, 7, 31));
    }

    void builtInThisWeek() {
        Season season;
        season.setName("This Week");
        season.setOffsetAndLength(1, 1, 0, 0, 0, 7);
        QCOMPARE(season.getStart(reference), QDate(2024, 8, 19));
        QCOMPARE(season.getEnd(reference), QDate(2024, 8, 25));
    }

    void builtInLastWeek() {
        Season season;
        season.setName("Last Week");
        season.setOffsetAndLength(1, 1, -1, 0, 0, 7);
        QCOMPARE(season.getStart(reference), QDate(2024, 8, 12));
        QCOMPARE(season.getEnd(reference), QDate(2024, 8, 18));
    }

    void builtInLast24Hours() {
        Season season;
        season.setName("Last 24 hours");
        season.setLengthOnly(0, 0, 2);
        QCOMPARE(season.getStart(reference), QDate(2024, 8, 19));
        QCOMPARE(season.getEnd(reference), QDate(2024, 8, 20));
    }

    void builtInLast7Days() {
        Season season;
        season.setName("Last 7 days");
        season.setLengthOnly(0, 0, 7);
        QCOMPARE(season.getEnd(reference), QDate(2024, 8, 20));
    }

    void builtInLast14Days() {
        Season season;
        season.setName("Last 14 days");
        season.setLengthOnly(0, 0, 14);
        QCOMPARE(season.getStart(reference), QDate(2024, 8, 7));
        QCOMPARE(season.getEnd(reference), QDate(2024, 8, 20));
    }

    void builtInLast21Days() {
        Season season;
        season.setName("Last 21 days");
        season.setLengthOnly(0, 0, 21);
        QCOMPARE(season.getStart(reference), QDate(2024, 7, 31));
        QCOMPARE(season.getEnd(reference), QDate(2024, 8, 20));
    }

    void builtInLast28Days() {
        Season season;
        season.setName("Last 28 days");
        season.setLengthOnly(0, 0, 28);
        QCOMPARE(season.getStart(reference), QDate(2024, 7, 24));
        QCOMPARE(season.getEnd(reference), QDate(2024, 8, 20));
    }

    void builtInLast6Weeks() {
        Season season;
        season.setName("Last 6 weeks");
        season.setLengthOnly(0, 0, 42);
        QCOMPARE(season.getStart(reference), QDate(2024, 7, 10));
        QCOMPARE(season.getEnd(reference), QDate(2024, 8, 20));
    }

    void builtInLast2Months() {
        Season season;
        season.setName("Last 2 months");
        season.setLengthOnly(0, 2, 0);
        QCOMPARE(season.getStart(reference), QDate(2024, 6, 21));
        QCOMPARE(season.getEnd(reference), QDate(2024, 8, 20));
    }

    void builtInLast3Months() {
        Season season;
        season.setName("Last 3 months");
        season.setLengthOnly(0, 3, 0);
        QCOMPARE(season.getStart(reference), QDate(2024, 5, 21));
        QCOMPARE(season.getEnd(reference), QDate(2024, 8, 20));
    }

    void builtInLast6Months() {
        Season season;
        season.setName("Last 6 months");
        season.setLengthOnly(0, 6, 0);
        QCOMPARE(season.getStart(reference), QDate(2024, 2, 21));
        QCOMPARE(season.getEnd(reference), QDate(2024, 8, 20));
    }

    void builtInLast12Months() {
        Season season;
        season.setName("Last 12 months");
        season.setLengthOnly(1, 0, 0);
        QCOMPARE(season.getStart(reference), QDate(2023, 8, 21));
        QCOMPARE(season.getEnd(reference), QDate(2024, 8, 20));
    }

    void absStartAbsEnd() {
        Season season;
        season.setAbsoluteStart(QDate(2022, 4, 8));
        season.setAbsoluteEnd(QDate(2023, 3, 7));
        QCOMPARE(season.getStart(reference), QDate(2022, 4, 8));
        QCOMPARE(season.getEnd(reference), QDate(2023, 3, 7));
    }

    void absStartRelEnd() {
        Season season;
        season.setAbsoluteStart(QDate(2022, 4, 8));
        season.setOffsetEnd(1, -5, 1, false);
        QCOMPARE(season.getStart(reference), QDate(2022, 4, 8));
        QCOMPARE(season.getEnd(reference), QDate(2024, 3, 20));
    }

    void absStartDurationAfter() {
        Season season;
        season.setAbsoluteStart(QDate(2022, 4, 8));
        season.setLength(0, 2, 0);
        QCOMPARE(season.getStart(reference), QDate(2022, 4, 8));
        QCOMPARE(season.getEnd(reference), QDate(2022, 6, 7));
    }

    void absStartYtd() {
        Season season;
        season.setAbsoluteStart(QDate(2022, 4, 8));
        season.setYtd();
        QCOMPARE(season.getStart(reference), QDate(2022, 4, 8));
        QCOMPARE(season.getEnd(reference), QDate(2022, 8, 20));
    }

    void absStartYtdOverflow() {
        Season season;
        season.setAbsoluteStart(QDate(2022, 9, 8));
        season.setYtd();
        QCOMPARE(season.getStart(reference), QDate(2022, 9, 8));
        QCOMPARE(season.getEnd(reference), QDate(2023, 8, 20));
    }

    void relStartAbsEnd() {
        Season season;
        season.setOffsetStart(-5, 1, 1, false);
        season.setAbsoluteEnd(QDate(2023, 3, 7));
        QCOMPARE(season.getStart(reference), QDate(2019, 8, 20));
        QCOMPARE(season.getEnd(reference), QDate(2023, 3, 7));
    }

    void relStartRelEndYear() {
        Season season;
        season.setOffsetStart(-2, 1, 1, false);
        season.setOffsetEnd(-1, 1, 1, false);
        QCOMPARE(season.getStart(reference), QDate(2022, 8, 20));
        QCOMPARE(season.getEnd(reference), QDate(2023, 8, 20));
    }

    void relStartRelEndMonthDay() {
        Season season;
        season.setOffsetStart(1, -5, 1, false);
        season.setOffsetEnd(1, 1, -5, false);
        QCOMPARE(season.getStart(reference), QDate(2024, 3, 20));
        QCOMPARE(season.getEnd(reference), QDate(2024, 7, 16));
    }

    void relStartDurationAfter() {
        Season season;
        season.setOffsetStart(1, -5, 1, false);
        season.setLength(0, 2, 0);
        QCOMPARE(season.getStart(reference), QDate(2024, 3, 20));
        QCOMPARE(season.getEnd(reference), QDate(2024, 5, 19));
    }

    void relStartYtd() {
        Season season;
        season.setOffsetStart(1, -2, 1, false);
        season.setYtd();
        QCOMPARE(season.getStart(reference), QDate(2024, 6, 20));
        QCOMPARE(season.getEnd(reference), QDate(2024, 8, 20));
    }

    void relStartYtdOverflow() {
        Season season;
        season.setOffsetStart(1, -21, 1, false);
        season.setYtd();
        QCOMPARE(season.getStart(reference), QDate(2022, 11, 20));
        QCOMPARE(season.getEnd(reference), QDate(2023, 8, 20));
    }

    void durationBeforeAbsEnd() {
        Season season;
        season.setAbsoluteEnd(QDate(2023, 3, 7));
        season.setLength(0, 2, 0);
        QCOMPARE(season.getStart(reference), QDate(2023, 1, 8));
        QCOMPARE(season.getEnd(reference), QDate(2023, 3, 7));
    }

    void durationBeforeRelEnd() {
        Season season;
        season.setOffsetEnd(1, -5, 1, false);
        season.setLength(0, 2, 0);
        QCOMPARE(season.getStart(reference), QDate(2024, 1, 21));
        QCOMPARE(season.getEnd(reference), QDate(2024, 3, 20));
    }

    void isAbsolute() {
        Season season;

        season.setOffsetEnd(1, -5, 1, false);
        season.setLength(0, 2, 0);
        QCOMPARE(season.isAbsolute(), false);

        season.resetTimeRange();
        season.setAbsoluteStart(QDate(2024, 1, 1));
        season.setYtd();
        QCOMPARE(season.isAbsolute(), false);

        season.resetTimeRange();
        season.setAbsoluteStart(QDate(2024, 1, 1));
        season.setAbsoluteEnd(QDate(2024, 2, 3));
        QCOMPARE(season.isAbsolute(), true);

        season.resetTimeRange();
        season.setAbsoluteStart(QDate(2024, 1, 1));
        season.setLength(0, 2, 0);
        QCOMPARE(season.isAbsolute(), true);

        season.resetTimeRange();
        season.setLength(0, 2, 0);
        season.setAbsoluteEnd(QDate(2024, 1, 1));
        QCOMPARE(season.isAbsolute(), true);
    }

private:
    const QDate reference;
};


QTEST_MAIN(TestSeason)
#include "testSeason.moc"
