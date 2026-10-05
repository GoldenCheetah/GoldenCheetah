#include "Core/Season.h"

#include <QTest>


class TestPhaseMesocycle: public QObject
{
    Q_OBJECT

public:
    TestPhaseMesocycle(): ref1(2026, 9, 14), ref2(2026, 10, 11) {
    }

private slots:

    //////////////////////////////////////////////////////////////////////////////
    // Microcycle only

    void microFrequency() {
        Microcycle micro;
        QCOMPARE(micro.getFrequency(), 0);
        QCOMPARE(micro.setFrequency(5), true);
        QCOMPARE(micro.getFrequency(), 5);
        QCOMPARE(micro.setFrequency(-1), false);
        QCOMPARE(micro.getFrequency(), 5);
        QCOMPARE(micro.setFrequency(0), true);
        QCOMPARE(micro.getFrequency(), 0);
    }

    void microLoad() {
        Microcycle micro;
        QCOMPARE(micro.getLoad(), 0);
        QCOMPARE(micro.setLoad(150), true);
        QCOMPARE(micro.getLoad(), 150);
        QCOMPARE(micro.setLoad(-1), false);
        QCOMPARE(micro.getLoad(), 150);
        QCOMPARE(micro.setLoad(0), true);
        QCOMPARE(micro.getLoad(), 0);
    }

    void microIntensity() {
        Microcycle micro;
        QCOMPARE(micro.getIntensity(), 0.0f);
        QCOMPARE(micro.setIntensity(0.75f), true);
        QCOMPARE(micro.getIntensity(), 0.75f);
        QCOMPARE(micro.setIntensity(-1.0f), false);
        QCOMPARE(micro.getIntensity(), 0.75f);
        QCOMPARE(micro.setIntensity(0.0f), true);
        QCOMPARE(micro.getIntensity(), 0.0f);
    }

    void microTime() {
        Microcycle micro;
        QCOMPARE(micro.getTime(), 0);
        QCOMPARE(micro.setTime(3600), true);
        QCOMPARE(micro.getTime(), 3600);
        QCOMPARE(micro.setTime(-1), false);
        QCOMPARE(micro.getTime(), 3600);
        QCOMPARE(micro.setTime(0), true);
        QCOMPARE(micro.getTime(), 0);
    }

    void microDescription() {
        Microcycle micro;
        QCOMPARE(micro.getDescription(), "");
        micro.setDescription("DESCRIPTION");
        QCOMPARE(micro.getDescription(), "DESCRIPTION");
        micro.setDescription("");
        QCOMPARE(micro.getDescription(), "");
    }

    //////////////////////////////////////////////////////////////////////////////
    // Slot only

    void slotStatusToString() {
        QCOMPARE(Slot::stringToStatus(""), Slot::StatusType::generated);
        QCOMPARE(Slot::stringToStatus("BAD"), Slot::StatusType::generated);
        QCOMPARE(Slot::stringToStatus("generated"), Slot::StatusType::generated);
        QCOMPARE(Slot::stringToStatus("adjusted"), Slot::StatusType::adjusted);
    }

    void slotStringToStatus() {
        QCOMPARE(Slot::statusToString(Slot::StatusType::generated), "generated");
        QCOMPARE(Slot::statusToString(Slot::StatusType::adjusted), "adjusted");
    }

    void slotDayOffset() {
        Slot slot;
        QCOMPARE(slot.getDayOffset(), 0);
        QCOMPARE(slot.setDayOffset(10), true);
        QCOMPARE(slot.getDayOffset(), 10);
        QCOMPARE(slot.setDayOffset(-1), false);
        QCOMPARE(slot.getDayOffset(), 10);
        QCOMPARE(slot.setDayOffset(0), true);
        QCOMPARE(slot.getDayOffset(), 0);
    }

    void slotSport() {
        Slot slot;
        QCOMPARE(slot.getSport(), "");
        QCOMPARE(slot.setSport("SPORT"), true);
        QCOMPARE(slot.getSport(), "SPORT");
        QCOMPARE(slot.setSport(""), true);
        QCOMPARE(slot.getSport(), "");
    }

    void slotStatus() {
        Slot slot;
        QCOMPARE(slot.getStatus(), Slot::StatusType::generated);
        QCOMPARE(slot.setStatus(Slot::StatusType::adjusted), true);
        QCOMPARE(slot.getStatus(), Slot::StatusType::adjusted);
        QCOMPARE(slot.setStatus(Slot::StatusType::generated), true);
        QCOMPARE(slot.getStatus(), Slot::StatusType::generated);
    }

    void slotLoad() {
        Slot slot;
        QCOMPARE(slot.hasLoad(), false);
        QCOMPARE(slot.getLoad(), 0);
        QCOMPARE(slot.setLoad(100), true);
        QCOMPARE(slot.hasLoad(), true);
        QCOMPARE(slot.getLoad(), 100);
        QCOMPARE(slot.setLoad(-1), false);
        QCOMPARE(slot.hasLoad(), true);
        QCOMPARE(slot.getLoad(), 100);
        QCOMPARE(slot.setLoad(0), true);
        QCOMPARE(slot.hasLoad(), false);
        QCOMPARE(slot.getLoad(), 0);
    }

