#ifndef AZUM_H
#define AZUM_H

#include "CloudService.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QImage>

class Azum : public CloudService {

    Q_OBJECT

    public:

        Azum(Context *context);
        CloudService *clone(Context *context) { return new Azum(context); }
        ~Azum();

        QString id() const { return "Azum"; }
        QString uiName() const { return tr("Azum"); }
        QString description() const { return (tr("Sync with new and unique coaching platform from Switzerland.")); }
        QImage logo() const { return QImage(":images/services/azum.png"); }

        int capabilities() const { return OAuth | Download | Query; }

        // open/connect and close/disconnect
        bool open(QStringList &errors);
        bool close();

        // home directory
        QString home();

        // create a folder
        bool createFolder(QString);

        // read a file
        bool readFile(QByteArray *data, QString remotename, QString remoteid);

        // athlete selection
        QList<CloudServiceAthlete> listAthletes();
        bool selectAthlete(CloudServiceAthlete);

        // dirent style api
        CloudServiceEntry *root() { return root_; }
        QList<CloudServiceEntry*> readdir(QString path, QStringList &errors, QDateTime from, QDateTime to);

    public slots:
        void readyRead();
        void readFileCompleted();

    private:
        Context *context;
        QNetworkAccessManager *nam;
        QNetworkReply *reply;
        CloudServiceEntry *root_;
        QMap<QNetworkReply*, QByteArray*> buffers;

    private slots:
        void onSslErrors(QNetworkReply *reply, const QList<QSslError>&error);
};

#endif
