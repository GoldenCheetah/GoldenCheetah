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

#ifndef GC_Wahoo_h
#define GC_Wahoo_h

#include <QObject>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QSslError>
#include <QList>
#include <QMap>
#include <QImage>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "Context.h"
#include "CloudService.h"
#include "WahooClient.h"

/*
*   Wahoo Cloud access is implemented through Wahoo, which depends on CloudService
*   CloudService pulls in a lot of dependencies, thus unit testing Wahoo directly is
*   practically impossible.
*   Therefore another class WahooClient is responsible for the API interactions.
*   It can be tested using a mock QNetworkReply class.
*/
class Wahoo : public CloudService {
    Q_OBJECT

    public:

        QString id() const override { return "Wahoo"; }
        QString uiName() const override { return tr("Wahoo"); }
        QString description() const override { return (tr("Sync workouts with Wahoo cloud service.")); }
        QImage logo() const override;

        Wahoo(Context *context);
        CloudService *clone(Context *context) override { return new Wahoo(context); }
        ~Wahoo() = default;

        int capabilities() const override { return OAuth | Download | Query; }
        int type() const override { return Activities; }

        bool open(QStringList &errors) override;
        bool close() override;

        bool readFile(QByteArray *data, QString remotename, QString remoteid) override;

        QList<CloudServiceEntry*> readdir(QString path, QStringList &errors, QDateTime from, QDateTime to) override;

    public slots:

        void readyRead();
        void readFileCompleted();

    private slots:
        void onSslErrors(QNetworkReply *reply, const QList<QSslError>&error);

    private:
        // Has a QMainWindow as parent, so Qt takes care of the deletion
        Context *context;

        // Initialized with parent(this) in the constructor
        // so Qt will delete it
        QNetworkAccessManager *nam = nullptr;

        // To allow unit testing without depending on CloudService,
        // which otherwise would pull in a lot of GoldenCheetah classes
        // that would have to be mocked, use a seperate class that interacts
        // with the API
        WahooClient client_;

        // QNetworkReply* is released in Wahoo::readFileCompleted
        // QByteArray* is released in CloudServiceAutoDownload::readComplete
        QMap<QNetworkReply*, QByteArray*> buffers;
};

#endif // GC_Wahoo_h