    void slotIntensity() {
        Slot slot;
        QCOMPARE(slot.hasIntensity(), false);
        QCOMPARE(slot.getIntensity(), 0.0f);
        QCOMPARE(slot.setIntensity(0.8f), true);
        QCOMPARE(slot.hasIntensity(), true);
        QCOMPARE(slot.getIntensity(), 0.8f);
        QCOMPARE(slot.setIntensity(-1.0f), false);
        QCOMPARE(slot.hasIntensity(), true);
        QCOMPARE(slot.getIntensity(), 0.8f);
        QCOMPARE(slot.setIntensity(0.0f), true);
        QCOMPARE(slot.hasIntensity(), false);
        QCOMPARE(slot.getIntensity(), 0.0f);
    }

    void slotTime() {
        Slot slot;
        QCOMPARE(slot.hasTime(), false);
        QCOMPARE(slot.getTime(), 0);
        QCOMPARE(slot.setTime(100), true);
        QCOMPARE(slot.hasTime(), true);
        QCOMPARE(slot.getTime(), 100);
        QCOMPARE(slot.setTime(-1), false);
        QCOMPARE(slot.hasTime(), true);
        QCOMPARE(slot.getTime(), 100);
        QCOMPARE(slot.setTime(0), true);
        QCOMPARE(slot.hasTime(), false);
        QCOMPARE(slot.getTime(), 0);
    }

    void slotDescription() {
        Slot slot;
        QCOMPARE(slot.hasDescription(), false);
        QCOMPARE(slot.getDescription(), "");
        QCOMPARE(slot.setDescription("DESC"), true);
        QCOMPARE(slot.hasDescription(), true);
        QCOMPARE(slot.getDescription(), "DESC");
        QCOMPARE(slot.setDescription(""), true);
        QCOMPARE(slot.hasDescription(), false);
        QCOMPARE(slot.getDescription(), "");
    }

    void slotActivityIds() {
        Slot slot;
        QCOMPARE(slot.getActivities().count(), 0);
        slot.addActivity("ID1");
        QCOMPARE(slot.getActivities().count(), 1);
        QCOMPARE(slot.hasActivity("ID1"), true);
        QCOMPARE(slot.hasActivity("ID2"), false);
        slot.setActivities(QStringList { "ID2", "ID3", "ID4" });
        QCOMPARE(slot.getActivities().count(), 3);
        QCOMPARE(slot.hasActivity("ID1"), false);
        QCOMPARE(slot.hasActivity("ID2"), true);
        QCOMPARE(slot.hasActivity("ID3"), true);
        QCOMPARE(slot.hasActivity("ID4"), true);
        QCOMPARE(slot.removeActivity("ID5"), false);
        QCOMPARE(slot.getActivities().count(), 3);
        QCOMPARE(slot.removeActivity("ID3"), true);
        QCOMPARE(slot.getActivities().count(), 2);
        QCOMPARE(slot.hasActivity("ID1"), false);
        QCOMPARE(slot.hasActivity("ID2"), true);
        QCOMPARE(slot.hasActivity("ID3"), false);
        QCOMPARE(slot.hasActivity("ID4"), true);
        slot.clearActivities();
        QCOMPARE(slot.getActivities().count(), 0);
        QCOMPARE(slot.hasActivity("ID1"), false);
        QCOMPARE(slot.hasActivity("ID2"), false);
        QCOMPARE(slot.hasActivity("ID3"), false);
        QCOMPARE(slot.hasActivity("ID4"), false);
    }

    //////////////////////////////////////////////////////////////////////////////
    // Mesocycle only

    void mesoMaxMicrocycles() {
        Mesocycle mesocycle;
        QCOMPARE(mesocycle.getMaxMicrocycles(), 0);
        QCOMPARE(mesocycle.setStart(ref1), true);
        QCOMPARE(mesocycle.setMaxMicrocycles(4), true);
        QCOMPARE(mesocycle.getMaxMicrocycles(), 4);
        QCOMPARE(mesocycle.setStart(QDate()), false);
        QCOMPARE(mesocycle.setMaxMicrocycles(-1), false);
        QCOMPARE(mesocycle.getMaxMicrocycles(), 4);
    }

