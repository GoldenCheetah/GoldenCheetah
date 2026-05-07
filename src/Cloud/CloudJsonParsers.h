#ifndef GC_CloudJsonParsers_h
#define GC_CloudJsonParsers_h

#include <QByteArray>
#include <QJsonArray>
#include <QString>

enum class OAuthTokenSite {
    Dropbox,
    Strava,
    CyclingAnalytics,
    Nolio,
    SportTracks,
    Withings,
    Polar,
    Xert,
    RideWithGPS,
    Azum
};

struct OAuthTokenResponse {
    bool ok = false;
    QString accessToken;
    QString refreshToken;
    double polarUserId = 0;
    QString errorMessage;
};

struct StravaTokenRefreshResponse {
    bool ok = false;
    QString accessToken;
    QString refreshToken;
    QString errorMessage;
};

struct CyclingAnalyticsRideListResponse {
    bool ok = false;
    QJsonArray rides;
    QString errorMessage;
};

OAuthTokenResponse parseOAuthTokenResponse(OAuthTokenSite site, const QByteArray &payload);
StravaTokenRefreshResponse parseStravaTokenRefreshResponse(const QByteArray &payload);
CyclingAnalyticsRideListResponse parseCyclingAnalyticsRideListResponse(const QByteArray &payload);

#endif // GC_CloudJsonParsers_h
