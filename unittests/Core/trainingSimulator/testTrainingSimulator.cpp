/*
 * Unit tests for TrainingSimulator.
 *
 * Tests TSS estimation, forward projection, goal scoring,
 * and candidate ranking — the core of Phase 1's simulation-based
 * workout selection.
 */

#include "Train/TrainingSimulator.h"

#include <QTest>

class TestTrainingSimulator : public QObject
{
    Q_OBJECT

private:

    WorkoutAthleteSnapshot makeSnapshot(double ctl = 60.0, double atl = 70.0)
    {
        WorkoutAthleteSnapshot s;
        s.athleteName = QStringLiteral("TestAthlete");
        s.date = QDate(2025, 6, 15);
        s.ftp = 250;
        s.cp = 245;
        s.ctl = ctl;
        s.atl = atl;
        s.tsb = ctl - atl;
        s.tsbTrend = 2.0;
        s.ctlRampRate = 3.0;
        s.daysToEvent = 28;
        s.nextEventName = QStringLiteral("Gran Fondo");
        s.recentStress = {50, 80, 60, 90, 0, 70, 60};
        return s;
    }

private slots:

    // ---------------------------------------------------------------
    // TSS estimation
    // ---------------------------------------------------------------

    void estimateTSS_recovery_lowTSS()
    {
        WorkoutAthleteSnapshot s = makeSnapshot();
        double tss = TrainingSimulator::estimateTSS(s, QStringLiteral("recovery"), 60);
        // Recovery: IF≈0.55, 1hr => ~30 TSS
        QVERIFY(tss > 20.0);
        QVERIFY(tss < 50.0);
    }

    void estimateTSS_threshold_highTSS()
    {
        WorkoutAthleteSnapshot s = makeSnapshot();
        double tss = TrainingSimulator::estimateTSS(s, QStringLiteral("threshold"), 60);
        // Threshold: IF≈1.0, 1hr => ~100 TSS
        QVERIFY(tss > 80.0);
        QVERIFY(tss < 120.0);
    }

    void estimateTSS_scalesWithDuration()
    {
        WorkoutAthleteSnapshot s = makeSnapshot();
        double tss60 = TrainingSimulator::estimateTSS(s, QStringLiteral("endurance"), 60);
        double tss120 = TrainingSimulator::estimateTSS(s, QStringLiteral("endurance"), 120);
        QVERIFY(std::abs(tss120 - 2.0 * tss60) < 1.0);
    }

    // ---------------------------------------------------------------
    // Workout types
    // ---------------------------------------------------------------

    void workoutTypes_containsStandardTypes()
    {
        QStringList types = TrainingSimulator::workoutTypes();
        QVERIFY(types.contains(QStringLiteral("recovery")));
        QVERIFY(types.contains(QStringLiteral("endurance")));
        QVERIFY(types.contains(QStringLiteral("sweetspot")));
        QVERIFY(types.contains(QStringLiteral("threshold")));
        QVERIFY(types.contains(QStringLiteral("vo2max")));
        QVERIFY(types.contains(QStringLiteral("anaerobic")));
        QVERIFY(types.size() >= 7);
    }

    // ---------------------------------------------------------------
    // Single candidate simulation
    // ---------------------------------------------------------------

    void simulateCandidate_producesProjection()
    {
        WorkoutAthleteSnapshot s = makeSnapshot();
        SimulationResult r = TrainingSimulator::simulateCandidate(
            s, QStringLiteral("endurance"), 60, 7);

        QVERIFY(!r.projCtl.isEmpty());
        QCOMPARE(r.projCtl.size(), 8); // 1 workout day + 7 projection days
        QVERIFY(r.estimatedTSS > 0.0);
    }

    void simulateCandidate_recovery_staysFeasible()
    {
        WorkoutAthleteSnapshot s = makeSnapshot(60.0, 70.0);
        SimulationResult r = TrainingSimulator::simulateCandidate(
            s, QStringLiteral("recovery"), 45, 7);

        QVERIFY(r.feasible);
    }

    // ---------------------------------------------------------------
    // Goal scoring
    // ---------------------------------------------------------------

    void scoring_buildFavorsHigherCTL()
    {
        WorkoutAthleteSnapshot s = makeSnapshot();
        SimulationResult threshold = TrainingSimulator::simulateCandidate(
            s, QStringLiteral("threshold"), 60, 7);
        SimulationResult recovery = TrainingSimulator::simulateCandidate(
            s, QStringLiteral("recovery"), 60, 7);

        double sThreshold = TrainingSimulator::scoreForGoal(threshold, s, TrainingGoal::Build);
        double sRecovery = TrainingSimulator::scoreForGoal(recovery, s, TrainingGoal::Build);

        QVERIFY(sThreshold > sRecovery);
    }