    void mesoToMicrocycle() {
        Mesocycle mesocycle;
        QCOMPARE(mesocycle.toMicrocycle(-1), 0);
        QCOMPARE(mesocycle.toMicrocycle(0), 0);
        QCOMPARE(mesocycle.toMicrocycle(1), 0);
        mesocycle.setStart(ref1);
        mesocycle.setMaxMicrocycles(4);
        QCOMPARE(mesocycle.toMicrocycle(-1), 0);
        QCOMPARE(mesocycle.toMicrocycle(0), 0);
        QCOMPARE(mesocycle.toMicrocycle(1), 0);
        QCOMPARE(mesocycle.toMicrocycle(6), 0);
        QCOMPARE(mesocycle.toMicrocycle(7), 1);
        QCOMPARE(mesocycle.toMicrocycle(8), 1);
        QCOMPARE(mesocycle.toMicrocycle(30), 3);
        QCOMPARE(mesocycle.toMicrocycle(300), 3);
        QCOMPARE(mesocycle.toMicrocycle(ref1.addDays(-1)), 0);
        QCOMPARE(mesocycle.toMicrocycle(ref1.addDays(0)), 0);
        QCOMPARE(mesocycle.toMicrocycle(ref1.addDays(1)), 0);
        QCOMPARE(mesocycle.toMicrocycle(ref1.addDays(6)), 0);
        QCOMPARE(mesocycle.toMicrocycle(ref1.addDays(7)), 1);
        QCOMPARE(mesocycle.toMicrocycle(ref1.addDays(8)), 1);
        QCOMPARE(mesocycle.toMicrocycle(ref1.addDays(30)), 3);
        QCOMPARE(mesocycle.toMicrocycle(ref1.addDays(300)), 3);
    }

    void mesoMicrocycles() {
        Mesocycle mesocycle;
        mesocycle.setStart(ref1);
        mesocycle.setMaxMicrocycles(4);
        QCOMPARE(mesocycle.getMicrocyclesFrequency(), 0);
        QCOMPARE(mesocycle.getMicrocyclesLoad(), 0);
        QCOMPARE(mesocycle.getMicrocyclesIntensity(), 0.0f);
        QCOMPARE(mesocycle.getMicrocyclesTime(), 0);
        QCOMPARE(mesocycle.getMicrocycle(-1), nullptr);
        QVERIFY(mesocycle.getMicrocycle(0) != nullptr);
        QVERIFY(mesocycle.getMicrocycle(1) != nullptr);
        QVERIFY(mesocycle.getMicrocycle(2) != nullptr);
        QVERIFY(mesocycle.getMicrocycle(3) != nullptr);
        QCOMPARE(mesocycle.getMicrocycle(4), nullptr);

        QCOMPARE(mesocycle.getMicrocycle(0)->setFrequency(1), true);
        QCOMPARE(mesocycle.getMicrocycle(0)->setLoad(100), true);
        QCOMPARE(mesocycle.getMicrocycle(0)->setIntensity(0.1f), true);
        QCOMPARE(mesocycle.getMicrocycle(0)->setTime(10), true);

        QCOMPARE(mesocycle.getMicrocycle(1)->setFrequency(2), true);
        QCOMPARE(mesocycle.getMicrocycle(1)->setLoad(200), true);
        QCOMPARE(mesocycle.getMicrocycle(1)->setIntensity(0.2f), true);
        QCOMPARE(mesocycle.getMicrocycle(1)->setTime(20), true);

        QCOMPARE(mesocycle.getMicrocycle(2)->setFrequency(3), true);
        QCOMPARE(mesocycle.getMicrocycle(2)->setLoad(300), true);
        QCOMPARE(mesocycle.getMicrocycle(2)->setIntensity(0.3f), true);
        QCOMPARE(mesocycle.getMicrocycle(2)->setTime(30), true);

        QCOMPARE(mesocycle.getMicrocycle(3)->setFrequency(4), true);
        QCOMPARE(mesocycle.getMicrocycle(3)->setLoad(400), true);
        QCOMPARE(mesocycle.getMicrocycle(3)->setIntensity(0.4f), true);
        QCOMPARE(mesocycle.getMicrocycle(3)->setTime(40), true);

        QCOMPARE(mesocycle.getMicrocyclesFrequency(), 10);
        QCOMPARE(mesocycle.getMicrocyclesLoad(), 1000);
        QCOMPARE(mesocycle.getMicrocyclesIntensity(), 300.0f / 1000.0f);
        QCOMPARE(mesocycle.getMicrocyclesTime(), 100);
    }

    void mesoSlottedFrequency() {
        Mesocycle mesocycle;
        mesocycle.setStart(ref1);
        mesocycle.setMaxMicrocycles(4);
        mesocycle.addSlot(1, "Bike");
        mesocycle.addSlot(1, "Run");
        mesocycle.addSlot(2, "Bike");
        mesocycle.addSlot(3, "Bike");
        mesocycle.addSlot(3, "Run");
        mesocycle.addSlot(7, "Bike");
        mesocycle.addSlot(7, "Run");
        mesocycle.addSlot(8, "Bike");
        mesocycle.addSlot(8, "Run");
        mesocycle.addSlot(9, "Bike");
        mesocycle.addSlot(9, "Run");
        QCOMPARE(mesocycle.getSlottedFrequency(), 11);
        QCOMPARE(mesocycle.getSlottedFrequency("Bike"), 6);
        QCOMPARE(mesocycle.getSlottedFrequency("Run"), 5);
        QCOMPARE(mesocycle.getSlottedFrequency(0), 5);
        QCOMPARE(mesocycle.getSlottedFrequency(0, "Bike"), 3);
        QCOMPARE(mesocycle.getSlottedFrequency(0, "Run"), 2);
        QCOMPARE(mesocycle.getSlottedFrequency(1), 6);
        QCOMPARE(mesocycle.getSlottedFrequency(1, "Bike"), 3);
        QCOMPARE(mesocycle.getSlottedFrequency(1, "Run"), 3);
    }

