/*
 * Unit tests for TrainingPlan.
 *
 * Tests JSON roundtrip serialization, phase validation,
 * status conversion, and computed fields.
 */

#include "Train/TrainingPlan.h"

#include <QTest>

class TestTrainingPlan : public QObject
{
    Q_OBJECT

private:

    TrainingPlanPhase makePhase(const QString &name, int weeks, double tss,
                                const QString &tmpl, const QString &goal)
    {
        TrainingPlanPhase p;
        p.name = name;
        p.durationWeeks = weeks;
        p.weeklyTSS = tss;
        p.templateName = tmpl;
        p.goal = goal;
        return p;
    }

    TrainingPlan makePlan()
    {
        TrainingPlan plan;
        plan.id = QStringLiteral("test-plan-001");
        plan.name = QStringLiteral("24-Week Dominance Plan");
        plan.description = QStringLiteral("Phased progressive build to peak fitness");
        plan.goal = QStringLiteral("build");
        plan.status = TrainingPlanStatus::Active;

        TrainingPlanPhase p1 = makePhase(QStringLiteral("Foundation"), 4, 120.0,
                                          QStringLiteral("flat"), QStringLiteral("recover"));
        p1.workoutMix[QStringLiteral("recovery")] = 0.4;
        p1.workoutMix[QStringLiteral("endurance")] = 0.6;
        plan.phases.append(p1);

        TrainingPlanPhase p2 = makePhase(QStringLiteral("Base"), 8, 200.0,
                                          QStringLiteral("progressive"), QStringLiteral("build"));
        p2.workoutMix[QStringLiteral("endurance")] = 0.5;
        p2.workoutMix[QStringLiteral("sweetspot")] = 0.3;
        p2.workoutMix[QStringLiteral("recovery")] = 0.2;
        plan.phases.append(p2);

        plan.startingCTL = 17.0;
        plan.startingATL = 29.0;
        plan.startingTSB = -12.0;
        plan.athleteFTP = 285;

        plan.banisterSnapshot.k1 = -0.0087;
        plan.banisterSnapshot.k2 = -0.0718;
        plan.banisterSnapshot.t1 = 50.0;
        plan.banisterSnapshot.t2 = 11.0;
        plan.banisterSnapshot.p0 = 81.4;
        plan.banisterSnapshot.isFitted = true;

        plan.startDate = QDate(2026, 4, 6);
        plan.endDate = QDate(2026, 7, 5);
        plan.generatorId = QStringLiteral("goldencheetah-mcp");
        plan.createdAt = QDateTime(QDate(2026, 4, 5), QTime(5, 20, 0), QTimeZone::UTC);
        plan.updatedAt = plan.createdAt;

        plan.evaluationDates.append(QDate(2026, 5, 4));
        plan.evaluationDates.append(QDate(2026, 6, 1));

        return plan;
    }

private slots:

    // ---------------------------------------------------------------
    // Phase JSON roundtrip
    // ---------------------------------------------------------------

    void phase_toJson_fromJson_roundtrip()
    {
        TrainingPlanPhase p = makePhase(QStringLiteral("Build"), 8, 300.0,
                                         QStringLiteral("progressive"), QStringLiteral("build"));
        p.workoutMix[QStringLiteral("threshold")] = 0.3;
        p.workoutMix[QStringLiteral("endurance")] = 0.5;
        p.notes = QStringLiteral("Focus on FTP development");

        QJsonObject json = p.toJson();
        bool ok = false;
        TrainingPlanPhase p2 = TrainingPlanPhase::fromJson(json, &ok);

        QVERIFY(ok);
        QCOMPARE(p2.name, p.name);
        QCOMPARE(p2.durationWeeks, p.durationWeeks);
        QCOMPARE(p2.weeklyTSS, p.weeklyTSS);
        QCOMPARE(p2.templateName, p.templateName);
        QCOMPARE(p2.goal, p.goal);
        QCOMPARE(p2.notes, p.notes);
        QCOMPARE(p2.workoutMix.size(), 2);
        QCOMPARE(p2.workoutMix[QStringLiteral("threshold")], 0.3);
    }

    void phase_fromJson_emptyName_fails()
    {
        QJsonObject json;
        json[QStringLiteral("durationWeeks")] = 4;
        // name is missing
        bool ok = true;
        TrainingPlanPhase::fromJson(json, &ok);
        QVERIFY(!ok);
    }

    // ---------------------------------------------------------------
    // Plan JSON roundtrip
    // ---------------------------------------------------------------

    void plan_toJson_fromJson_roundtrip()
    {
        TrainingPlan plan = makePlan();
        QJsonObject json = plan.toJson();

        bool ok = false;
        TrainingPlan plan2 = TrainingPlan::fromJson(json, &ok);

        QVERIFY(ok);
        QCOMPARE(plan2.id, plan.id);
        QCOMPARE(plan2.name, plan.name);
        QCOMPARE(plan2.description, plan.description);
        QCOMPARE(plan2.goal, plan.goal);
        QCOMPARE(plan2.status, TrainingPlanStatus::Active);
        QCOMPARE(plan2.phases.size(), 2);
        QCOMPARE(plan2.phases[0].name, QStringLiteral("Foundation"));
        QCOMPARE(plan2.phases[1].name, QStringLiteral("Base"));
        QCOMPARE(plan2.startingCTL, 17.0);
        QCOMPARE(plan2.athleteFTP, 285);
        QCOMPARE(plan2.startDate, QDate(2026, 4, 6));
        QCOMPARE(plan2.endDate, QDate(2026, 7, 5));
        QCOMPARE(plan2.generatorId, QStringLiteral("goldencheetah-mcp"));
        QCOMPARE(plan2.evaluationDates.size(), 2);
    }

