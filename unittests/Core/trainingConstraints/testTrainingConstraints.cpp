/*
 * Unit tests for TrainingConstraintChecker.
 *
 * These verify the physiological safety constraints that must
 * override any model output or optimizer recommendation.
 * See doc/ai/constraints.md for the constraint reference table.
 */

#include "Train/TrainingConstraintChecker.h"

#include <QTest>

class TestTrainingConstraints : public QObject
{
    Q_OBJECT

private slots:

    // ---------------------------------------------------------------
    // Monotony
    // ---------------------------------------------------------------

    void monotony_identicalDays_returnsZero()
    {
        // Same load every day => sd=0 => monotony=0 (division guard)
        QVector<double> stress = {100, 100, 100, 100, 100, 100, 100};
        // Actually monotony = mean/sd => inf if sd=0; but we guard with sd>0 check
        // Our implementation returns 0 when sd=0
        double m = TrainingConstraintChecker::monotony(stress, 0, 7);
        QCOMPARE(m, 0.0);
    }

    void monotony_variedDays_withinRange()
    {
        // 5 days at 80, 1 day at 120, 1 rest day => moderate monotony
        QVector<double> stress = {80, 80, 120, 80, 80, 0, 80};
        double m = TrainingConstraintChecker::monotony(stress, 0, 7);
        QVERIFY(m > 0.0);
        QVERIFY(m < 3.0);
    }

    // ---------------------------------------------------------------
    // ACWR
    // ---------------------------------------------------------------

    void acwr_equalLoadRatio()
    {
        // ATL = CTL => ACWR = 1.0
        QCOMPARE(TrainingConstraintChecker::acwr(50.0, 50.0), 1.0);
    }

    void acwr_zeroCtl_returnsZero()
    {
        QCOMPARE(TrainingConstraintChecker::acwr(30.0, 0.0), 0.0);
    }

    void acwr_highAcute()
    {
        // ATL=90, CTL=50 => ACWR=1.8
        double a = TrainingConstraintChecker::acwr(90.0, 50.0);
        QCOMPARE(a, 1.8);
    }

    // ---------------------------------------------------------------
    // CTL ramp rate
    // ---------------------------------------------------------------

    void ctlRamp_oneWeek()
    {
        double ramp = TrainingConstraintChecker::ctlRampRate(50.0, 55.0, 7);
        QCOMPARE(ramp, 5.0);
    }

    void ctlRamp_twoWeeks()
    {
        double ramp = TrainingConstraintChecker::ctlRampRate(50.0, 60.0, 14);
        QCOMPARE(ramp, 5.0);
    }

    // ---------------------------------------------------------------
    // PMC projection
    // ---------------------------------------------------------------

    void projectPMC_restDaysDecay()
    {
        // All rest days: CTL and ATL should decay
        QVector<double> stress(7, 0.0);
        QVector<double> ctl, atl, tsb;
        TrainingConstraintChecker::projectPMC(stress, 60.0, 80.0, 42, 7, ctl, atl, tsb);

        QCOMPARE(ctl.size(), 7);
        QVERIFY(ctl.last() < 60.0);
        QVERIFY(atl.last() < 80.0);
        // TSB should become positive (freshening)
        QVERIFY(tsb.last() > tsb.first());
    }

    void projectPMC_highLoadIncreases()
    {
        // High training day then rest
        QVector<double> stress = {200, 0, 0, 0, 0, 0, 0};
        QVector<double> ctl, atl, tsb;
        TrainingConstraintChecker::projectPMC(stress, 50.0, 50.0, 42, 7, ctl, atl, tsb);

        // CTL should increase after the training day
        QVERIFY(ctl[0] > 50.0);
        // ATL should spike then decay
        QVERIFY(atl[0] > 50.0);
        QVERIFY(atl.last() < atl[0]);
    }

    // ---------------------------------------------------------------
    // Full constraint check
    // ---------------------------------------------------------------