    void mesoSlottedLoad() {
        Mesocycle mesocycle;
        mesocycle.setStart(ref1);
        mesocycle.setMaxMicrocycles(4);
        mesocycle.addSlot(1, "Bike")->setLoad(100);
        mesocycle.addSlot(1, "Run")->setLoad(10);
        mesocycle.addSlot(2, "Bike")->setLoad(200);
        mesocycle.addSlot(3, "Bike")->setLoad(300);
        mesocycle.addSlot(3, "Run")->setLoad(30);
        mesocycle.addSlot(7, "Bike")->setLoad(700);
        mesocycle.addSlot(7, "Run")->setLoad(70);
        mesocycle.addSlot(8, "Bike")->setLoad(800);
        mesocycle.addSlot(9, "Bike")->setLoad(900);
        mesocycle.addSlot(9, "Run")->setLoad(90);
        QCOMPARE(mesocycle.getSlottedLoad(), 3200);
        QCOMPARE(mesocycle.getSlottedLoad("Bike"), 3000);
        QCOMPARE(mesocycle.getSlottedLoad("Run"), 200);
        QCOMPARE(mesocycle.getSlottedLoad(0), 640);
        QCOMPARE(mesocycle.getSlottedLoad(0, "Bike"), 600);
        QCOMPARE(mesocycle.getSlottedLoad(0, "Run"), 40);
        QCOMPARE(mesocycle.getSlottedLoad(1), 2560);
        QCOMPARE(mesocycle.getSlottedLoad(1, "Bike"), 2400);
        QCOMPARE(mesocycle.getSlottedLoad(1, "Run"), 160);
    }

    void mesoSlottedIntensity() {
        Mesocycle mesocycle;
        mesocycle.setStart(ref1);
        mesocycle.setMaxMicrocycles(4);
        mesocycle.addSlot(1, "Bike")->setIntensity(0.1f);
        mesocycle.addSlot(1, "Bike")->setLoad(10);
        mesocycle.addSlot(1, "Run")-> setIntensity(1.1f);
        mesocycle.addSlot(1, "Run")->setLoad(10);
        mesocycle.addSlot(2, "Bike")->setIntensity(0.2f);
        mesocycle.addSlot(2, "Bike")->setLoad(20);
        mesocycle.addSlot(3, "Bike")->setIntensity(0.3f);
        mesocycle.addSlot(3, "Bike")->setLoad(30);
        mesocycle.addSlot(3, "Run")-> setIntensity(1.3f);
        mesocycle.addSlot(3, "Run")-> setLoad(30);
        mesocycle.addSlot(7, "Bike")->setIntensity(0.7f);
        mesocycle.addSlot(7, "Bike")->setLoad(70);
        mesocycle.addSlot(7, "Run")-> setIntensity(1.7f);
        mesocycle.addSlot(7, "Run")-> setLoad(70);
        mesocycle.addSlot(8, "Bike")->setIntensity(0.8f);
        mesocycle.addSlot(8, "Bike")->setLoad(80);
        mesocycle.addSlot(9, "Bike")->setIntensity(0.9f);
        mesocycle.addSlot(9, "Bike")->setLoad(90);
        mesocycle.addSlot(9, "Run")-> setIntensity(1.9f);
        mesocycle.addSlot(9, "Run")-> setLoad(90);
        QCOMPARE(mesocycle.getSlottedIntensity(), 548.0f / 500.0f);
        QCOMPARE(mesocycle.getSlottedIntensity("Bike"), 208.0f / 300.0f);
        QCOMPARE(mesocycle.getSlottedIntensity("Run"), 340.0f / 200.0f);
        QCOMPARE(mesocycle.getSlottedIntensity(0), 64.0f / 100.0f);
        QCOMPARE(mesocycle.getSlottedIntensity(0, "Bike"), 14.0f / 60.0f);
        QCOMPARE(mesocycle.getSlottedIntensity(0, "Run"), 50.0f / 40.0f);
        QCOMPARE(mesocycle.getSlottedIntensity(1), 484.0f / 400.0f);
        QCOMPARE(mesocycle.getSlottedIntensity(1, "Bike"), 194.0f / 240.0f);
        QCOMPARE(mesocycle.getSlottedIntensity(1, "Run"), 290.0f / 160.0f);
    }

