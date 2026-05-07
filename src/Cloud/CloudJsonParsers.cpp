#include "CloudJsonParsers.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QStringList>

namespace {

QString compactJsonValue(const QJsonValue &value)
{
    if (value.isString()) return value.toString().trimmed();
    if (value.isDouble()) return QString::number(value.toDouble());
    if (value.isBool()) return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    if (value.isObject()) return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
    if (value.isArray()) return QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
    return QString();
}

void appendMessage(QStringList &messages, const QString &message)
{
    const QString trimmed = message.trimmed();
    if (!trimmed.isEmpty() && !messages.contains(trimmed)) messages << trimmed;
}

void appendMessage(QStringList &messages, const QJsonValue &value)
{
    appendMessage(messages, compactJsonValue(value));
}

QString jsonErrorMessage(const QJsonObject &object)
{
    QStringList messages;
    appendMessage(messages, object.value("error"));
    appendMessage(messages, object.value("error_description"));
    appendMessage(messages, object.value("message"));
    appendMessage(messages, object.value("detail"));

    const QJsonValue errorsValue = object.value("errors");
    if (errorsValue.isArray()) {
        for (const QJsonValue &entry : errorsValue.toArray()) {
            if (entry.isObject()) {
                const QJsonObject errorObject = entry.toObject();
                QStringList parts;
                appendMessage(parts, errorObject.value("message"));
                appendMessage(parts, errorObject.value("resource"));
                appendMessage(parts, errorObject.value("field"));
                appendMessage(parts, errorObject.value("code"));
                if (!parts.isEmpty()) appendMessage(messages, parts.join(QStringLiteral(" ")));
            } else {
                appendMessage(messages, entry);
            }
        }
    } else if (errorsValue.isObject()) {
        appendMessage(messages, compactJsonValue(errorsValue));
    }

    return messages.join(QStringLiteral(": "));
}

bool requiresRefreshToken(OAuthTokenSite site)
{
    switch (site) {
    case OAuthTokenSite::Strava:
    case OAuthTokenSite::Nolio:
    case OAuthTokenSite::SportTracks:
    case OAuthTokenSite::Withings:
    case OAuthTokenSite::Xert:
    case OAuthTokenSite::Azum:
        return true;
    default:
        return false;
    }
}

OAuthTokenResponse parseOAuthTokenDocument(OAuthTokenSite site, const QJsonObject &object)
{
    OAuthTokenResponse response;

    const QString apiError = jsonErrorMessage(object);
    if (!apiError.isEmpty()) {
        response.errorMessage = apiError;
        return response;
    }

    switch (site) {
    case OAuthTokenSite::RideWithGPS:
        response.accessToken = object.value("user").toObject().value("auth_token").toString().trimmed();
        break;
    case OAuthTokenSite::Withings: {
        const QJsonObject body = object.value("body").toObject();
        response.accessToken = body.value("access_token").toString().trimmed();
        response.refreshToken = body.value("refresh_token").toString().trimmed();
        if (object.contains("status") && object.value("status").toInt() != 0 && response.accessToken.isEmpty()) {
            response.errorMessage = QStringLiteral("Withings returned status %1").arg(object.value("status").toInt());
            return response;
        }
        break;
    }
    case OAuthTokenSite::Polar:
        response.accessToken = object.value("access_token").toString().trimmed();
        response.refreshToken = object.value("refresh_token").toString().trimmed();
        response.polarUserId = object.value("x_user_id").toDouble();
        break;
    default:
        response.accessToken = object.value("access_token").toString().trimmed();
        response.refreshToken = object.value("refresh_token").toString().trimmed();
        break;
    }

    if (response.accessToken.isEmpty()) {
        response.errorMessage = QStringLiteral("Access token missing from OAuth response.");
        return response;
    }

    if (requiresRefreshToken(site) && response.refreshToken.isEmpty()) {
        response.errorMessage = QStringLiteral("Refresh token missing from OAuth response.");
        return response;
    }

    if (site == OAuthTokenSite::Polar && response.polarUserId <= 0) {
        response.errorMessage = QStringLiteral("Polar user id missing from OAuth response.");
        return response;
    }

    response.ok = true;
    return response;
}

template <typename Result>
Result jsonParseFailure(const QString &message)
{
    Result result;
    result.errorMessage = message;
    return result;
}

} // namespace

OAuthTokenResponse parseOAuthTokenResponse(OAuthTokenSite site, const QByteArray &payload)
{
    if (payload.trimmed().isEmpty()) {
        return jsonParseFailure<OAuthTokenResponse>(QStringLiteral("Empty OAuth token response."));
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return jsonParseFailure<OAuthTokenResponse>(QStringLiteral("JSON parser error: %1").arg(parseError.errorString()));
    }
    if (!document.isObject()) {
        return jsonParseFailure<OAuthTokenResponse>(QStringLiteral("Malformed OAuth token response."));
    }

    return parseOAuthTokenDocument(site, document.object());
}

StravaTokenRefreshResponse parseStravaTokenRefreshResponse(const QByteArray &payload)
{
    if (payload.trimmed().isEmpty()) {
        return jsonParseFailure<StravaTokenRefreshResponse>(QStringLiteral("Empty Strava token response."));
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return jsonParseFailure<StravaTokenRefreshResponse>(QStringLiteral("JSON parser error: %1").arg(parseError.errorString()));
    }
    if (!document.isObject()) {
        return jsonParseFailure<StravaTokenRefreshResponse>(QStringLiteral("Malformed Strava token response."));
    }

    StravaTokenRefreshResponse response;
    const QJsonObject object = document.object();
    response.errorMessage = jsonErrorMessage(object);
    if (!response.errorMessage.isEmpty()) return response;

    response.accessToken = object.value("access_token").toString().trimmed();
    response.refreshToken = object.value("refresh_token").toString().trimmed();
    if (response.accessToken.isEmpty()) {
        response.errorMessage = QStringLiteral("Access token missing from Strava token response.");
        return response;
    }

    response.ok = true;
    return response;
}

CyclingAnalyticsRideListResponse parseCyclingAnalyticsRideListResponse(const QByteArray &payload)
{
    if (payload.trimmed().isEmpty()) {
        return jsonParseFailure<CyclingAnalyticsRideListResponse>(QStringLiteral("Empty Cycling Analytics ride list response."));
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return jsonParseFailure<CyclingAnalyticsRideListResponse>(QStringLiteral("JSON parser error: %1").arg(parseError.errorString()));
    }
    if (!document.isObject()) {
        return jsonParseFailure<CyclingAnalyticsRideListResponse>(QStringLiteral("Malformed Cycling Analytics ride list response."));
    }

    CyclingAnalyticsRideListResponse response;
    const QJsonObject object = document.object();
    response.errorMessage = jsonErrorMessage(object);
    if (!response.errorMessage.isEmpty()) return response;

    const QJsonValue ridesValue = object.value("rides");
    if (!ridesValue.isArray()) {
        response.errorMessage = QStringLiteral("Cycling Analytics ride list response did not contain a rides array.");
        return response;
    }

    response.ok = true;
    response.rides = ridesValue.toArray();
    return response;
}
