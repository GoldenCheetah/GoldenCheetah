#include <QString>
#include <QStringList>
#include <QList>
#include <QByteArray>
#include <QTimer>
#include <QDateTime>
#include <QTimeZone>
#include <QScopedPointer>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QTest>

#include "WahooClient.h"
#include "mockNetworkReply.h"

class WahooClientTest : public WahooClient
{
    public:
        explicit WahooClientTest(const QString &messageIn)
            : messageIn_(messageIn.toUtf8()) {}

    void setStatusCode(const QVariant &httpStatusCode)
    {
        httpStatusCode_ = httpStatusCode;
    }

    protected:
        QNetworkReply* namGet(QNetworkRequest &request) override
        {
            QNetworkReply *reply = new MockReply(messageIn_, httpStatusCode_, request);
            QTimer::singleShot(0, reply, [reply]() {
                emit reply->readyRead();
                emit reply->finished();
            });
            return reply;
        }

    private:
        QByteArray messageIn_;
        QVariant httpStatusCode_ = 200;
};

class TestWahoo : public QObject
{
    Q_OBJECT

    public:
        TestWahoo() = default;
        virtual ~TestWahoo() {}

    private slots:
        void testReaddirEmpty()
        {
            QString emptyResponse = R"json({
                                        "workouts": [],
                                        "total": 0,
                                        "page": 1,
                                        "per_page": 30,
                                        "order": "descending",
                                        "sort": "starts"
                                    })json";
            const QDateTime now = QDateTime::currentDateTime();
            QDateTime from = QDateTime(now.addDays(-30).date(), QTime());

            WahooClientTest client(emptyResponse);
            QStringList errors;
            auto result = client.fetchWorkouts("test_token", errors, from, now);
            QVERIFY2(errors.isEmpty(), qPrintable(errors.join("&&")));
            QVERIFY(result.isEmpty());
        }


        void testReaddirSingle()
        {
            QString singleWorkout = R"json({
                                    "workouts": [
                                        {
                                        "id": 56519,
                                        "starts": "2026-09-30T09:00:00.000Z",
                                        "minutes": 12,
                                        "name": "Friday afternoon",
                                        "plan_id": null,
                                        "plan_ids": [],
                                        "route_id": null,
                                        "workout_token": "123",
                                        "workout_type_id": 40,
                                        "workout_summary": null,
                                        "created_at": "2026-09-30T20:41:55.000Z",
                                        "updated_at": "2026-09-30T20:41:55.000Z"
                                        }
                                    ],
                                    "total": 1,
                                    "page": 1,
                                    "per_page": 1,
                                    "order": "descending",
                                    "sort": "starts"
                                    })json";
            QDateTime from = QDateTime(QDate(std::chrono::year_month_day(
                std::chrono::year(2026),
                std::chrono::month(9),
                std::chrono::day(30))), QTime());
                from.setTimeZone(berlinZone);

            QDateTime to = QDateTime(QDate(std::chrono::year_month_day(
                std::chrono::year(2026),
                std::chrono::month(10),
                std::chrono::day(1))), QTime());
                to.setTimeZone(berlinZone);

            WahooClientTest client(singleWorkout);
            QStringList errors;
            auto result = client.fetchWorkouts("test_token", errors, from, to);
            QVERIFY2(errors.isEmpty(), qPrintable(errors.join("&&")));
            QCOMPARE(result.size(), 1);
            QCOMPARE(result.at(0).id, "56519");
            QCOMPARE(result.at(0).name, "2026_09_30_11_00_00.fit");
        }

        void testReaddirTwo()
        {
            QString twoWorkouts = R"json({
                                    "workouts": [
                                        {
                                        "id": 1,
                                        "starts": "2026-09-30T09:00:00.000Z",
                                        "minutes": 12,
                                        "name": "Friday afternoon",
                                        "plan_id": null,
                                        "plan_ids": [],
                                        "route_id": null,
                                        "workout_token": "123",
                                        "workout_type_id": 40,
                                        "workout_summary": null
                                        },
                                        {
                                        "id": 2,
                                        "starts": "2026-09-10T09:00:00.000Z",
                                        "minutes": 12,
                                        "name": "Friday afternoon",
                                        "plan_id": null,
                                        "plan_ids": [],
                                        "route_id": null,
                                        "workout_token": "123",
                                        "workout_type_id": 40,
                                        "workout_summary": null
                                        }
                                    ],
                                    "total": 2,
                                    "page": 1,
                                    "per_page": 1,
                                    "order": "descending",
                                    "sort": "starts"
                                    })json";
            QDateTime from = QDateTime(QDate(std::chrono::year_month_day(
                std::chrono::year(2026),
                std::chrono::month(9),
                std::chrono::day(10))), QTime());
                from.setTimeZone(berlinZone);

            QDateTime to = QDateTime(QDate(std::chrono::year_month_day(
                std::chrono::year(2026),
                std::chrono::month(10),
                std::chrono::day(1))), QTime());
                to.setTimeZone(berlinZone);

            WahooClientTest client(twoWorkouts);
            QStringList errors;
            auto result = client.fetchWorkouts("test_token", errors, from, to);
            QVERIFY2(errors.isEmpty(), qPrintable(errors.join("&&")));
            QCOMPARE(result.size(), 2);
            QCOMPARE(result.at(0).id, "1");
            QCOMPARE(result.at(0).name, "2026_09_30_11_00_00.fit");
            QCOMPARE(result.at(1).id, "2");
            QCOMPARE(result.at(1).name, "2026_09_10_11_00_00.fit");

            qDebug() << "Checking from after second item";
            from = QDateTime(QDate(std::chrono::year_month_day(
                std::chrono::year(2026),
                std::chrono::month(9),
                std::chrono::day(11))), QTime());

            result = client.fetchWorkouts("test_token", errors, from, to);
            QCOMPARE(result.size(), 1);
            QCOMPARE(result.at(0).id, "1");

            qDebug() << "Checking to before first item";
            from = QDateTime(QDate(std::chrono::year_month_day(
                std::chrono::year(2026),
                std::chrono::month(9),
                std::chrono::day(10))), QTime());
            to = QDateTime(QDate(std::chrono::year_month_day(
                std::chrono::year(2026),
                std::chrono::month(9),
                std::chrono::day(11))), QTime());
            result = client.fetchWorkouts("test_token", errors, from, to);
            QCOMPARE(result.size(), 1);
            QCOMPARE(result.at(0).id, "2");

            qDebug() << "Checking empty range";
            from = to;
            result = client.fetchWorkouts("test_token", errors, from, to);
            QCOMPARE(result.size(), 0);
        }

        void testReadFile()
        {
            QString summaryResponse = R"json({
                "file": {
                    "url": "https://testurl/file.fit"
                }
            })json";

            WahooClientTest client(summaryResponse);
            QStringList errors;
            QNetworkReply *reply = client.startFileDownload("test_token", "12345", errors);
            QVERIFY(errors.isEmpty());
            QVERIFY(reply != nullptr);
            QCOMPARE(reply->request().url().toString(), "https://testurl/file.fit");
            delete reply;
        }

        void test401()
        {
            QString emptyResponse = R"json({
                            "workouts": [],
                            "total": 0,
                            "page": 1,
                            "per_page": 30,
                            "order": "descending",
                            "sort": "starts"
                        })json";
            WahooClientTest client(emptyResponse);
            client.setStatusCode(401);
            QStringList errors;
            const QDateTime now = QDateTime::currentDateTime();
            auto result = client.fetchWorkouts("test_token", errors, now, now);
            QVERIFY(errors.isEmpty() > 0);
            QCOMPARE(errors.at(0), "The API key is revoked.");
        }

    private:
        const QTimeZone berlinZone = QTimeZone(QByteArray("Europe/Berlin"));
};

QTEST_MAIN(TestWahoo)
#include "testWahoo.moc"