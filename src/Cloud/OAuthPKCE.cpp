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

#include "OAuthPKCE.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRandomGenerator>
#include <QEventLoop>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

OAuthPKCE::OAuthPKCE(QObject *parent)
    : QObject(parent), callbackPath_("/callback"), timeout_(120), expiresIn_(0)
{
}

OAuthPKCE::~OAuthPKCE()
{
}

void OAuthPKCE::setAuthorizationUrl(const QString &url) { authorizationUrl_ = url; }
void OAuthPKCE::setTokenUrl(const QString &url) { tokenUrl_ = url; }
void OAuthPKCE::setClientId(const QString &clientId) { clientId_ = clientId; }
void OAuthPKCE::setScope(const QString &scope) { scope_ = scope; }
void OAuthPKCE::setCallbackPath(const QString &path) { callbackPath_ = path; }
void OAuthPKCE::setTimeout(int seconds) { timeout_ = seconds; }
void OAuthPKCE::setExtraAuthParams(const QMap<QString,QString> &params) { extraAuthParams_ = params; }

QString OAuthPKCE::accessToken() const { return accessToken_; }
QString OAuthPKCE::refreshToken() const { return refreshToken_; }
int OAuthPKCE::expiresIn() const { return expiresIn_; }
QString OAuthPKCE::errorString() const { return errorString_; }

QString
OAuthPKCE::generateCodeVerifier()
{
    // RFC 7636: 32 random bytes -> 43 base64url characters
    QByteArray random(32, 0);
    QRandomGenerator *rng = QRandomGenerator::global();
    for (int i = 0; i < 32; i++)
        random[i] = static_cast<char>(rng->bounded(256));

    return QString::fromLatin1(
        random.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)
    );
}

QString
OAuthPKCE::computeCodeChallenge(const QString &verifier)
{
    QByteArray hash = QCryptographicHash::hash(
        verifier.toUtf8(), QCryptographicHash::Sha256
    );
    return QString::fromLatin1(
        hash.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)
    );
}

bool
OAuthPKCE::execute()
{
    errorString_.clear();
    accessToken_.clear();
    refreshToken_.clear();
    expiresIn_ = 0;

    // generate PKCE pair
    QString codeVerifier = generateCodeVerifier();
    QString codeChallenge = computeCodeChallenge(codeVerifier);

    // generate random state for CSRF protection
    QByteArray stateBytes(16, 0);
    QRandomGenerator *rng = QRandomGenerator::global();
    for (int i = 0; i < 16; i++)
        stateBytes[i] = static_cast<char>(rng->bounded(256));
    QString state = QString::fromLatin1(
        stateBytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)
    );

    // start local server on a free port
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost, 0)) {
        errorString_ = tr("Failed to start local HTTP server: %1").arg(server.errorString());
        return false;
    }
    quint16 port = server.serverPort();
    QString redirectUri = QString("http://localhost:%1%2").arg(port).arg(callbackPath_);

    // build authorization URL
    QUrl authUrl(authorizationUrl_);
    QUrlQuery authParams;
    authParams.addQueryItem("client_id", clientId_);
    authParams.addQueryItem("response_type", "code");
    authParams.addQueryItem("redirect_uri", redirectUri);
    authParams.addQueryItem("code_challenge", codeChallenge);
    authParams.addQueryItem("code_challenge_method", "S256");
    authParams.addQueryItem("state", state);
    if (!scope_.isEmpty())
        authParams.addQueryItem("scope", scope_);

    QMapIterator<QString,QString> it(extraAuthParams_);
    while (it.hasNext()) {
        it.next();
        authParams.addQueryItem(it.key(), it.value());
    }
    authUrl.setQuery(authParams);

    // open system browser
    if (!QDesktopServices::openUrl(authUrl)) {
        server.close();
        errorString_ = tr("Failed to open system browser.");
        return false;
    }

    // wait for the callback
    QEventLoop loop;
    QString authCode;
    bool timedOut = false;
    bool gotCallback = false;

    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, [&timedOut, &loop]() {
        timedOut = true;
        loop.quit();
    });
    timer.start(timeout_ * 1000);

    connect(&server, &QTcpServer::newConnection, &server, [this, &server, &state, &gotCallback, &loop, &authCode]() {
        QTcpSocket *socket = server.nextPendingConnection();
        if (!socket) return;

        // wait for the request data
        connect(socket, &QTcpSocket::readyRead, socket, [this, socket, &state, &gotCallback, &loop, &authCode]() {
            QByteArray requestData = socket->readAll();
            QString requestLine = QString::fromUtf8(requestData).section("\r\n", 0, 0);

            // parse: GET /callback?code=XXX&state=YYY HTTP/1.1
            QStringList parts = requestLine.split(' ');
            if (parts.size() < 2) {
                socket->close();
                socket->deleteLater();
                return;
            }

            QUrl requestUrl = QUrl("http://localhost" + parts[1]);
            QUrlQuery query(requestUrl);

            QString receivedState = query.queryItemValue("state");
            if (receivedState != state) {
                // state mismatch, send error and keep waiting
                QByteArray response =
                    "HTTP/1.1 400 Bad Request\r\n"
                    "Content-Type: text/html\r\n"
                    "Connection: close\r\n\r\n"
                    "<html><body><h2>Authorization Failed</h2>"
                    "<p>State parameter mismatch. Please try again.</p>"
                    "</body></html>";
                socket->write(response);
                socket->flush();
                socket->disconnectFromHost();
                socket->deleteLater();
                return;
            }

            if (query.hasQueryItem("error")) {
                errorString_ = query.queryItemValue("error_description");
                if (errorString_.isEmpty())
                    errorString_ = query.queryItemValue("error");

                QByteArray response =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n"
                    "Connection: close\r\n\r\n"
                    "<html><body><h2>Authorization Failed</h2>"
                    "<p>You can close this window.</p>"
                    "</body></html>";
                socket->write(response);
                socket->flush();
                socket->disconnectFromHost();
                socket->deleteLater();
                gotCallback = true;
                loop.quit();
                return;
            }

            authCode = query.queryItemValue("code");

            QByteArray response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Connection: close\r\n\r\n"
                "<html><body style=\"font-family:sans-serif;text-align:center;padding:40px\">"
                "<h2>Authorization Successful</h2>"
                "<p>You can close this window and return to GoldenCheetah.</p>"
                "</body></html>";
            socket->write(response);
            socket->flush();
            socket->disconnectFromHost();
            socket->deleteLater();
            gotCallback = true;
            loop.quit();
        });
    });

    loop.exec();
    timer.stop();
    server.close();

    if (timedOut) {
        errorString_ = tr("Authorization timed out. No response from browser.");
        return false;
    }

    if (authCode.isEmpty()) {
        if (errorString_.isEmpty())
            errorString_ = tr("No authorization code received.");
        return false;
    }

    // exchange authorization code for tokens
    return exchangeCodeForTokens(authCode, codeVerifier, redirectUri);
}

