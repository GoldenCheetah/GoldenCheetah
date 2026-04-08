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

    // ---------------------------------------------------------------
    // HRV integration
    // ---------------------------------------------------------------

    void hrv_noData_noEffect()
    {
        // Without HRV data, ranking should be identical to baseline
        WorkoutAthleteSnapshot s = makeSnapshot();
        s.hrvAvailable = false;
        SimulationRanking ranking = TrainingSimulator::rankCandidates(
            s, TrainingGoal::Build, 60, 7);

        // VO2max should rank highest for build goal (most CTL gain)
        QVERIFY(!ranking.candidates.isEmpty());
        QCOMPARE(ranking.candidates.first().workoutType, QStringLiteral("vo2max"));
    }

    void hrv_normalRatio_noEffect()
    {
        // HRV ratio 1.05 (above baseline) should not change rankings
        WorkoutAthleteSnapshot s = makeSnapshot();
        s.hrvAvailable = true;
        s.hrvRMSSD = 52.5;
        s.hrvBaseline = 50.0;
        s.hrvRatio = 1.05;

        SimulationRanking ranking = TrainingSimulator::rankCandidates(
            s, TrainingGoal::Build, 60, 7);

        QVERIFY(!ranking.candidates.isEmpty());
        // All should be feasible with normal HRV
        for (const SimulationResult &r : ranking.candidates)
            QVERIFY(r.feasible);
    }

    void hrv_severeSuppression_blocksHighIntensity()
    {
        // HRV ratio 0.75 (severe suppression) should make high-intensity infeasible
        WorkoutAthleteSnapshot s = makeSnapshot();
        s.hrvAvailable = true;
        s.hrvRMSSD = 37.5;
        s.hrvBaseline = 50.0;
        s.hrvRatio = 0.75;

        SimulationRanking ranking = TrainingSimulator::rankCandidates(
            s, TrainingGoal::Build, 60, 7);

        // Find the vo2max and threshold candidates — they should be infeasible
        for (const SimulationResult &r : ranking.candidates) {
            if (r.workoutType == QStringLiteral("vo2max") ||
                r.workoutType == QStringLiteral("threshold") ||
                r.workoutType == QStringLiteral("sweetspot") ||
                r.workoutType == QStringLiteral("anaerobic")) {
                QVERIFY2(!r.feasible,
                         qPrintable(QStringLiteral("%1 should be infeasible with severe HRV suppression")
                                    .arg(r.workoutType)));
            }
        }

        // Recovery and endurance should remain feasible
        for (const SimulationResult &r : ranking.candidates) {
            if (r.workoutType == QStringLiteral("recovery") ||
                r.workoutType == QStringLiteral("endurance")) {
                QVERIFY2(r.feasible,
                         qPrintable(QStringLiteral("%1 should remain feasible with HRV suppression")
                                    .arg(r.workoutType)));
            }
        }
    }

    void hrv_mildSuppression_penalizesScore()
    {
        // HRV ratio 0.90 (mild suppression) should reduce high-intensity scores
        WorkoutAthleteSnapshot sNormal = makeSnapshot();
        sNormal.hrvAvailable = false;

        WorkoutAthleteSnapshot sSuppressed = makeSnapshot();
        sSuppressed.hrvAvailable = true;
        sSuppressed.hrvRMSSD = 45.0;
        sSuppressed.hrvBaseline = 50.0;
        sSuppressed.hrvRatio = 0.90;

        SimulationResult normalVO2 = TrainingSimulator::simulateCandidate(
            sNormal, QStringLiteral("vo2max"), 60, 7);
        double normalScore = TrainingSimulator::scoreForGoal(normalVO2, sNormal, TrainingGoal::Build);

        SimulationResult suppressedVO2 = TrainingSimulator::simulateCandidate(
            sSuppressed, QStringLiteral("vo2max"), 60, 7);
        double suppressedScore = TrainingSimulator::scoreForGoal(suppressedVO2, sSuppressed, TrainingGoal::Build);

        // Suppressed HRV should produce a lower (more penalized) score for VO2max
        QVERIFY2(suppressedScore < normalScore,
                 qPrintable(QStringLiteral("Suppressed score %1 should be < normal score %2")
                            .arg(suppressedScore).arg(normalScore)));
    }

    void hrv_suppressionHasConstraintViolation()
    {
        WorkoutAthleteSnapshot s = makeSnapshot();
        s.hrvAvailable = true;
        s.hrvRMSSD = 40.0;
        s.hrvBaseline = 50.0;
        s.hrvRatio = 0.80;

        SimulationResult result = TrainingSimulator::simulateCandidate(
            s, QStringLiteral("vo2max"), 60, 7);

        bool foundHrv = false;
        for (const ConstraintViolation &v : result.constraints.violations) {
            if (v.constraintId == QStringLiteral("hrvSuppressed")) {
                foundHrv = true;
                QCOMPARE(v.severity, ConstraintViolation::Hard);
            }
        }
        QVERIFY2(foundHrv, "Expected hrvSuppressed constraint violation");
    }
};

QTEST_MAIN(TestTrainingSimulator)
#include "testTrainingSimulator.moc"