    void scoring_recoverFavorsLowStress()
    {
        WorkoutAthleteSnapshot s = makeSnapshot();
        SimulationResult threshold = TrainingSimulator::simulateCandidate(
            s, QStringLiteral("threshold"), 60, 7);
        SimulationResult recovery = TrainingSimulator::simulateCandidate(
            s, QStringLiteral("recovery"), 45, 7);

        double sThreshold = TrainingSimulator::scoreForGoal(threshold, s, TrainingGoal::Recover);
        double sRecovery = TrainingSimulator::scoreForGoal(recovery, s, TrainingGoal::Recover);

        QVERIFY(sRecovery > sThreshold);
    }

    // ---------------------------------------------------------------
    // Candidate ranking
    // ---------------------------------------------------------------

    void rankCandidates_returnsAllTypes()
    {
        WorkoutAthleteSnapshot s = makeSnapshot();
        SimulationRanking ranking = TrainingSimulator::rankCandidates(
            s, TrainingGoal::Build, 60, 7);

        QCOMPARE(ranking.candidates.size(), TrainingSimulator::workoutTypes().size());
        QCOMPARE(ranking.goal, TrainingGoal::Build);
    }

    void rankCandidates_buildRanksHighLoadFirst()
    {
        WorkoutAthleteSnapshot s = makeSnapshot();
        SimulationRanking ranking = TrainingSimulator::rankCandidates(
            s, TrainingGoal::Build, 60, 7);

        // First feasible candidate should not be recovery
        for (const SimulationResult &r : ranking.candidates) {
            if (r.feasible) {
                QVERIFY(r.workoutType != QStringLiteral("recovery"));
                break;
            }
        }
    }

    void rankCandidates_recoverRanksLowLoadFirst()
    {
        WorkoutAthleteSnapshot s = makeSnapshot();
        SimulationRanking ranking = TrainingSimulator::rankCandidates(
            s, TrainingGoal::Recover, 60, 7);

        // First candidate should be recovery or low-load
        QVERIFY(ranking.candidates.first().estimatedTSS <
                ranking.candidates.last().estimatedTSS);
    }

    // ---------------------------------------------------------------
    // JSON serialization
    // ---------------------------------------------------------------

    void simulationResult_toJson()
    {
        WorkoutAthleteSnapshot s = makeSnapshot();
        SimulationResult r = TrainingSimulator::simulateCandidate(
            s, QStringLiteral("sweetspot"), 60, 7);
        r.score = 3.5;

        QJsonObject json = r.toJson();
        QCOMPARE(json[QStringLiteral("workoutType")].toString(), QStringLiteral("sweetspot"));
        QVERIFY(json.contains(QStringLiteral("projection")));
        QVERIFY(json[QStringLiteral("estimatedTSS")].toDouble() > 0.0);
    }

    void ranking_toJson()
    {
        WorkoutAthleteSnapshot s = makeSnapshot();
        SimulationRanking ranking = TrainingSimulator::rankCandidates(
            s, TrainingGoal::Taper, 60, 7);

        QJsonObject json = ranking.toJson();
        QCOMPARE(json[QStringLiteral("goal")].toString(), QStringLiteral("taper"));
        QVERIFY(json[QStringLiteral("candidates")].toArray().size() > 0);
    }

    // ---------------------------------------------------------------
    // Goal string conversion
    // ---------------------------------------------------------------

    void goalRoundtrip()
    {
        QCOMPARE(trainingGoalFromString(QStringLiteral("build")), TrainingGoal::Build);
        QCOMPARE(trainingGoalFromString(QStringLiteral("Maintain")), TrainingGoal::Maintain);
        QCOMPARE(trainingGoalFromString(QStringLiteral("TAPER")), TrainingGoal::Taper);
        QCOMPARE(trainingGoalFromString(QStringLiteral("recover")), TrainingGoal::Recover);
        QCOMPARE(trainingGoalFromString(QStringLiteral("unknown")), TrainingGoal::Build); // default

        QCOMPARE(trainingGoalToString(TrainingGoal::Build), QStringLiteral("build"));
        QCOMPARE(trainingGoalToString(TrainingGoal::Taper), QStringLiteral("taper"));
    }
};

QTEST_MAIN(TestTrainingSimulator)
#include "testTrainingSimulator.moc"
