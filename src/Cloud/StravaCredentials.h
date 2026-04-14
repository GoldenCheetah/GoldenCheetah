#ifndef GC_StravaCredentials_h
#define GC_StravaCredentials_h

#include <QString>

QString configuredStravaClientId();
QString configuredStravaClientSecret();
bool hasConfiguredStravaClientSecret();
QString stravaCredentialSetupMessage();

#endif // GC_StravaCredentials_h