    void mesoSlottedTime() {
        Mesocycle mesocycle;
        mesocycle.setMaxMicrocycles(4);
        mesocycle.addSlot(1, "Bike")->setTime(100);
        mesocycle.addSlot(1, "Run")->setTime(10);
        mesocycle.addSlot(2, "Bike")->setTime(200);
        mesocycle.addSlot(3, "Bike")->setTime(300);
        mesocycle.addSlot(3, "Run")->setTime(30);
        mesocycle.addSlot(7, "Bike")->setTime(700);
        mesocycle.addSlot(7, "Run")->setTime(70);
        mesocycle.addSlot(8, "Bike")->setTime(800);
        mesocycle.addSlot(9, "Bike")->setTime(900);
        mesocycle.addSlot(9, "Run")->setTime(90);
        QCOMPARE(mesocycle.getSlottedTime(), 3200);
        QCOMPARE(mesocycle.getSlottedTime("Bike"), 3000);
        QCOMPARE(mesocycle.getSlottedTime("Run"), 200);
        QCOMPARE(mesocycle.getSlottedTime(0), 640);
        QCOMPARE(mesocycle.getSlottedTime(0, "Bike"), 600);
        QCOMPARE(mesocycle.getSlottedTime(0, "Run"), 40);
        QCOMPARE(mesocycle.getSlottedTime(1), 2560);
        QCOMPARE(mesocycle.getSlottedTime(1, "Bike"), 2400);
        QCOMPARE(mesocycle.getSlottedTime(1, "Run"), 160);
    }

    void mesoAddHasSlotOffset() {
        Mesocycle mesocycle;
        mesocycle.setStart(ref1);
        mesocycle.setMaxMicrocycles(4);
        QVERIFY(mesocycle.addSlot(-1, "Bike") == nullptr);
        QVERIFY(mesocycle.addSlot(0, "") == nullptr);
        QVERIFY(mesocycle.addSlot(28, "Bike") == nullptr);
        QVERIFY(mesocycle.addSlot(0, "Bike") != nullptr);
        QVERIFY(mesocycle.addSlot(1, "Run") != nullptr);
        QVERIFY(mesocycle.addSlot(2, "Bike") != nullptr);
        QVERIFY(mesocycle.addSlot(3, "Bike") != nullptr);
        QVERIFY(mesocycle.addSlot(3, "Run") != nullptr);
        QVERIFY(mesocycle.addSlot(7, "Bike") != nullptr);
        QVERIFY(mesocycle.addSlot(7, "Run") != nullptr);
        QVERIFY(mesocycle.addSlot(8, "Bike") != nullptr);
        QVERIFY(mesocycle.addSlot(8, "Run") != nullptr);
        QVERIFY(mesocycle.addSlot(9, "Bike") != nullptr);
        QVERIFY(mesocycle.addSlot(9, "Run") != nullptr);
        QCOMPARE(mesocycle.hasSlot(-1, "Bike"), false);
        QCOMPARE(mesocycle.hasSlot(0, ""), false);
        QCOMPARE(mesocycle.hasSlot(28, "Bike"), false);
        QCOMPARE(mesocycle.hasSlot(0, "Bike"), true);
        QCOMPARE(mesocycle.hasSlot(1, "Run"), true);
        QCOMPARE(mesocycle.hasSlot(2, "Bike"), true);
        QCOMPARE(mesocycle.hasSlot(3, "Bike"), true);
        QCOMPARE(mesocycle.hasSlot(3, "Run"), true);
        QCOMPARE(mesocycle.hasSlot(7, "Bike"), true);
        QCOMPARE(mesocycle.hasSlot(7, "Run"), true);
        QCOMPARE(mesocycle.hasSlot(8, "Bike"), true);
        QCOMPARE(mesocycle.hasSlot(8, "Run"), true);
        QCOMPARE(mesocycle.hasSlot(9, "Bike"), true);
        QCOMPARE(mesocycle.hasSlot(9, "Run"), true);
        QCOMPARE(mesocycle.hasSlot(0, "Run"), false);
        QCOMPARE(mesocycle.hasSlot(1, "run"), false);
        QCOMPARE(mesocycle.hasSlot(4, "Bike"), false);
    }