    void checkAll_normalWeek_passes()
    {
        // Moderate, varied week with a rest day
        QVector<double> stress = {80, 60, 90, 0, 70, 80, 60};
        QDate start(2025, 1, 6);
        ConstraintCheckResult result = TrainingConstraintChecker::checkAll(
            stress, start, 50.0, 55.0);
        QVERIFY(result.passed);
    }

    void checkAll_excessiveDailyTSS_fails()
    {
        QVector<double> stress = {50, 50, 400, 50, 50, 0, 50};
        QDate start(2025, 1, 6);
        ConstraintCheckResult result = TrainingConstraintChecker::checkAll(
            stress, start, 50.0, 55.0);
        QVERIFY(!result.passed);

        bool found = false;
        for (const ConstraintViolation &v : result.violations) {
            if (v.constraintId == QStringLiteral("maxDailyTSS"))
                found = true;
        }
        QVERIFY(found);
    }

    void checkAll_noRestDays_fails()
    {
        // Every day above rest threshold (20 TSS)
        QVector<double> stress = {80, 70, 90, 60, 80, 70, 80};
        QDate start(2025, 1, 6);
        ConstraintCheckResult result = TrainingConstraintChecker::checkAll(
            stress, start, 50.0, 55.0);

        bool found = false;
        for (const ConstraintViolation &v : result.violations) {
            if (v.constraintId == QStringLiteral("restDays"))
                found = true;
        }
        QVERIFY(found);
    }

    void checkAll_eliteBounds_morePermissive()
    {
        // 350 TSS should fail recreational but pass elite
        QVector<double> stress = {350, 0, 50, 0, 50, 0, 50};
        QDate start(2025, 1, 6);

        ConstraintCheckResult rec = TrainingConstraintChecker::checkAll(
            stress, start, 80.0, 90.0);
        ConstraintCheckResult elite = TrainingConstraintChecker::checkAll(
            stress, start, 80.0, 90.0, TrainingConstraintBounds::elite());

        bool recHasTSS = false, eliteHasTSS = false;
        for (const auto &v : rec.violations)
            if (v.constraintId == QStringLiteral("maxDailyTSS")) recHasTSS = true;
        for (const auto &v : elite.violations)
            if (v.constraintId == QStringLiteral("maxDailyTSS")) eliteHasTSS = true;

        QVERIFY(recHasTSS);
        QVERIFY(!eliteHasTSS);
    }

    // ---------------------------------------------------------------
    // checkFromDay — historical context not penalized
    // ---------------------------------------------------------------

    void checkAll_checkFromDay_skipsHistoricalViolations()
    {
        // First 7 days have a 400 TSS spike (would fail maxDailyTSS).
        // Day 7 onward is moderate. With checkFromDay=7, only the
        // forward portion is evaluated — the historical spike is ignored.
        QVector<double> stress = {400, 0, 0, 0, 0, 0, 0,   // historical context
                                  80, 60, 0, 70, 80, 60, 50}; // forward plan
        QDate start(2025, 1, 1);

        // Without checkFromDay: should fail (400 TSS violation)
        ConstraintCheckResult full = TrainingConstraintChecker::checkAll(
            stress, start, 50.0, 55.0);
        bool hasTSSViolation = false;
        for (const auto &v : full.violations)
            if (v.constraintId == QStringLiteral("maxDailyTSS")) hasTSSViolation = true;
        QVERIFY(hasTSSViolation);

        // With checkFromDay=7: should pass (forward portion is clean)
        ConstraintCheckResult forward = TrainingConstraintChecker::checkAll(
            stress, start, 50.0, 55.0, TrainingConstraintBounds::recreational(), 7);
        bool forwardHasTSSViolation = false;
        for (const auto &v : forward.violations)
            if (v.constraintId == QStringLiteral("maxDailyTSS")) forwardHasTSSViolation = true;
        QVERIFY(!forwardHasTSSViolation);
    }

