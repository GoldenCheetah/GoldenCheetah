/*
 * Copyright (c) 2026 Felix Gertz (felix@tredict.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef GC_OAuthPKCE_h
#define GC_OAuthPKCE_h

#include <QObject>
#include <QString>
#include <QMap>

class QNetworkAccessManager;

// Reusable OAuth2 + PKCE authorization flow using a localhost callback
// and the system browser. Provider-agnostic: any CloudService can use
// this by supplying its authorization URL, token URL, client ID, and scopes.
//
// Usage:
//   OAuthPKCE oauth;
//   oauth.setAuthorizationUrl("https://provider.com/authorize");
//   oauth.setTokenUrl("https://provider.com/token");
//   oauth.setClientId("my_client_id");
//   oauth.setScope("read write");
//   if (oauth.execute()) {
//       // oauth.accessToken(), oauth.refreshToken()
//   }

class OAuthPKCE : public QObject {
    Q_OBJECT

public:
    OAuthPKCE(QObject *parent = nullptr);
    ~OAuthPKCE();

    // Configuration - call before execute()
    void setAuthorizationUrl(const QString &url);
    void setTokenUrl(const QString &url);
    void setClientId(const QString &clientId);
    void setScope(const QString &scope);
    void setCallbackPath(const QString &path);
    void setTimeout(int seconds);
    void setExtraAuthParams(const QMap<QString,QString> &params);

    // Blocking: opens system browser, waits for callback, exchanges token.
    bool execute();

    // Results - valid after successful execute()
    QString accessToken() const;
    QString refreshToken() const;
    int expiresIn() const;
    QString errorString() const;

    // Static utilities
    static QString generateCodeVerifier();
    static QString computeCodeChallenge(const QString &verifier);

    static bool refreshAccessToken(
        const QString &tokenUrl,
        const QString &clientId,
        const QString &currentRefreshToken,
        QString &newAccessToken,
        QString &newRefreshToken,
        int &expiresIn,
        QString &errorString
    );

private:
    bool exchangeCodeForTokens(const QString &code, const QString &codeVerifier,
                               const QString &redirectUri);

    QString authorizationUrl_;
    QString tokenUrl_;
    QString clientId_;
    QString scope_;
    QString callbackPath_;
    int timeout_;
    QMap<QString,QString> extraAuthParams_;

    QString accessToken_;
    QString refreshToken_;
    int expiresIn_;
    QString errorString_;
};

#endif // GC_OAuthPKCE_h