bool
OAuthPKCE::exchangeCodeForTokens(const QString &code, const QString &codeVerifier,
                                  const QString &redirectUri)
{
    QNetworkAccessManager nam;
    QUrlQuery params;
    params.addQueryItem("grant_type", "authorization_code");
    params.addQueryItem("code", code);
    params.addQueryItem("client_id", clientId_);
    params.addQueryItem("code_verifier", codeVerifier);
    params.addQueryItem("redirect_uri", redirectUri);

    QUrl url(tokenUrl_);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply *reply = nam.post(request, params.query(QUrl::FullyEncoded).toUtf8());

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        errorString_ = tr("Token exchange failed: %1").arg(reply->errorString());
        reply->deleteLater();
        return false;
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        errorString_ = tr("Failed to parse token response: %1").arg(parseError.errorString());
        return false;
    }

    QJsonObject obj = doc.object();
    accessToken_ = obj["access_token"].toString();
    refreshToken_ = obj["refresh_token"].toString();

    if (obj.contains("expires_at"))
        expiresIn_ = obj["expires_at"].toInt();
    else if (obj.contains("expires_in"))
        expiresIn_ = obj["expires_in"].toInt();

    if (accessToken_.isEmpty()) {
        errorString_ = tr("No access token in response.");
        if (obj.contains("error"))
            errorString_ = obj["error_description"].toString();
        return false;
    }

    return true;
}

bool
OAuthPKCE::refreshAccessToken(const QString &tokenUrl, const QString &clientId,
                               const QString &currentRefreshToken,
                               QString &newAccessToken, QString &newRefreshToken,
                               int &expiresIn, QString &error)
{
    QNetworkAccessManager nam;
    QUrlQuery params;
    params.addQueryItem("grant_type", "refresh_token");
    params.addQueryItem("refresh_token", currentRefreshToken);
    params.addQueryItem("client_id", clientId);

    QUrl url(tokenUrl);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply *reply = nam.post(request, params.query(QUrl::FullyEncoded).toUtf8());

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        error = QObject::tr("Token refresh failed: %1").arg(reply->errorString());
        reply->deleteLater();
        return false;
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        error = QObject::tr("Failed to parse refresh response: %1").arg(parseError.errorString());
        return false;
    }

    QJsonObject obj = doc.object();
    newAccessToken = obj["access_token"].toString();
    newRefreshToken = obj["refresh_token"].toString();

    if (obj.contains("expires_at"))
        expiresIn = obj["expires_at"].toInt();
    else if (obj.contains("expires_in"))
        expiresIn = obj["expires_in"].toInt();

    if (newAccessToken.isEmpty()) {
        error = QObject::tr("No access token in refresh response.");
        if (obj.contains("error"))
            error = obj["error_description"].toString();
        return false;
    }

    return true;
}
