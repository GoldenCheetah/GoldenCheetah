#include "Core/Season.h"

#include <QTest>


class TestSeasonOffset: public QObject
{
    Q_OBJECT

private slots:
    void isValidFalse() {
        SeasonOffset offset;
        QCOMPARE(offset.isValid(), false);
    }

    void isValidTrue() {
        SeasonOffset offset(-1, 1, 1);
        QCOMPARE(offset.isValid(), true);
    }

    void significantItemInvalid() {
        SeasonOffset offset;
        std::pair<SeasonOffset::SeasonOffsetType, int> item = offset.getSignificantItem();
        QCOMPARE(item.first, SeasonOffset::invalid);
        QCOMPARE(item.second, 0);
    }

    void significantItemYear() {
        SeasonOffset offset(-2, 1, 1);
        std::pair<SeasonOffset::SeasonOffsetType, int> item = offset.getSignificantItem();
        QCOMPARE(item.first, SeasonOffset::year);
        QCOMPARE(item.second, -2);
    }

    void significantItemMonth() {
        SeasonOffset offset(1, -2, 1);
        std::pair<SeasonOffset::SeasonOffsetType, int> item = offset.getSignificantItem();
        QCOMPARE(item.first, SeasonOffset::month);
        QCOMPARE(item.second, -2);
    }

    void significantItemWeek() {
        SeasonOffset offset(1, 1, -2);
        std::pair<SeasonOffset::SeasonOffsetType, int> item = offset.getSignificantItem();
        QCOMPARE(item.first, SeasonOffset::week);
        QCOMPARE(item.second, -2);
    }
};


QTEST_MAIN(TestSeasonOffset)
#include "testSeasonOffset.moc"
