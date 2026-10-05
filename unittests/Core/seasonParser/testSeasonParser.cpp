#include "Core/Seasons.h"
#include "Core/Season.h"

#include <QTest>
#include <QFile>
#include <QList>


class TestSeasonParser: public QObject
{
    Q_OBJECT

private slots:
    void readSeasons() {
        QFile file("seasons.xml");
        bool idEnriched = false;
        QList<Season> seasons = SeasonParser::readSeasons(&file, &idEnriched);

        QCOMPARE(seasons.size(), 5);
        QCOMPARE(idEnriched, true);

        int sx = 0;

        QCOMPARE(seasons[sx].getName(), "3-0 months ago");
        QCOMPARE(seasons[sx].getType(), Season::season);
        QCOMPARE(seasons[sx].id().toString(), "{728c23b7-d576-482c-a16a-d28c2ea9d76f}");
        QCOMPARE(seasons[sx].getSeed(), 0);
        QCOMPARE(seasons[sx].getLow(), -50);
        QCOMPARE(seasons[sx].getAbsoluteStart(), QDate());
        QCOMPARE(seasons[sx].getAbsoluteEnd(), QDate());
        QCOMPARE(seasons[sx].getOffsetStart(), SeasonOffset(1, -3, 1, false));
        QCOMPARE(seasons[sx].getOffsetEnd(), SeasonOffset(1, 1, 0, false));
        QCOMPARE(seasons[sx].isYtd(), false);
        QCOMPARE(seasons[sx].getLength(), SeasonLength());
        QCOMPARE(seasons[sx].events.size(), 0);
        QCOMPARE(seasons[sx].phases.size(), 0);
        QCOMPARE(seasons[sx].isMacrocycle(), false);
        QCOMPARE(seasons[sx].getModelType(), Season::ModelType::none);
        QCOMPARE(seasons[sx].getFirstDayOfWeek(), Qt::Monday);

        ++sx;
        QCOMPARE(seasons[sx].getName(), "6-3 months ago");
        QCOMPARE(seasons[sx].getType(), Season::season);
        QCOMPARE(seasons[sx].id().toString(), "{80348210-8dba-413d-b016-31db8813dffa}");
        QCOMPARE(seasons[sx].getSeed(), 0);
        QCOMPARE(seasons[sx].getLow(), -50);
        QCOMPARE(seasons[sx].getAbsoluteStart(), QDate());
        QCOMPARE(seasons[sx].getAbsoluteEnd(), QDate());
        QCOMPARE(seasons[sx].getOffsetStart(), SeasonOffset(1, -6, 1, false));
        QCOMPARE(seasons[sx].getOffsetEnd(), SeasonOffset());
        QCOMPARE(seasons[sx].isYtd(), false);
        QCOMPARE(seasons[sx].getLength(), SeasonLength(0, 3, 0));
        QCOMPARE(seasons[sx].events.size(), 0);
        QCOMPARE(seasons[sx].phases.size(), 0);
        QCOMPARE(seasons[sx].isMacrocycle(), false);
        QCOMPARE(seasons[sx].getModelType(), Season::ModelType::none);
        QCOMPARE(seasons[sx].getFirstDayOfWeek(), Qt::Monday);

        ++sx;
        QCOMPARE(seasons[sx].getName(), "2023 - YTD");
        QCOMPARE(seasons[sx].getType(), Season::season);
        QCOMPARE(seasons[sx].id().toString(), "{e41cbecf-9dac-41db-bae1-df45877a09ae}");
        QCOMPARE(seasons[sx].getSeed(), 0);
        QCOMPARE(seasons[sx].getLow(), -50);
        QCOMPARE(seasons[sx].getAbsoluteStart(), QDate(2023, 1, 1));
        QCOMPARE(seasons[sx].getAbsoluteEnd(), QDate());
        QCOMPARE(seasons[sx].getOffsetStart(), SeasonOffset());
        QCOMPARE(seasons[sx].getOffsetEnd(), SeasonOffset());
        QCOMPARE(seasons[sx].isYtd(), true);
        QCOMPARE(seasons[sx].getLength(), SeasonLength());
        QCOMPARE(seasons[sx].events.size(), 0);
        QCOMPARE(seasons[sx].phases.size(), 0);
        QCOMPARE(seasons[sx].isMacrocycle(), false);
        QCOMPARE(seasons[sx].getModelType(), Season::ModelType::none);
        QCOMPARE(seasons[sx].getFirstDayOfWeek(), Qt::Monday);

        ++sx;
        QCOMPARE(seasons[sx].getName(), "2023");
        QCOMPARE(seasons[sx].getType(), Season::season);
        QCOMPARE(seasons[sx].id().toString(), "{ed5613a6-875a-483b-a59d-dec36b79fa21}");
        QCOMPARE(seasons[sx].getSeed(), 0);
        QCOMPARE(seasons[sx].getLow(), -50);
        QCOMPARE(seasons[sx].getAbsoluteStart(), QDate(2023, 1, 1));
        QCOMPARE(seasons[sx].getAbsoluteEnd(), QDate());
        QCOMPARE(seasons[sx].getOffsetStart(), SeasonOffset());
        QCOMPARE(seasons[sx].getOffsetEnd(), SeasonOffset());
        QCOMPARE(seasons[sx].isYtd(), false);
        QCOMPARE(seasons[sx].getLength(), SeasonLength(1, 0, 0));
        QCOMPARE(seasons[sx].events.size(), 0);
        QCOMPARE(seasons[sx].phases.size(), 0);
        QCOMPARE(seasons[sx].isMacrocycle(), false);
        QCOMPARE(seasons[sx].getModelType(), Season::ModelType::none);
        QCOMPARE(seasons[sx].getFirstDayOfWeek(), Qt::Monday);

        ++sx;
        QCOMPARE(seasons[sx].getName(), "2024");
        QCOMPARE(seasons[sx].getType(), Season::season);
        QCOMPARE(seasons[sx].id().toString(), "{ea549d73-7d8b-4df4-a16e-0ce50f92d131}");
        QCOMPARE(seasons[sx].getSeed(), 0);
        QCOMPARE(seasons[sx].getLow(), -50);
        QCOMPARE(seasons[sx].getAbsoluteStart(), QDate(2024, 1, 1));
        QCOMPARE(seasons[sx].getAbsoluteEnd(), QDate(2024, 12, 31));
        QCOMPARE(seasons[sx].getOffsetStart(), SeasonOffset());
        QCOMPARE(seasons[sx].getOffsetEnd(), SeasonOffset());
        QCOMPARE(seasons[sx].isYtd(), false);
        QCOMPARE(seasons[sx].getLength(), SeasonLength());
        QCOMPARE(seasons[sx].events.size(), 1);
        QCOMPARE(seasons[sx].events[0].name, "Event");
        QCOMPARE(seasons[sx].events[0].date, QDate(2024, 10, 19));
        QCOMPARE(seasons[sx].events[0].priority, 1);
        QCOMPARE(seasons[sx].events[0].description, "Description");
        QCOMPARE_NE(seasons[sx].events[0].id, "");
        QCOMPARE(seasons[sx].phases.size(), 5);
        QCOMPARE(seasons[sx].phases[0].getName(), "Phase");
        QCOMPARE(seasons[sx].phases[0].getAbsoluteStart(), QDate(2024, 1, 25));
        QCOMPARE(seasons[sx].phases[0].getAbsoluteEnd(), QDate(2024, 1, 31));
        QCOMPARE(seasons[sx].phases[0].getType(), Phase::phase);
        QCOMPARE(seasons[sx].phases[0].id().toString(), "{a3d28e7c-bf42-4a04-8928-c18671c698bc}");
        QCOMPARE(seasons[sx].phases[0].getSeed(), 0);
        QCOMPARE(seasons[sx].phases[0].getLow(), -50);
        QCOMPARE(seasons[sx].phases[1].getName(), "Prep");
        QCOMPARE(seasons[sx].phases[1].getAbsoluteStart(), QDate(2024, 2, 1));
        QCOMPARE(seasons[sx].phases[1].getAbsoluteEnd(), QDate(2024, 2, 10));
        QCOMPARE(seasons[sx].phases[1].getType(), Phase::prep);
        QCOMPARE(seasons[sx].phases[1].id().toString(), "{86489889-ba6e-4006-a7ea-2f9bb7be0620}");
        QCOMPARE(seasons[sx].phases[1].getSeed(), 0);
        QCOMPARE(seasons[sx].phases[1].getLow(), -50);
        QCOMPARE(seasons[sx].phases[2].getName(), "Base");
        QCOMPARE(seasons[sx].phases[2].getAbsoluteStart(), QDate(2024, 3, 1));
        QCOMPARE(seasons[sx].phases[2].getAbsoluteEnd(), QDate(2024, 3, 31));
        QCOMPARE(seasons[sx].phases[2].getType(), Phase::base);
        QCOMPARE(seasons[sx].phases[2].id().toString(), "{f7147da0-de38-41de-9bee-e50562f1dd5f}");
        QCOMPARE(seasons[sx].phases[2].getSeed(), 0);
        QCOMPARE(seasons[sx].phases[2].getLow(), -50);
        QCOMPARE(seasons[sx].phases[3].getName(), "Build");
        QCOMPARE(seasons[sx].phases[3].getAbsoluteStart(), QDate(2024, 4, 1));
        QCOMPARE(seasons[sx].phases[3].getAbsoluteEnd(), QDate(2024, 4, 30));
        QCOMPARE(seasons[sx].phases[3].getType(), Phase::build);
        QCOMPARE(seasons[sx].phases[3].id().toString(), "{98c59d49-65a8-4555-b7c2-8622f24d6091}");
        QCOMPARE(seasons[sx].phases[3].getSeed(), 0);
        QCOMPARE(seasons[sx].phases[3].getLow(), -50);
        QCOMPARE(seasons[sx].phases[4].getName(), "Camp");
        QCOMPARE(seasons[sx].phases[4].getAbsoluteStart(), QDate(2024, 5, 1));
        QCOMPARE(seasons[sx].phases[4].getAbsoluteEnd(), QDate(2024, 5, 31));
        QCOMPARE(seasons[sx].phases[4].getType(), Phase::camp);
        QCOMPARE(seasons[sx].phases[4].id().toString(), "{13fc11c3-4dd1-49df-b927-5d5e33074ee4}");
        QCOMPARE(seasons[sx].phases[4].getSeed(), 0);
        QCOMPARE(seasons[sx].phases[4].getLow(), -50);
        QCOMPARE(seasons[sx].isMacrocycle(), false);
        QCOMPARE(seasons[sx].getModelType(), Season::ModelType::none);
        QCOMPARE(seasons[sx].getFirstDayOfWeek(), Qt::Monday);
    }


