#include "StravaCredentials.h"
#include "Secrets.h"

#include <QObject>
#include <QtGlobal>

namespace {

QString normalizedCredential(QString value, const QString &placeholder = QString())
{
    value = value.trimmed();
    if (value.isEmpty()) {
        return QString();
    }
    if (!placeholder.isEmpty() && value == placeholder) {
        return QString();
    }
    return value;
}

QString environmentCredential(const char *name, const QString &placeholder = QString())
{
    return normalizedCredential(QString::fromLocal8Bit(qgetenv(name)), placeholder);
}

} // namespace

QString configuredStravaClientId()
{
    const QString envValue = environmentCredential("GC_STRAVA_CLIENT_ID");
    if (!envValue.isEmpty()) {
        return envValue;
    }
    return normalizedCredential(QString::fromLatin1(GC_STRAVA_CLIENT_ID));
}

QString configuredStravaClientSecret()
{
    const QString placeholder = QStringLiteral("__GC_STRAVA_CLIENT_SECRET__");
    const QString envValue = environmentCredential("GC_STRAVA_CLIENT_SECRET", placeholder);
    if (!envValue.isEmpty()) {
        return envValue;
    }
    return normalizedCredential(QString::fromLatin1(GC_STRAVA_CLIENT_SECRET), placeholder);
}

bool hasConfiguredStravaClientSecret()
{
    return !configuredStravaClientSecret().isEmpty();
}

QString stravaCredentialSetupMessage()
{
    return QObject::tr("Set GC_STRAVA_CLIENT_ID and GC_STRAVA_CLIENT_SECRET in the environment, add them to src/gcconfig.pri, or use the official build.");
}