    void mesoAddHasSlotDate() {
        Mesocycle mesocycle;
        mesocycle.setStart(ref1);
        mesocycle.setMaxMicrocycles(4);
        QVERIFY(mesocycle.addSlot(ref1.addDays(-1), "Bike") == nullptr);
        QVERIFY(mesocycle.addSlot(ref1.addDays(0), "") == nullptr);
        QVERIFY(mesocycle.addSlot(ref1.addDays(28), "Bike") == nullptr);
        QVERIFY(mesocycle.addSlot(ref1.addDays(0), "Bike") != nullptr);
        QVERIFY(mesocycle.addSlot(ref1.addDays(1), "Run") != nullptr);
        QVERIFY(mesocycle.addSlot(ref1.addDays(2), "Bike") != nullptr);
        QVERIFY(mesocycle.addSlot(ref1.addDays(3), "Bike") != nullptr);
        QVERIFY(mesocycle.addSlot(ref1.addDays(3), "Run") != nullptr);
        QVERIFY(mesocycle.addSlot(ref1.addDays(7), "Bike") != nullptr);
        QVERIFY(mesocycle.addSlot(ref1.addDays(7), "Run") != nullptr);
        QVERIFY(mesocycle.addSlot(ref1.addDays(8), "Bike") != nullptr);
        QVERIFY(mesocycle.addSlot(ref1.addDays(8), "Run") != nullptr);
        QVERIFY(mesocycle.addSlot(ref1.addDays(9), "Bike") != nullptr);
        QVERIFY(mesocycle.addSlot(ref1.addDays(9), "Run") != nullptr);
        QCOMPARE(mesocycle.hasSlot(ref1.addDays(-1), "Bike"), false);
        QCOMPARE(mesocycle.hasSlot(ref1.addDays(0), ""), false);
        QCOMPARE(mesocycle.hasSlot(ref1.addDays(28), "Bike"), false);
        QCOMPARE(mesocycle.hasSlot(ref1.addDays(0), "Bike"), true);
        QCOMPARE(mesocycle.hasSlot(ref1.addDays(1), "Run"), true);
        QCOMPARE(mesocycle.hasSlot(ref1.addDays(2), "Bike"), true);
        QCOMPARE(mesocycle.hasSlot(ref1.addDays(3), "Bike"), true);
        QCOMPARE(mesocycle.hasSlot(ref1.addDays(3), "Run"), true);
        QCOMPARE(mesocycle.hasSlot(ref1.addDays(7), "Bike"), true);
        QCOMPARE(mesocycle.hasSlot(ref1.addDays(7), "Run"), true);
        QCOMPARE(mesocycle.hasSlot(ref1.addDays(8), "Bike"), true);
        QCOMPARE(mesocycle.hasSlot(ref1.addDays(8), "Run"), true);
        QCOMPARE(mesocycle.hasSlot(ref1.addDays(9), "Bike"), true);
        QCOMPARE(mesocycle.hasSlot(ref1.addDays(9), "Run"), true);
        QCOMPARE(mesocycle.hasSlot(ref1.addDays(0), "Run"), false);
        QCOMPARE(mesocycle.hasSlot(ref1.addDays(1), "run"), false);
        QCOMPARE(mesocycle.hasSlot(ref1.addDays(4), "Bike"), false);
    }

    void mesoGetSlotOffset() {
        Mesocycle mesocycle;
        mesocycle.setStart(ref1);
        mesocycle.setMaxMicrocycles(1);
        mesocycle.addSlot(0, "Run")->setLoad(10);
        mesocycle.addSlot(2, "Bike")->setLoad(200);
        mesocycle.addSlot(6, "Bike")->setLoad(600);
        mesocycle.addSlot(6, "Run")->setLoad(60);
        QVERIFY(mesocycle.getSlot(-1, "Bike") == nullptr);
        QVERIFY(mesocycle.getSlot(0, "Bike") == nullptr);
        QVERIFY(mesocycle.getSlot(7, "Bike") == nullptr);
        QVERIFY(mesocycle.getSlot(0, "Run") != nullptr);
        QVERIFY(mesocycle.getSlot(2, "Bike") != nullptr);
        QVERIFY(mesocycle.getSlot(6, "Bike") != nullptr);
        QVERIFY(mesocycle.getSlot(6, "Run") != nullptr);
        QCOMPARE(mesocycle.getSlot(6, "Bike")->getDayOffset(), 6);
        QCOMPARE(mesocycle.getSlot(6, "Bike")->getSport(), "Bike");
        QCOMPARE(mesocycle.getSlot(6, "Bike")->getLoad(), 600);
        QCOMPARE(mesocycle.getSlot(6, "Bike")->getStatus(), Slot::StatusType::generated);
        QCOMPARE(mesocycle.getSlot(6, "Run")->getDayOffset(), 6);
        QCOMPARE(mesocycle.getSlot(6, "Run")->getSport(), "Run");
        QCOMPARE(mesocycle.getSlot(6, "Run")->getLoad(), 60);
        QCOMPARE(mesocycle.getSlot(6, "Run")->getStatus(), Slot::StatusType::generated);
    }

    void mesoGetSlotDate() {
        Mesocycle mesocycle;
        mesocycle.setStart(ref1);
        mesocycle.setMaxMicrocycles(1);
        mesocycle.addSlot(ref1.addDays(0), "Run")->setLoad(10);
        mesocycle.addSlot(ref1.addDays(2), "Bike")->setLoad(200);
        mesocycle.addSlot(ref1.addDays(6), "Bike")->setLoad(600);
        mesocycle.addSlot(ref1.addDays(6), "Run")->setLoad(60);
        QVERIFY(mesocycle.getSlot(ref1.addDays(-1), "Bike") == nullptr);
        QVERIFY(mesocycle.getSlot(ref1.addDays(0), "Bike") == nullptr);
        QVERIFY(mesocycle.getSlot(ref1.addDays(7), "Bike") == nullptr);
        QVERIFY(mesocycle.getSlot(ref1.addDays(0), "Run") != nullptr);
        QVERIFY(mesocycle.getSlot(ref1.addDays(2), "Bike") != nullptr);
        QVERIFY(mesocycle.getSlot(ref1.addDays(6), "Bike") != nullptr);
        QVERIFY(mesocycle.getSlot(ref1.addDays(6), "Run") != nullptr);
        QCOMPARE(mesocycle.getSlot(ref1.addDays(6), "Bike")->getDayOffset(), 6);
        QCOMPARE(mesocycle.getSlot(ref1.addDays(6), "Bike")->getSport(), "Bike");
        QCOMPARE(mesocycle.getSlot(ref1.addDays(6), "Bike")->getLoad(), 600);
        QCOMPARE(mesocycle.getSlot(ref1.addDays(6), "Bike")->getStatus(), Slot::StatusType::generated);
        QCOMPARE(mesocycle.getSlot(ref1.addDays(6), "Run")->getDayOffset(), 6);
        QCOMPARE(mesocycle.getSlot(ref1.addDays(6), "Run")->getSport(), "Run");
        QCOMPARE(mesocycle.getSlot(ref1.addDays(6), "Run")->getLoad(), 60);
        QCOMPARE(mesocycle.getSlot(ref1.addDays(6), "Run")->getStatus(), Slot::StatusType::generated);
    }

