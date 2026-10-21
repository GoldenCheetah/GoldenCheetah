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
#include <QNetworkRequest>
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
        QNetworkReply *namGet(QNetworkRequest &request) override
        {
            QNetworkReply *reply = new MockReply(messageIn_, httpStatusCode_, request);
            QTimer::singleShot(0, reply, [reply]() {
                emit reply->readyRead();
                emit reply->finished();
            });
            return reply;
        }

        QNetworkReply *namPost(QNetworkRequest &request, QByteArray &body) override
        {
            QNetworkReply *reply = new MockReply(body, httpStatusCode_, request);
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
        void testReaddirEmpty();
        void testReaddirSingle();
        void testReaddirTwo();
        void testReaddirToday();
        void testReadFile();
        void testStatusError();
        void testSendFile();
        void testCheckUpload();

    private:
        const QTimeZone berlinZone = QTimeZone(QByteArray("Europe/Berlin"));
};

void
TestWahoo::testReaddirEmpty()
{
   QString emptyResponse = R"json({
                                "workouts": [],
                                "total": 1,
                                "page": 1,
                                "per_page": 30,
                                "order": "descending",
                                "sort": "starts"
                            })json";
    const QDateTime now = QDateTime::currentDateTime();
    QDateTime from = QDateTime(now.addDays(-30).date(), QTime());

    WahooClientTest client(emptyResponse);
    QStringList errors;
    QList<WahooWorkoutEntry> result = client.fetchWorkouts("test_token", errors, from, now);
    QVERIFY2(errors.isEmpty(), qPrintable(errors.join("&&")));
    result.clear();
    QCOMPARE(result.isEmpty(), true);
    qDebug() << "End of testReaddirEmpty()";
}


void
TestWahoo::testReaddirSingle()
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
    QList<WahooWorkoutEntry> result = client.fetchWorkouts("test_token", errors, from, to);
    QVERIFY2(errors.isEmpty(), qPrintable(errors.join("&&")));
    QCOMPARE(result.size(), 1);
    QCOMPARE(result.at(0).id, "56519");
    QCOMPARE(result.at(0).name, "2026_09_30_11_00_00.fit");
}

void
TestWahoo::testReaddirTwo()
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
    QList<WahooWorkoutEntry> result = client.fetchWorkouts("test_token", errors, from, to);
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

void 
TestWahoo::testReaddirToday()
{
    QString todayRide = R"json({
                            "workouts": [
                                {
                                "id": 1,
                                "starts": "2026-10-18T09:00:00.000Z",
                                "minutes": 12,
                                "name": "Friday afternoon",
                                "plan_id": null,
                                "plan_ids": [],
                                "route_id": null,
                                "workout_token": "123",
                                "workout_type_id": 40,
                                "workout_summary": null,
                                "created_at": "2026-10-18T10:41:55.000Z",
                                "updated_at": "2026-10-18T10:41:55.000Z"
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
        std::chrono::day(18))), QTime());
        from.setTimeZone(berlinZone);

    QDateTime to = QDateTime(QDate(std::chrono::year_month_day(
        std::chrono::year(2026),
        std::chrono::month(10),
        std::chrono::day(18))), QTime(11, 00, 01));
        to.setTimeZone(berlinZone);

    WahooClientTest client(todayRide);
    QStringList errors;
    QList<WahooWorkoutEntry> result = client.fetchWorkouts("test_token", errors, from, to);
    QVERIFY2(errors.isEmpty(), qPrintable(errors.join("&&")));
    QCOMPARE(result.size(), 1);
    QCOMPARE(result.at(0).id, "1");
    QCOMPARE(result.at(0).name, "2026_10_18_11_00_00.fit");
}

void
TestWahoo::testReadFile()
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

void
TestWahoo::testStatusError()
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
    QList<WahooWorkoutEntry> result = client.fetchWorkouts("test_token", errors, now, now);
    QVERIFY(!errors.isEmpty());
    QCOMPARE(errors.at(0), "The API key test_token is revoked.");
}

void
TestWahoo::testSendFile()
{
    QByteArray fileData = "Data";
    const QByteArray base64String = fileData.toBase64(QByteArray::Base64UrlEncoding);
    QString apiResponse = R"json({
                            "id": 5,
                            "user_id": 1,
                            "token": "fRa2HvZ1N0WxuHw",
                            "status": "pending",
                            "time_zone": null,
                            "workout_id": null,
                            "workout_summary_id": null,
                            "workout_file_id": null,
                            "workout_name": "test",
                            "error": null,
                            "target_workout_id": null,
                            "created_at": "2018-10-23T20:43:50.000Z",
                            "updated_at": "2018-10-23T20:43:50.000Z"
                        })json";
    WahooClientTest client(apiResponse);
    auto *reply = client.sendFile("test_token", fileData, "test", "5");
    
    auto result = reply->readAll();
    QUrlQuery received;
    received.setQuery(QString::fromUtf8(result));

    QCOMPARE(
        received.queryItemValue(
            QStringLiteral("workout_file_upload[file]")
        ),
        /* Data base64 encoded is RGF0YQ==*/
        QStringLiteral("data:application/vnd.fit;base64,RGF0YQ==")
    );

    QCOMPARE(
        received.queryItemValue(
            QStringLiteral("workout_file_upload[filename]")
        ),
        QStringLiteral("test")
    );

    QCOMPARE(
        received.queryItemValue(
            QStringLiteral("workout_file_upload[time_zone]")
        ),
        QTimeZone::systemTimeZoneId()
    );
    reply->deleteLater();
}

void
TestWahoo::testCheckUpload()
{
    QString success = R"json({
                            "id": 5,
                            "user_id": 1,
                            "token": "fRa2HvZ1N0WxuHw",
                            "status": "complete",
                            "time_zone": null,
                            "workout_id": null,
                            "workout_summary_id": null,
                            "workout_file_id": null,
                            "workout_name": null,
                            "error": null,
                            "target_workout_id": null,
                            "created_at": "2018-10-23T20:43:50.000Z",
                            "updated_at": "2018-10-23T20:43:50.000Z"
                        })json";
    WahooClientTest client(success);
    QNetworkRequest request = QNetworkRequest(QUrl("https://upload.com/test"));
    QNetworkReply *reply = new MockReply(success.toUtf8(), 200, request);
    QString result = client.checkUpload(reply, "test");
    QCOMPARE(result, "Completed.");
    reply->deleteLater();

    QString error = R"json(
                            {
                                "id": 5,
                                "user_id": 1,
                                "token": "fRa2HvZ1N0WxuHw",
                                "status": "error",
                                "time_zone": null,
                                "workout_id": null,
                                "workout_summary_id": null,
                                "workout_file_id": null,
                                "workout_name": null,
                                "error": "My API error",
                                "target_workout_id": null,
                                "created_at": "2018-10-23T20:43:50.000Z",
                                "updated_at": "2018-10-23T20:43:50.000Z"
                            }
    )json";
    reply = new MockReply(error.toUtf8(), 200, request);
    result = client.checkUpload(reply, "test");
    QCOMPARE(result, "My API error");
}

QTEST_MAIN(TestWahoo)
#include "testWahoo.moc"