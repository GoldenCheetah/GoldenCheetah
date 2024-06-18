/*
 * Copyright (c) 2017 Joern Rischmueller (joern.rm@gmail.com)
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

#include "StravaRoutesDownload.h"
#include "MainWindow.h"
#include "TrainDB.h"
#include "HelpWhatsThis.h"
#include "TimeUtils.h"

StravaRoutesDownload::StravaRoutesDownload(Context *context) : QDialog(context->mainWindow), context(context)
{

    nam = new QNetworkAccessManager(this);
    connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    setWindowTitle(tr("Download Routes as workouts from Strava"));

    // help
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Tools_Download_StravaRoutes));

    // make the dialog a resonable size
    setMinimumWidth(650);
    setMinimumHeight(400);

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    // TopLine Fields and Layout

    all = new QCheckBox(tr("check/uncheck all"), this);
    all->setChecked(true);

    refreshButton = new QPushButton(tr("Refresh List"), this);

    QHBoxLayout *topline = new QHBoxLayout;
    topline->addWidget(all);
    topline->addStretch();
    topline->addStretch();
    topline->addWidget(refreshButton);

    // Filelist Widget

    files = new QTreeWidget;
#ifdef Q_OS_MAC
    files->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    files->headerItem()->setText(0, tr(""));
    files->headerItem()->setText(1, tr("Name"));
    files->headerItem()->setText(2, tr("Description"));
    files->headerItem()->setText(3, tr("Action"));

    files->setColumnCount(4);
    files->setColumnWidth(0, 30*dpiXFactor); // selector
    files->setColumnWidth(1, 220*dpiXFactor); // name
    files->setColumnWidth(2, 240*dpiXFactor); // description
    files->setSelectionMode(QAbstractItemView::SingleSelection);
    files->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    files->setUniformRowHeights(true);
    files->setIndentation(0);

    // BottomLine Fields and Layout
    QHBoxLayout *bottomLine = new QHBoxLayout;
    status = new QLabel("", this);
    status->hide();
    overwrite = new QCheckBox(tr("Overwrite existing workouts"), this);
    close = new QPushButton(tr("Close"), this);
    download = new QPushButton(tr("Download"), this);
    bottomLine->addWidget(overwrite);
    bottomLine->addWidget(status);
    bottomLine->addStretch();
    bottomLine->addWidget(close);
    bottomLine->addWidget(download);

    layout->addLayout(topline);
    layout->addWidget(files);
    layout->addLayout(bottomLine);

    // connect signals and slots up..
    connect(download, SIGNAL(clicked()), this, SLOT(downloadClicked()));
    connect(all, SIGNAL(stateChanged(int)), this, SLOT(allClicked()));
    connect(close, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(refreshButton, SIGNAL(clicked()), this, SLOT(refreshClicked()));

    // fill the data
    refreshClicked();

}

void
StravaRoutesDownload::allClicked()
{
    // set/uncheck all rides according to the "all"
    bool checked = all->isChecked();

    for(int i=0; i<files->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *current = files->invisibleRootItem()->child(i);
        static_cast<QCheckBox*>(files->itemWidget(current,0))->setChecked(checked);
    }
}

void
StravaRoutesDownload::downloadClicked()
{
    if (download->text() == tr("Download")) {
        downloads = fails = 0;
        aborted = false;
        refreshButton->setEnabled(false);
        overwrite->hide();
        status->setText(tr("Download..."));
        status->show();
        close->hide();
        download->setText(tr("Abort"));
        downloadFiles();
        status->setText(QString(tr("%1 workouts downloaded, %2 failed or skipped.")).arg(downloads).arg(fails));
        download->setText(tr("Download"));
        close->show();
        overwrite->show();
        refreshButton->setEnabled(true);

        context->notifyWorkoutsChanged();

    } else if (download->text() == tr("Abort")) {
        aborted = true;
    }
}

void
StravaRoutesDownload::cancelClicked()
{
    reject();
}



void
StravaRoutesDownload::refreshClicked()
{
    // reset download information
    status->setText("");
    downloads = fails = 0;
    download->show();
    close->show();
    files->clear(); // delete existing entries
    QString error;

    QList<StravaRoutesListEntry*> routes = getFileList(error);
    if (error != "") {
        QMessageBox::warning(this, tr("Today's Plan Workout Download"), QString(tr("The following error occured: %1").arg(error)));
        return;
    }


    foreach(StravaRoutesListEntry *item, routes) {

       QTreeWidgetItem *add = new QTreeWidgetItem(files->invisibleRootItem());
       add->setFlags(add->flags() | Qt::ItemIsEditable);

       // selector
       QCheckBox *checkBox = new QCheckBox("", this);
       checkBox->setChecked(true);
       files->setItemWidget(add, 0, checkBox);

       add->setText(1, item->name);
       add->setText(2, item->description);

       // interval action
       add->setText(3, tr("Download"));

       // hide away the id
       add->setText(4, QString("%1").arg(item->routeId));
    }
}

void
StravaRoutesDownload::downloadFiles()
{
    // where to place them
    QString workoutDir = appsettings->value(this, GC_WORKOUTDIR).toString();

    // for library updating, transactional for 10x performance
    trainDB->startLUW();

    // loop through the table and export all selected
    for(int i=0; i<files->invisibleRootItem()->childCount(); i++) {

        // give user a chance to abort..
        QApplication::processEvents();

        // did they?
        if (aborted == true) {
            trainDB->endLUW(); // need to commit whatever was copied.
            return; // user aborted!
        }

        QTreeWidgetItem *current = files->invisibleRootItem()->child(i);

        // is it selected
        if (static_cast<QCheckBox*>(files->itemWidget(current,0))->isChecked()) {

            files->setCurrentItem(current); QApplication::processEvents();

            // this one then
            current->setText(3, tr("Downloading...")); QApplication::processEvents();

            // get the id
            int id = current->text(4).toInt();
            QByteArray content;
            if (!readFile(&content, id)) {
                current->setText(3,tr("Error downloading")); QApplication::processEvents();
                fails++;
                continue;

            }

            // Slightly involved filename
            // - it is sourced from a Strava Route, hence the prefix
            // - Strava allows multiple Routes to be identically named, hence inclusion of id for total unambiguity
            QString file_basename = "Strava-Route-" + current->text(4) + "-" + current->text(1).replace(" ", "_") + ".gpx";
            QString filename = workoutDir + "/" + file_basename;

            // create a temporary file
            QString tmp = context->athlete->home->temp().absolutePath() + "/" + file_basename;

            QFile ufile(tmp); // look at uncompressed version mot the source
            ufile.open(QFile::ReadWrite);
            ufile.write(content);
            ufile.close();

            ErgFile *p = new ErgFile(tmp, ErgFileFormat::crs, context);

            // now zap the temporary file
            ufile.remove();

            // open success?
            if (p->isValid()) {


                if (QFile(filename).exists()) {

                   if (overwrite->isChecked() == false) {
                        // skip existing files
                        current->setText(3,tr("Exists already")); QApplication::processEvents();
                        fails++;
                        delete p; // free memory!
                        continue;

                    } else {

                        // remove existing
                        QFile(filename).remove();
                        current->setText(3, tr("Removing...")); QApplication::processEvents();
                    }

                }

                QFile out(filename);
                if (out.open(QIODevice::WriteOnly) == true) {

                    QTextStream output(&out);
                    output << content;
                    out.close();

                    downloads++;
                    current->setText(3, tr("Saved")); QApplication::processEvents();
                    trainDB->importWorkout(filename, *p); // add to library

                } else {

                    fails++;
                    current->setText(3, tr("Write failed")); QApplication::processEvents();
                }

                delete p; // free memory!

            // couldn't parse
            } else {

                delete p; // free memory!
                fails++;
                current->setText(3, tr("Invalid File")); QApplication::processEvents();

            }

        }
    }
    // for library updating, transactional for 10x performance
    trainDB->endLUW();
}

QString
StravaRoutesDownload::getAthleteId(QString token)
{
    QString urlstr = "https://www.strava.com/api/v3/athlete";
    QUrl url = QUrl( urlstr );

    // request using the bearer token
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());

    QNetworkReply *reply = nam->get(request);

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "error" << reply->errorString();
        return "";
    }

    // did we get a good response ?
    QByteArray r = reply->readAll();
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return "";
    }

    // result
    return QString::number(document.object()["id"].toDouble(), 'f', 0);
}

QList<StravaRoutesListEntry*>
StravaRoutesDownload::getFileList(QString &error)
{
    QList<StravaRoutesListEntry*> returning;

    // do we have a token
    QString token = appsettings->cvalue(context->athlete->cyclist, GC_STRAVA_TOKEN, "").toString();
    if (token == "") {
        error = tr("You must authorise with Strava first");
        return returning;
    }

    // Get the authenticated athlete id
    QString athleteId = getAthleteId(token);
    if (athleteId.size() == 0) {
        error = tr("Unable to determine authenticated user id");
        return returning;
    }

    // Do Paginated Access to the Routes List
    const int pageSize = 100;
    int offset = 0;
    int resultCount = INT_MAX;

    while (offset < resultCount) {
        QString urlstr = "https://www.strava.com/api/v3/athletes/" + athleteId + "/routes?";

        QUrlQuery params;

        params.addQueryItem("per_page", QString("%1").arg(pageSize));
        params.addQueryItem("page",  QString("%1").arg(offset/pageSize+1));

        QUrl url = QUrl( urlstr + params.toString() );
        //printd("URL used: %s\n", url.url().toStdString().c_str());

        // request using the bearer token
        QNetworkRequest request(url);
        request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());

        QNetworkReply *reply = nam->get(request);

        // blocking request
        QEventLoop loop;
        connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();

        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "error" << reply->errorString();
            error = tr("Network Problem reading Strava data");
            //return returning;
        }
        // did we get a good response ?
        QByteArray r = reply->readAll();
        //printd("response: %s\n", r.toStdString().c_str());

        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

        // if path was returned all is good, lets set root
        if (parseError.error == QJsonParseError::NoError) {

            // results ?
            QJsonArray results = document.array();

            // lets look at that then
            if (results.size()>0) {
                for(int i=0; i<results.size(); i++) {
                    QJsonObject each = results.at(i).toObject();
                    StravaRoutesListEntry *add = new StravaRoutesListEntry();

                    add->routeId = QString("%1").arg(each["id"].toVariant().toULongLong());
                    add->name = each["name"].toString();
                    add->description = each["description"].toString();

                    //printd("direntry: %s %s\n", add->routeId.toStdString().c_str(), add->name.toStdString().c_str());

                    returning << add;
                }
                // next page
                offset += pageSize;
            } else
                offset = INT_MAX;

        } else {
            // we had a parsing error - so something is wrong - stop requesting more data by ending the loop
            offset = INT_MAX;
        }
    }

    // all good ?
    return returning;
}

bool
StravaRoutesDownload::readFile(QByteArray *data, int routeId)
{
    // this must be performed asyncronously and call made
    // to notifyReadComplete(QByteArray &data, QString remotename, QString message) when done

    // do we have a token ?
    QString token = appsettings->cvalue(context->athlete->cyclist, GC_STRAVA_TOKEN, "").toString();
    if (token == "") return false;

    // lets connect and get basic info on the root directory
    QString url = QString("https://www.strava.com/api/v3/routes/%1/export_gpx")
          .arg(routeId);

    //printd("url:%s\n", url.toStdString().c_str());

    // request using the bearer token
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());

    // put the file
    QNetworkReply *reply = nam->get(request);

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        return false;
    };
    // did we get a good response ?
    *data = reply->readAll();
    return true;
}


void
StravaRoutesDownload::onSslErrors(QNetworkReply *reply, const QList<QSslError>&)
{
    reply->ignoreSslErrors();
}

