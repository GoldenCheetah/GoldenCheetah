#include "Core/Seasons.h"
#include "Core/Season.h"

#include <QTest>
#include <QFile>
#include <QList>
#include <QSignalSpy>


class TestSeasons: public QObject
{
    Q_OBJECT

private slots:
    void editMacrocycle() {
        const QString seasonId = "{fa549d73-7d8b-4df4-a16e-0ce50f92d131}";
        const QString phase0Id = "{a4d28e7c-bf42-4a04-8928-c18671c698bc}";
        const QString phase1Id = "{96489889-ba6e-4006-a7ea-2f9bb7be0620}";
        const QString phaseNonMesoId = "{a7147da0-de38-41de-9bee-e50562f1dd5f}";
        Seasons seasons(QDir("."));
        Season const *season = seasons.getSeason(seasonId);
        QVERIFY(season != nullptr);
        QCOMPARE(season->phases[0].id().toString(), phase0Id);
        QCOMPARE(season->phases[1].id().toString(), phase1Id);
        QCOMPARE(season->phases[2].id().toString(), phaseNonMesoId);

        QSignalSpy macroSpy(&seasons, &Seasons::macroPlanChanged);
        QVERIFY(macroSpy.isValid());

        QCOMPARE(seasons.editMacroPlan(seasonId, phaseNonMesoId, [](Mesocycle &m) {}), false);
        QCOMPARE(macroSpy.count(), 0);

        QCOMPARE(seasons.editMacroPlan(seasonId, phase0Id, [](Mesocycle &m) {}), true);
        QCOMPARE(macroSpy.count(), 1);
        QCOMPARE(macroSpy.at(0).at(0), seasonId);

        int idx = 0;
        macroSpy.clear();
        QCOMPARE(season->phases[0].getMesocycle()->getMicrocycle(idx)->getLoad(), 140);
        QCOMPARE(seasons.editMacroPlan(seasonId, phase0Id, [idx](Mesocycle &m) {
            QCOMPARE(m.hasMicrocycle(idx), true);
            m.getMicrocycle(idx)->setLoad(100);
        }), true);
        QCOMPARE(macroSpy.count(), 1);
        QCOMPARE(macroSpy.at(0).at(0), seasonId);
        QCOMPARE(season->phases[0].getMesocycle()->getMicrocycle(idx)->getLoad(), 100);
    }
};


QTEST_MAIN(TestSeasons)
#include "testSeasons.moc"