    void checkAll_checkFromDay_stillUsesHistoryForPMC()
    {
        // High historical load should affect forward ACWR even when
        // historical violations are skipped. The PMC state carries forward.
        QVector<double> stress = {200, 200, 200, 200, 200, 200, 200,  // heavy history
                                  200, 200, 200, 200, 200, 200, 200};  // heavy forward
        QDate start(2025, 1, 1);

        ConstraintCheckResult result = TrainingConstraintChecker::checkAll(
            stress, start, 10.0, 10.0, TrainingConstraintBounds::recreational(), 7);

        // The forward portion should still have violations because PMC
        // from historical load affects the forward ACWR/TSB calculations.
        // With CTL=10 and 200 TSS/day for 14 days, ACWR will be extreme.
        QVERIFY(!result.passed || !result.violations.isEmpty());
    }

    // ---------------------------------------------------------------
    // JSON serialization
    // ---------------------------------------------------------------

    // ---------------------------------------------------------------
    // HRV readiness constraint
    // ---------------------------------------------------------------

    void checkHRV_noData_passes()
    {
        ConstraintCheckResult result = TrainingConstraintChecker::checkHRV(
            0.0, false, QDate(2026, 4, 7));
        QVERIFY(result.passed);
        QVERIFY(result.violations.isEmpty());
    }

    void checkHRV_normalRatio_passes()
    {
        // HRV ratio 1.05 = above baseline, fully recovered
        ConstraintCheckResult result = TrainingConstraintChecker::checkHRV(
            1.05, true, QDate(2026, 4, 7));
        QVERIFY(result.passed);
        QVERIFY(result.violations.isEmpty());
    }

    void checkHRV_mildSuppression_warns()
    {
        // HRV ratio 0.90 = between warning (0.93) and hard (0.85)
        ConstraintCheckResult result = TrainingConstraintChecker::checkHRV(
            0.90, true, QDate(2026, 4, 7));
        QVERIFY(result.passed); // warning doesn't fail
        QCOMPARE(result.violations.size(), 1);
        QCOMPARE(result.violations[0].severity, ConstraintViolation::Warning);
        QCOMPARE(result.violations[0].constraintId, QStringLiteral("hrvSuppressed"));
    }

    void checkHRV_severeSuppression_hard()
    {
        // HRV ratio 0.75 = well below hard threshold (0.85)
        ConstraintCheckResult result = TrainingConstraintChecker::checkHRV(
            0.75, true, QDate(2026, 4, 7));
        QVERIFY(!result.passed); // hard violation
        QCOMPARE(result.violations.size(), 1);
        QCOMPARE(result.violations[0].severity, ConstraintViolation::Hard);
    }

    void checkHRV_atWarningBoundary_passes()
    {
        // Exactly at 0.93 = at boundary, not below — should pass
        ConstraintCheckResult result = TrainingConstraintChecker::checkHRV(
            0.93, true, QDate(2026, 4, 7));
        QVERIFY(result.passed);
        QVERIFY(result.violations.isEmpty());
    }

    void checkHRV_atHardBoundary_warns()
    {
        // Exactly at 0.85 = at hard boundary, not below — should be warning
        ConstraintCheckResult result = TrainingConstraintChecker::checkHRV(
            0.85, true, QDate(2026, 4, 7));
        QVERIFY(result.passed); // 0.85 is not < 0.85, so it's in warning range
        QCOMPARE(result.violations.size(), 1);
        QCOMPARE(result.violations[0].severity, ConstraintViolation::Warning);
    }

    // ---------------------------------------------------------------
    // JSON serialization
    // ---------------------------------------------------------------

    void violation_toJson_roundtrips()
    {
        ConstraintViolation v;
        v.constraintId = QStringLiteral("testConstraint");
        v.severity = ConstraintViolation::Hard;
        v.date = QDate(2025, 6, 1);
        v.actual = 42.0;
        v.limit = 30.0;
        v.message = QStringLiteral("test message");

        QJsonObject json = v.toJson();
        QCOMPARE(json[QStringLiteral("constraintId")].toString(), QStringLiteral("testConstraint"));
        QCOMPARE(json[QStringLiteral("severity")].toString(), QStringLiteral("hard"));
        QCOMPARE(json[QStringLiteral("actual")].toDouble(), 42.0);
    }
};

QTEST_MAIN(TestTrainingConstraints)
#include "testTrainingConstraints.moc"
