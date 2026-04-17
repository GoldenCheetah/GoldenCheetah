#include "Cloud/StravaCredentials.h"

#include <QByteArray>
#include <QTest>

namespace {

struct ScopedEnvVar
{
    QByteArray name;
    QByteArray originalValue;
    bool hadOriginal = false;

    explicit ScopedEnvVar(const char *varName)
        : name(varName)
    {
        originalValue = qgetenv(name.constData());
        hadOriginal = qEnvironmentVariableIsSet(name.constData());
    }

    ~ScopedEnvVar()
    {
        if (hadOriginal) {
            qputenv(name.constData(), originalValue);
        } else {
            qunsetenv(name.constData());
        }
    }
};

} // namespace

class TestStravaCredentials : public QObject
{
    Q_OBJECT

private slots:
    void configuredStravaClientId_prefersEnvironmentOverride()
    {
        ScopedEnvVar clientId("GC_STRAVA_CLIENT_ID");
        ScopedEnvVar clientSecret("GC_STRAVA_CLIENT_SECRET");

        qputenv("GC_STRAVA_CLIENT_ID", "local-client-id");
        qputenv("GC_STRAVA_CLIENT_SECRET", "local-client-secret");

        QCOMPARE(configuredStravaClientId(), QString("local-client-id"));
        QCOMPARE(configuredStravaClientSecret(), QString("local-client-secret"));
        QVERIFY(hasConfiguredStravaClientSecret());
    }

    void configuredStravaClientSecret_rejectsPlaceholderOverride()
    {
        ScopedEnvVar clientSecret("GC_STRAVA_CLIENT_SECRET");

        qputenv("GC_STRAVA_CLIENT_SECRET", "__GC_STRAVA_CLIENT_SECRET__");

        QCOMPARE(configuredStravaClientSecret(), QString());
        QVERIFY(!hasConfiguredStravaClientSecret());
    }

    void stravaCredentialSetupMessage_mentionsSupportedConfigurationPaths()
    {
        const QString message = stravaCredentialSetupMessage();

        QVERIFY(message.contains("GC_STRAVA_CLIENT_ID"));
        QVERIFY(message.contains("GC_STRAVA_CLIENT_SECRET"));
        QVERIFY(message.contains("src/gcconfig.pri"));
    }
};

QTEST_MAIN(TestStravaCredentials)
#include "testStravaCredentials.moc"