    void readSeasonsMacroplan() {
        QFile file("seasons-macroplan.xml");
        QList<Season> seasons = SeasonParser::readSeasons(&file);

        QCOMPARE(seasons.size(), 1);

        int sx = 0;

        QCOMPARE(seasons[sx].getName(), "Macrocycle");
        QCOMPARE(seasons[sx].getType(), Season::season);
        QCOMPARE(seasons[sx].id().toString(), "{fa549d73-7d8b-4df4-a16e-0ce50f92d131}");
        QCOMPARE(seasons[sx].getSeed(), 0);
        QCOMPARE(seasons[sx].getLow(), -50);
        QCOMPARE(seasons[sx].getAbsoluteStart(), QDate(2026, 1, 1));
        QCOMPARE(seasons[sx].getAbsoluteEnd(), QDate(2026, 12, 31));
        QCOMPARE(seasons[sx].getOffsetStart(), SeasonOffset());
        QCOMPARE(seasons[sx].getOffsetEnd(), SeasonOffset());
        QCOMPARE(seasons[sx].isYtd(), false);
        QCOMPARE(seasons[sx].getLength(), SeasonLength());
        QCOMPARE(seasons[sx].events.size(), 3);
        QCOMPARE(seasons[sx].events[0].name, "Event-1");
        QCOMPARE(seasons[sx].events[0].date, QDate(2026, 2, 10));
        QCOMPARE(seasons[sx].events[0].priority, 1);
        QCOMPARE(seasons[sx].events[0].targetLTS, 105);
        QCOMPARE(seasons[sx].events[0].targetCP, 300);
        QCOMPARE(seasons[sx].events[0].targetFTP, 301);
        QCOMPARE(seasons[sx].events[0].description, "Description");
        QCOMPARE(seasons[sx].events[0].id, "{bf53a06a-bf02-4d74-83ac-594e31fd4e42}");
        QCOMPARE(seasons[sx].events[1].name, "Event-2");
        QCOMPARE(seasons[sx].events[1].date, QDate(2026, 2, 10));
        QCOMPARE(seasons[sx].events[1].priority, 1);
        QCOMPARE(seasons[sx].events[1].targetLTS, 0);
        QCOMPARE(seasons[sx].events[1].targetCP, 0);
        QCOMPARE(seasons[sx].events[1].targetFTP, 0);
        QCOMPARE(seasons[sx].events[1].description, "Description");
        QCOMPARE(seasons[sx].events[1].id, "{e7a4daee-5877-432c-b11a-781fe9684dda}");
        QCOMPARE(seasons[sx].events[2].name, "Event-3");
        QCOMPARE(seasons[sx].events[2].date, QDate(2026, 2, 10));
        QCOMPARE(seasons[sx].events[2].priority, 1);
        QCOMPARE(seasons[sx].events[2].targetLTS, 0);
        QCOMPARE(seasons[sx].events[2].targetCP, 0);
        QCOMPARE(seasons[sx].events[2].targetFTP, 0);
        QCOMPARE(seasons[sx].events[2].description, "Description");
        QCOMPARE(seasons[sx].events[2].id, "{ae37cfb9-9b2e-4869-b9b9-5dccd0780c8f}");
        QCOMPARE(seasons[sx].phases.size(), 3);
        QCOMPARE(seasons[sx].phases[0].getName(), "Mesocycle-1");
        QCOMPARE(seasons[sx].phases[0].getAbsoluteStart(), QDate(2026, 1, 5));
        QCOMPARE(seasons[sx].phases[0].getAbsoluteEnd(), QDate(2026, 1, 25));
        QCOMPARE(seasons[sx].phases[0].getType(), Phase::mesocycle);
        QCOMPARE(seasons[sx].phases[0].id().toString(), "{a4d28e7c-bf42-4a04-8928-c18671c698bc}");
        QCOMPARE(seasons[sx].phases[0].getSeed(), 0);
        QCOMPARE(seasons[sx].phases[0].getLow(), -50);
        QCOMPARE(seasons[sx].phases[0].getDescription(), "Description");
        QCOMPARE(seasons[sx].phases[0].hasMesocycle(), true);
        QCOMPARE(seasons[sx].phases[0].numMicrocycles(), 3);
        QVERIFY(seasons[sx].phases[0].getMesocycle() != nullptr);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getTargetCP(), 300);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getTargetFTP(), 301);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getTargetLTS(), 100);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getMicrocyclesFrequency(), 12);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getMicrocyclesLoad(), 450);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getMicrocyclesIntensity(), 178.0f / 450.0f);
        Microcycle *micro = seasons[sx].phases[0].getMesocycle()->getMicrocycle(0);
        QCOMPARE(micro->getFrequency(), 5);
        QCOMPARE(micro->getIntensity(), 0.5f);
        QCOMPARE(micro->getLoad(), 140);
        QCOMPARE(micro->getTime(), 18000);
        QCOMPARE(micro->getDescription(), "My goals for microcycle 1");
        micro = seasons[sx].phases[0].getMesocycle()->getMicrocycle(1);
        QCOMPARE(micro->getFrequency(), 4);
        QCOMPARE(micro->getIntensity(), 0.4f);
        QCOMPARE(micro->getLoad(), 150);
        QCOMPARE(micro->getTime(), 18001);
        QCOMPARE(micro->getDescription(), "My goals for microcycle 2");
        micro = seasons[sx].phases[0].getMesocycle()->getMicrocycle(2);
        QCOMPARE(micro->getFrequency(), 3);
        QCOMPARE(micro->getIntensity(), 0.3f);
        QCOMPARE(micro->getLoad(), 160);
        QCOMPARE(micro->getTime(), 18002);
        QCOMPARE(micro->getDescription(), "My goals for microcycle 3");
        micro = seasons[sx].phases[0].getMesocycle()->getMicrocycle(3);
        QCOMPARE(micro, nullptr);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlotSportOffsets(), (QMap<QString, QList<int>> {
            { "Bike", { 1, 3, 4 } },
            { "Run", { 1 } }
        }));
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->hasSlot(1, "Bike"), true);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(1, "Bike")->getDayOffset(), 1);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(1, "Bike")->getSport(), "Bike");
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(1, "Bike")->getLoad(), 25);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(1, "Bike")->getTime(), 19001);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(1, "Bike")->getStatus(), Slot::StatusType::generated);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(1, "Bike")->getDescription(), "Slot-1");
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(1, "Bike")->getActivities().count(), 2);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(1, "Bike")->hasActivity("{UUID1}"), true);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(1, "Bike")->hasActivity("{UUID2}"), true);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->hasSlot(1, "Run"), true);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(1, "Run")->getDayOffset(), 1);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(1, "Run")->getSport(), "Run");
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(1, "Run")->getLoad(), 40);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(1, "Run")->getTime(), 19004);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(1, "Run")->getStatus(), Slot::StatusType::generated);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(1, "Run")->getDescription(), "Slot-4");
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(1, "Run")->getActivities().count(), 0);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->hasSlot(3, "Bike"), true);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(3, "Bike")->getDayOffset(), 3);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(3, "Bike")->getSport(), "Bike");
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(3, "Bike")->getLoad(), 30);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(3, "Bike")->getTime(), 19002);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(3, "Bike")->getStatus(), Slot::StatusType::generated);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(3, "Bike")->getDescription(), "Slot-2");
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->hasSlot(4, "Bike"), true);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(4, "Bike")->getDayOffset(), 4);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(4, "Bike")->getSport(), "Bike");
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(4, "Bike")->getLoad(), 35);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(4, "Bike")->getTime(), 19003);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(4, "Bike")->getStatus(), Slot::StatusType::adjusted);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(4, "Bike")->getDescription(), "Slot-3");
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(4, "Bike")->getActivities().count(), 1);
        QCOMPARE(seasons[sx].phases[0].getMesocycle()->getSlot(4, "Bike")->hasActivity("{UUID4}"), true);
        QCOMPARE(seasons[sx].phases[1].getName(), "Mesocycle-2");
        QCOMPARE(seasons[sx].phases[1].getAbsoluteStart(), QDate(2026, 2, 2));
        QCOMPARE(seasons[sx].phases[1].getAbsoluteEnd(), QDate(2026, 4, 19));
        QCOMPARE(seasons[sx].phases[1].getType(), Phase::mesocycle);
        QCOMPARE(seasons[sx].phases[1].id().toString(), "{96489889-ba6e-4006-a7ea-2f9bb7be0620}");
        QCOMPARE(seasons[sx].phases[1].getSeed(), 0);
        QCOMPARE(seasons[sx].phases[1].getLow(), -50);
        QCOMPARE(seasons[sx].phases[1].getDescription(), "");
        QCOMPARE(seasons[sx].phases[1].hasMesocycle(), true);
        QCOMPARE(seasons[sx].phases[1].numMicrocycles(), 11);
        QVERIFY(seasons[sx].phases[1].getMesocycle() != nullptr);
        QCOMPARE(seasons[sx].phases[1].getMesocycle()->getTargetCP(), 0);
        QCOMPARE(seasons[sx].phases[1].getMesocycle()->getTargetFTP(), 0);
        QCOMPARE(seasons[sx].phases[1].getMesocycle()->getTargetLTS(), 0);
        micro = seasons[sx].phases[1].getMesocycle()->getMicrocycle(0);
        QCOMPARE(micro->getFrequency(), 5);
        QCOMPARE(micro->getLoad(), 150);
        QCOMPARE(micro->getIntensity(), 0.7f);
        QCOMPARE(micro->getTime(), 0);
        micro = seasons[sx].phases[1].getMesocycle()->getMicrocycle(1);
        QCOMPARE(micro->getFrequency(), 0);
        QCOMPARE(micro->getLoad(), 160);
        QCOMPARE(micro->getIntensity(), 0.7f);
        QCOMPARE(micro->getTime(), 0);
        micro = seasons[sx].phases[1].getMesocycle()->getMicrocycle(2);
        QCOMPARE(micro->getFrequency(), 0);
        QCOMPARE(micro->getLoad(), 160);
        QCOMPARE(micro->getIntensity(), 0.7f);
        QCOMPARE(micro->getTime(), 0);
        micro = seasons[sx].phases[1].getMesocycle()->getMicrocycle(3);
        QCOMPARE(micro->getFrequency(), 0);
        QCOMPARE(micro->getLoad(), 170);
        QCOMPARE(micro->getIntensity(), 0.7f);
        QCOMPARE(micro->getTime(), 1800);
        micro = seasons[sx].phases[1].getMesocycle()->getMicrocycle(4);
        QCOMPARE(micro->getFrequency(), 5);
        QCOMPARE(micro->getLoad(), 0);
        QCOMPARE(micro->getIntensity(), 0.7f);
        QCOMPARE(micro->getTime(), 0);
        micro = seasons[sx].phases[1].getMesocycle()->getMicrocycle(5);
        QCOMPARE(micro->getFrequency(), 5);
        QCOMPARE(micro->getLoad(), 0);
        QCOMPARE(micro->getIntensity(), 0.7f);
        micro = seasons[sx].phases[1].getMesocycle()->getMicrocycle(6);
        QCOMPARE(micro->getFrequency(), 5);
        QCOMPARE(micro->getLoad(), 0);
        QCOMPARE(micro->getIntensity(), 0.7f);
        micro = seasons[sx].phases[1].getMesocycle()->getMicrocycle(7);
        QCOMPARE(micro->getFrequency(), 5);
        QCOMPARE(micro->getLoad(), 210);
        QCOMPARE(micro->getIntensity(), 0.0f);
        micro = seasons[sx].phases[1].getMesocycle()->getMicrocycle(8);
        QCOMPARE(micro->getFrequency(), 5);
        QCOMPARE(micro->getLoad(), 220);
        QCOMPARE(micro->getIntensity(), 0.0f);
        micro = seasons[sx].phases[1].getMesocycle()->getMicrocycle(9);
        QCOMPARE(micro->getFrequency(), 5);
        QCOMPARE(micro->getLoad(), 220);
        QCOMPARE(micro->getIntensity(), 0.0f);
        micro = seasons[sx].phases[1].getMesocycle()->getMicrocycle(10);
        QCOMPARE(micro->getFrequency(), 5);
        QCOMPARE(micro->getLoad(), 230);
        QCOMPARE(micro->getIntensity(), 0.7f);
        QCOMPARE(seasons[sx].phases[1].getMesocycle()->getSlotSportOffsets(), (QMap<QString, QList<int>> { { "Bike", { 5 } }, }));
        QCOMPARE(seasons[sx].phases[1].getMesocycle()->hasSlot(4, "Bike"), false);
        QCOMPARE(seasons[sx].phases[1].getMesocycle()->hasSlot(5, "Bike"), true);
        QCOMPARE(seasons[sx].phases[1].getMesocycle()->getSlot(5, "Bike")->getDayOffset(), 5);
        QCOMPARE(seasons[sx].phases[1].getMesocycle()->getSlot(5, "Bike")->getSport(), "Bike");
        QCOMPARE(seasons[sx].phases[1].getMesocycle()->getSlot(5, "Bike")->getLoad(), 55);
        QCOMPARE(seasons[sx].phases[1].getMesocycle()->getSlot(5, "Bike")->getStatus(), Slot::StatusType::adjusted);
        QCOMPARE(seasons[sx].phases[2].getName(), "non-Meso");
        QCOMPARE(seasons[sx].phases[2].getAbsoluteStart(), QDate(2026, 1, 20));
        QCOMPARE(seasons[sx].phases[2].getAbsoluteEnd(), QDate(2026, 2, 10));
        QCOMPARE(seasons[sx].phases[2].getType(), Phase::phase);
        QCOMPARE(seasons[sx].phases[2].id().toString(), "{a7147da0-de38-41de-9bee-e50562f1dd5f}");
        QCOMPARE(seasons[sx].phases[2].getSeed(), 0);
        QCOMPARE(seasons[sx].phases[2].getLow(), -50);
        QCOMPARE(seasons[sx].phases[2].hasMesocycle(), false);
        QCOMPARE(seasons[sx].phases[2].numMicrocycles(), 4);
        QVERIFY(seasons[sx].phases[2].getMesocycle() == nullptr);
        QCOMPARE(seasons[sx].isMacrocycle(), true);
        QCOMPARE(seasons[sx].getModelType(), Season::ModelType::skiba);
        QCOMPARE(seasons[sx].getFirstDayOfWeek(), Qt::Wednesday);
    }
};


QTEST_MAIN(TestSeasonParser)
#include "testSeasonParser.moc"