    void plan_fromJson_missingId_fails()
    {
        QJsonObject json;
        json[QStringLiteral("name")] = QStringLiteral("Test Plan");
        // id is missing
        bool ok = true;
        TrainingPlan::fromJson(json, &ok);
        QVERIFY(!ok);
    }

    void plan_fromJson_missingName_fails()
    {
        QJsonObject json;
        json[QStringLiteral("id")] = QStringLiteral("abc-123");
        // name is missing
        bool ok = true;
        TrainingPlan::fromJson(json, &ok);
        QVERIFY(!ok);
    }

    // ---------------------------------------------------------------
    // Banister snapshot roundtrip
    // ---------------------------------------------------------------

    void plan_banisterSnapshot_roundtrip()
    {
        TrainingPlan plan = makePlan();
        QJsonObject json = plan.toJson();

        bool ok = false;
        TrainingPlan plan2 = TrainingPlan::fromJson(json, &ok);

        QVERIFY(ok);
        QCOMPARE(plan2.banisterSnapshot.t1, 50.0);
        QCOMPARE(plan2.banisterSnapshot.t2, 11.0);
        QVERIFY(plan2.banisterSnapshot.isFitted);
    }

    // ---------------------------------------------------------------
    // Constraint bounds roundtrip
    // ---------------------------------------------------------------

    void plan_constraintBounds_defaultValues()
    {
        TrainingPlan plan;
        plan.id = QStringLiteral("test");
        plan.name = QStringLiteral("test");
        QJsonObject json = plan.toJson();

        bool ok = false;
        TrainingPlan plan2 = TrainingPlan::fromJson(json, &ok);

        QVERIFY(ok);
        QCOMPARE(plan2.constraintBounds.maxCtlRampPerWeek, 5.0);
        QCOMPARE(plan2.constraintBounds.maxACWR, 1.3);
        QCOMPARE(plan2.constraintBounds.tsbFloor, -30.0);
    }

    // ---------------------------------------------------------------
    // Computed fields
    // ---------------------------------------------------------------

    void totalWeeks_sumsPhaseDurations()
    {
        TrainingPlan plan = makePlan();
        QCOMPARE(plan.totalWeeks(), 12); // 4 + 8
    }

    void totalPlannedTSS_sumsCorrectly()
    {
        TrainingPlan plan = makePlan();
        // Phase 1: 120 * 4 = 480, Phase 2: 200 * 8 = 1600
        QCOMPARE(plan.totalPlannedTSS(), 2080.0);
    }

    // ---------------------------------------------------------------
    // Status conversion
    // ---------------------------------------------------------------

    void statusRoundtrip()
    {
        QCOMPARE(trainingPlanStatusFromString(QStringLiteral("draft")), TrainingPlanStatus::Draft);
        QCOMPARE(trainingPlanStatusFromString(QStringLiteral("Active")), TrainingPlanStatus::Active);
        QCOMPARE(trainingPlanStatusFromString(QStringLiteral("COMPLETED")), TrainingPlanStatus::Completed);
        QCOMPARE(trainingPlanStatusFromString(QStringLiteral("archived")), TrainingPlanStatus::Archived);
        QCOMPARE(trainingPlanStatusFromString(QStringLiteral("unknown")), TrainingPlanStatus::Draft);

        QCOMPARE(trainingPlanStatusToString(TrainingPlanStatus::Draft), QStringLiteral("draft"));
        QCOMPARE(trainingPlanStatusToString(TrainingPlanStatus::Active), QStringLiteral("active"));
        QCOMPARE(trainingPlanStatusToString(TrainingPlanStatus::Completed), QStringLiteral("completed"));
        QCOMPARE(trainingPlanStatusToString(TrainingPlanStatus::Archived), QStringLiteral("archived"));
    }

    // ---------------------------------------------------------------
    // ID generation
    // ---------------------------------------------------------------

    void generateId_isUnique()
    {
        QString id1 = TrainingPlan::generateId();
        QString id2 = TrainingPlan::generateId();
        QVERIFY(!id1.isEmpty());
        QVERIFY(!id2.isEmpty());
        QVERIFY(id1 != id2);
    }

    // ---------------------------------------------------------------
    // Schema version
    // ---------------------------------------------------------------

    void schemaVersion_isPersisted()
    {
        TrainingPlan plan;
        plan.id = QStringLiteral("v-test");
        plan.name = QStringLiteral("version test");
        QJsonObject json = plan.toJson();
        QCOMPARE(json[QStringLiteral("schemaVersion")].toString(), QStringLiteral("1"));
    }
};

QTEST_MAIN(TestTrainingPlan)
#include "testTrainingPlan.moc"
