/*
* Copyright (c) 2026 Simon-Friedrich Böttger (simonboettger (at) googlemail.com)
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

#ifndef GC_WahooClient_h
#define GC_WahooClient_h

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QMap>
#include <QDateTime>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSharedPointer>
#include <QLoggingCategory>

// Declare a logging category gcWahoo
Q_DECLARE_LOGGING_CATEGORY(gcWahoo)

#ifdef Q_CC_MSVC
#define printd(fmt, ...) qCDebug(gcWahoo, fmt, __VA_ARGS__);
#else
#define printd(fmt, args...) qCDebug(gcWahoo, fmt, ##args);
#endif

inline constexpr QStringView WAHOO_URL = u"https://api.wahooligan.com";

// This makes including CloudService.h obsolete,
// thus avoiding dependencies which cause trouble in testing
struct WahooWorkoutEntry {
    QString id;
    QString label;
    QString name;
    double distance = 0.0; // km
    double duration = 0.0; // seconds
    bool isDir = false;
};

QJsonObject network2Json(const QByteArray &replyData, QStringList &errors);

/*
*   This is responsible for the interaction with the Wahoo Cloud API
*   The license of the Wahoo Cloud API can be found at https://www.wahoofitness.com/wahoo-api-agreement
*   The documentation of the Wahoo Cloud API is at https://cloud-api.wahooligan.com
*   The network functionality is in protected functions, therefore allowing them to be overriden
*   in derived test classes.
*/
class WahooClient {
    public:
        explicit WahooClient(QNetworkAccessManager *nam = nullptr) : nam_(nam) {}
        virtual ~WahooClient() = default;

        void setNam(QNetworkAccessManager *nam) { nam_ = nam; }

        QList<WahooWorkoutEntry> fetchWorkouts(const QString &token,
                                              QStringList &errors,
                                              const QDateTime &from,
                                              const QDateTime &to);

        // Resolves workout_summary to get file download url and returns the active QNetworkReply* for download
        QNetworkReply *startFileDownload(const QString &token,
                                         const QString &remoteId,
                                         QStringList &errors);

        QNetworkReply *sendFile(const QString &token,
                                QByteArray &data,
                                const QString &remotename,
                                const QString &remoteid);

        QString checkUpload(QNetworkReply *reply, const QString &token);

    protected:
        virtual QNetworkReply *namGet(QNetworkRequest &request)
        {
            return nam_ != nullptr ? nam_->get(request) : nullptr;
        }

        virtual QNetworkReply *namPost(QNetworkRequest &request, QByteArray &body)
        {
            return nam_ != nullptr ? nam_->post(request, body) : nullptr;
        }

        virtual QByteArray getNetworkResponse(QUrl url,
                                              const QString &token,
                                              QStringList &errors,
                                              QUrlQuery params = {});

    private:
        QNetworkAccessManager *nam_ = nullptr;
};

#endif // GC_WahooClient_h