    void mesoGetSlotSports() {
        Mesocycle mesocycle;
        mesocycle.setStart(ref1);
        mesocycle.setMaxMicrocycles(1);
        mesocycle.addSlot(ref1.addDays(0), "Run");
        mesocycle.addSlot(ref1.addDays(1), "Bike");
        mesocycle.addSlot(ref1.addDays(2), "Bike");
        mesocycle.addSlot(ref1.addDays(6), "Bike");
        mesocycle.addSlot(ref1.addDays(6), "Run");
        QMap<QString, QList<int>> offsets = mesocycle.getSlotSportOffsets();
        QCOMPARE(offsets.count(), 2);
        QVERIFY(offsets.contains("Bike"));
        QVERIFY(offsets.contains("Run"));
        QCOMPARE(offsets["Bike"].count(), 3);
        QCOMPARE(offsets["Bike"][0], 1);
        QCOMPARE(offsets["Bike"][1], 2);
        QCOMPARE(offsets["Bike"][2], 6);
        QCOMPARE(offsets["Run"].count(), 2);
        QCOMPARE(offsets["Run"][0], 0);
        QCOMPARE(offsets["Run"][1], 6);
        QMap<QString, QList<QDate>> dates = mesocycle.getSlotSportDates();
        QCOMPARE(dates.count(), 2);
        QVERIFY(dates.contains("Bike"));
        QVERIFY(dates.contains("Run"));
        QCOMPARE(dates["Bike"].count(), 3);
        QCOMPARE(dates["Bike"][0], ref1.addDays(1));
        QCOMPARE(dates["Bike"][1], ref1.addDays(2));
        QCOMPARE(dates["Bike"][2], ref1.addDays(6));
        QCOMPARE(dates["Run"].count(), 2);
        QCOMPARE(dates["Run"][0], ref1.addDays(0));
        QCOMPARE(dates["Run"][1], ref1.addDays(6));
    }

    void mesoGetSlotOffsets() {
        Mesocycle mesocycle;
        mesocycle.setStart(ref1);
        mesocycle.setMaxMicrocycles(1);
        mesocycle.addSlot(ref1.addDays(0), "Run");
        mesocycle.addSlot(ref1.addDays(1), "Bike");
        mesocycle.addSlot(ref1.addDays(2), "Bike");
        mesocycle.addSlot(ref1.addDays(6), "Bike");
        mesocycle.addSlot(ref1.addDays(6), "Run");
        QMap<int, QList<QString>> offsets = mesocycle.getSlotOffsetSports();
        QCOMPARE(offsets.count(), 4);
        QVERIFY(offsets.contains(0));
        QVERIFY(offsets.contains(1));
        QVERIFY(offsets.contains(2));
        QVERIFY(offsets.contains(6));
        QCOMPARE(offsets[0].count(), 1);
        QCOMPARE(offsets[0][0], "Run");
        QCOMPARE(offsets[1].count(), 1);
        QCOMPARE(offsets[1][0], "Bike");
        QCOMPARE(offsets[2].count(), 1);
        QCOMPARE(offsets[2][0], "Bike");
        QCOMPARE(offsets[6].count(), 2);
        QCOMPARE(offsets[6][0], "Bike");
        QCOMPARE(offsets[6][1], "Run");
        QMap<QDate, QList<QString>> dates = mesocycle.getSlotDateSports();
        QCOMPARE(dates.count(), 4);
        QVERIFY(dates.contains(ref1.addDays(0)));
        QVERIFY(dates.contains(ref1.addDays(1)));
        QVERIFY(dates.contains(ref1.addDays(2)));
        QVERIFY(dates.contains(ref1.addDays(6)));
        QCOMPARE(dates[ref1.addDays(0)].count(), 1);
        QCOMPARE(dates[ref1.addDays(0)][0], "Run");
        QCOMPARE(dates[ref1.addDays(1)].count(), 1);
        QCOMPARE(dates[ref1.addDays(1)][0], "Bike");
        QCOMPARE(dates[ref1.addDays(2)].count(), 1);
        QCOMPARE(dates[ref1.addDays(2)][0], "Bike");
        QCOMPARE(dates[ref1.addDays(6)].count(), 2);
        QCOMPARE(dates[ref1.addDays(6)][0], "Bike");
        QCOMPARE(dates[ref1.addDays(6)][1], "Run");
    }


