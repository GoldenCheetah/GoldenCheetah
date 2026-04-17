#include "Cloud/CloudJsonParsers.h"

#include <QTest>

class TestCloudJsonParsers : public QObject
{
    Q_OBJECT

private slots:

    void parseOAuthTokenResponse_rejectsMalformedPayload()
    {
        const OAuthTokenResponse response = parseOAuthTokenResponse(OAuthTokenSite::Strava, QByteArray("{"));

        QVERIFY(!response.ok);
        QVERIFY(response.errorMessage.contains("JSON parser error", Qt::CaseInsensitive));
    }

    void parseOAuthTokenResponse_rejectsExplicitErrorPayload()
    {
        const QByteArray payload = R"({"error":"invalid_grant","error_description":"expired code"})";
        const OAuthTokenResponse response = parseOAuthTokenResponse(OAuthTokenSite::Strava, payload);

        QVERIFY(!response.ok);
        QVERIFY(response.errorMessage.contains("invalid_grant"));
        QVERIFY(response.errorMessage.contains("expired code"));
    }

    void parseOAuthTokenResponse_rejectsMissingRequiredTokens()
    {
        const QByteArray payload = R"({"refresh_token":"refresh-only"})";
        const OAuthTokenResponse response = parseOAuthTokenResponse(OAuthTokenSite::Strava, payload);

        QVERIFY(!response.ok);
        QVERIFY(response.errorMessage.contains("Access token", Qt::CaseInsensitive));
    }

    void parseOAuthTokenResponse_acceptsWithingsNestedTokens()
    {
        const QByteArray payload = R"({"status":0,"body":{"access_token":"withings-access","refresh_token":"withings-refresh"}})";
        const OAuthTokenResponse response = parseOAuthTokenResponse(OAuthTokenSite::Withings, payload);

        QVERIFY(response.ok);
        QCOMPARE(response.accessToken, QString("withings-access"));
        QCOMPARE(response.refreshToken, QString("withings-refresh"));
    }

    void parseOAuthTokenResponse_acceptsRideWithGpsNestedToken()
    {
        const QByteArray payload = R"({"user":{"auth_token":"rwgps-token"}})";
        const OAuthTokenResponse response = parseOAuthTokenResponse(OAuthTokenSite::RideWithGPS, payload);

        QVERIFY(response.ok);
        QCOMPARE(response.accessToken, QString("rwgps-token"));
    }

    void parseOAuthTokenResponse_rejectsMissingPolarUserId()
    {
        const QByteArray payload = R"({"access_token":"polar-token"})";
        const OAuthTokenResponse response = parseOAuthTokenResponse(OAuthTokenSite::Polar, payload);

        QVERIFY(!response.ok);
        QVERIFY(response.errorMessage.contains("Polar user id", Qt::CaseInsensitive));
    }

    void parseStravaTokenRefreshResponse_rejectsErrorPayload()
    {
        const QByteArray payload = R"({"message":"Authorization Error","errors":[{"resource":"AccessToken","field":"refresh_token","code":"invalid"}]})";
        const StravaTokenRefreshResponse response = parseStravaTokenRefreshResponse(payload);

        QVERIFY(!response.ok);
        QVERIFY(response.errorMessage.contains("Authorization Error"));
        QVERIFY(response.errorMessage.contains("refresh_token"));
    }

    void parseStravaTokenRefreshResponse_acceptsValidPayload()
    {
        const QByteArray payload = R"({"access_token":"strava-access","refresh_token":"strava-refresh"})";
        const StravaTokenRefreshResponse response = parseStravaTokenRefreshResponse(payload);

        QVERIFY(response.ok);
        QCOMPARE(response.accessToken, QString("strava-access"));
        QCOMPARE(response.refreshToken, QString("strava-refresh"));
    }

    void parseCyclingAnalyticsRideListResponse_rejectsErrorPayload()
    {
        const QByteArray payload = R"({"error":"bad_token"})";
        const CyclingAnalyticsRideListResponse response = parseCyclingAnalyticsRideListResponse(payload);

        QVERIFY(!response.ok);
        QVERIFY(response.errorMessage.contains("bad_token"));
    }

    void parseCyclingAnalyticsRideListResponse_acceptsEmptyRideList()
    {
        const QByteArray payload = R"({"rides":[]})";
        const CyclingAnalyticsRideListResponse response = parseCyclingAnalyticsRideListResponse(payload);

        QVERIFY(response.ok);
        QCOMPARE(response.rides.size(), 0);
    }
};

QTEST_MAIN(TestCloudJsonParsers)
#include "testCloudJsonParsers.moc"