    //////////////////////////////////////////////////////////////////////////////
    // Mesocycle in Phase

    void phaseNormalHasMesocycle() {
        Phase phase("NAME", ref1, ref2);
        phase.setType(Phase::phase);
        QCOMPARE(phase.hasMesocycle(), false);
        QCOMPARE(phase.getMesocycle(), nullptr);
    }

    void phaseMesoCreation() {
        Phase phase("NAME", ref1, ref2);
        QCOMPARE(phase.hasMesocycle(), false);
        QCOMPARE(phase.numMicrocycles(), 4);
        phase.setType(Phase::mesocycle);
        QCOMPARE(phase.hasMesocycle(), true);
        QCOMPARE(phase.getMesocycle()->getMaxMicrocycles(), 4);
        QCOMPARE(phase.getMesocycle()->toMicrocycle(ref1), 0);
    }

    void phaseChangeFromMesocycle() {
        Phase phase("NAME", ref1, ref2);
        phase.setType(Phase::mesocycle);
        QCOMPARE(phase.hasMesocycle(), true);
        QCOMPARE(phase.getType(), Phase::mesocycle);
        phase.setType(Phase::phase);
        QCOMPARE(phase.hasMesocycle(), true);
        QCOMPARE(phase.getType(), Phase::mesocycle);
    }

    void phaseMesoSetAbsoluteStart() {
        Phase phase("NAME", ref1, ref2);
        QCOMPARE(phase.numMicrocycles(), 4);
        phase.setType(Phase::mesocycle);
        phase.setAbsoluteStart(ref1.addDays(7));
        QCOMPARE(phase.hasMesocycle(), true);
        QCOMPARE(phase.getMesocycle()->getMaxMicrocycles(), 3);
        QCOMPARE(phase.getMesocycle()->toMicrocycle(ref1.addDays(7)), 0);
    }

    void phaseMesoSetAbsoluteStartTruncatesSlots() {
        Phase phase("NAME", ref1, ref2);
        phase.setType(Phase::mesocycle);
        phase.getMesocycle()->addSlot(ref1.addDays(3), "Bike");
        phase.getMesocycle()->addSlot(ref2.addDays(-3), "Bike");
        QCOMPARE(phase.getMesocycle()->hasSlot(ref1.addDays(3), "Bike"), true);
        QCOMPARE(phase.getMesocycle()->hasSlot(ref2.addDays(-3), "Bike"), true);
        phase.setAbsoluteStart(ref1.addDays(7));
        QCOMPARE(phase.getMesocycle()->hasSlot(ref1.addDays(3), "Bike"), false);
        QCOMPARE(phase.getMesocycle()->hasSlot(ref1.addDays(10), "Bike"), true);
        QCOMPARE(phase.getMesocycle()->hasSlot(ref2.addDays(-3), "Bike"), false);
    }

    void phaseMesoSetAbsoluteEndTruncatesSlots() {
        Phase phase("NAME", ref1, ref2);
        phase.setType(Phase::mesocycle);
        phase.getMesocycle()->addSlot(ref1.addDays(3), "Bike");
        phase.getMesocycle()->addSlot(ref2.addDays(-9), "Bike");
        QCOMPARE(phase.getMesocycle()->hasSlot(ref1.addDays(3), "Bike"), true);
        QCOMPARE(phase.getMesocycle()->hasSlot(ref2.addDays(-9), "Bike"), true);
        phase.setAbsoluteEnd(ref2.addDays(-7));
        QCOMPARE(phase.getMesocycle()->hasSlot(ref1.addDays(3), "Bike"), true);
        QCOMPARE(phase.getMesocycle()->hasSlot(ref2.addDays(-9), "Bike"), true);
        phase.setAbsoluteEnd(ref2.addDays(-14));
        QCOMPARE(phase.getMesocycle()->hasSlot(ref1.addDays(3), "Bike"), true);
        QCOMPARE(phase.getMesocycle()->hasSlot(ref2.addDays(-9), "Bike"), false);
    }

    void phaseMesoSetAbsoluteEnd() {
        Phase phase("NAME", ref1, ref2);
        QCOMPARE(phase.numMicrocycles(), 4);
        phase.setType(Phase::mesocycle);
        phase.setAbsoluteEnd(ref2.addDays(7));
        QCOMPARE(phase.hasMesocycle(), true);
        QCOMPARE(phase.getMesocycle()->getMaxMicrocycles(), 5);
        QCOMPARE(phase.getMesocycle()->toMicrocycle(ref1.addDays(7)), 1);
        QCOMPARE(phase.getMesocycle()->toMicrocycle(ref2), 3);
        QCOMPARE(phase.getMesocycle()->toMicrocycle(ref2.addDays(7)), 4);
    }

private:
    const QDate ref1;
    const QDate ref2;
};


QTEST_MAIN(TestPhaseMesocycle)
#include "testPhaseMesocycle.moc"
