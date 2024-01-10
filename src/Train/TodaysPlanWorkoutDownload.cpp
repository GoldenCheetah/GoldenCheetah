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

#include "TodaysPlanWorkoutDownload.h"
#include "MainWindow.h"
#include "TrainDB.h"
#include "HelpWhatsThis.h"
#include "TimeUtils.h"

TodaysPlanWorkoutDownload::TodaysPlanWorkoutDownload(Context *context) : QDialog(context->mainWindow), context(context)
{

    nam = new QNetworkAccessManager(this);
    connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    setWindowTitle(tr("Download future workouts from Today's Plan"));

    // help
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Tools_Download_TP));

    // make the dialog a resonable size
    setMinimumWidth(650);
    setMinimumHeight(400);

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    // TopLine Fields and Layout

    all = new QCheckBox(tr("check/uncheck all"), this);
    all->setChecked(true);

    QLabel *fromLabel = new QLabel(tr("From:"), this);
    QLabel *toLabel = new QLabel(tr("To:"), this);
    from = new QDateEdit(this);
    from->setDate(QDate::currentDate());
    from->setCalendarPopup(true);
    to = new QDateEdit(this);
    to->setDate(QDate::currentDate().addMonths(1));
    to->setCalendarPopup(true);

    refreshButton = new QPushButton(tr("Refresh List"), this);

    QHBoxLayout *topline = new QHBoxLayout;
    topline->addWidget(all);
    topline->addStretch();
    topline->addWidget(fromLabel);
    topline->addWidget(from);
    topline->addWidget(toLabel);
    topline->addWidget(to);
    topline->addStretch();
    topline->addWidget(refreshButton);

    // Filelist Widget

    files = new QTreeWidget;
#ifdef Q_OS_MAC
    files->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    files->headerItem()->setText(0, tr(""));
    files->headerItem()->setText(1, tr("Date"));
    files->headerItem()->setText(2, tr("Name"));
    files->headerItem()->setText(3, tr("T-Score"));
    files->headerItem()->setText(4, tr("Duration"));
    files->headerItem()->setText(5, tr("Action"));

    files->setColumnCount(6);
    files->setColumnWidth(0, 30*dpiXFactor); // selector
    files->setColumnWidth(1, 100*dpiXFactor); // date
    files->setColumnWidth(2, 220*dpiXFactor); // workout description
    files->setColumnWidth(3, 70*dpiXFactor); // T-Score
    files->setColumnWidth(4, 70*dpiXFactor); // duration
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
TodaysPlanWorkoutDownload::allClicked()
{
    // set/uncheck all rides according to the "all"
    bool checked = all->isChecked();

    for(int i=0; i<files->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *current = files->invisibleRootItem()->child(i);
        static_cast<QCheckBox*>(files->itemWidget(current,0))->setChecked(checked);
    }
}

void
TodaysPlanWorkoutDownload::downloadClicked()
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
TodaysPlanWorkoutDownload::cancelClicked()
{
    reject();
}



void
TodaysPlanWorkoutDownload::refreshClicked()
{
    // reset download information
    status->setText("");
    downloads = fails = 0;
    download->show();
    close->show();
    files->clear(); // delete existing entries
    QString error;

    // check period - download is only possible for future workouts !
    if (from->date() < QDate::currentDate()) {
        QMessageBox::warning(this, tr("Today's Plan Workout Download"), QString(tr("You can only download future workouts, 'From' will be adjusted to current date")));
        from->setDate(QDate::currentDate());
    }
    if (from->date() > to->date()) {
        QMessageBox::warning(this, tr("Today's Plan Workout Download"), QString(tr("Invalid date range, please check your input.")));
        return;
    }

    QList<TodaysPlanWorkoutListEntry*> workouts = getFileList(error, from->dateTime(), to->dateTime());
    if (error != "") {
        QMessageBox::warning(this, tr("Today's Plan Workout Download"), QString(tr("The following error occured: %1").arg(error)));
        return;
    }


    foreach(TodaysPlanWorkoutListEntry *item, workouts) {

       QTreeWidgetItem *add = new QTreeWidgetItem(files->invisibleRootItem());
       add->setFlags(add->flags() | Qt::ItemIsEditable);

       // selector
       QCheckBox *checkBox = new QCheckBox("", this);
       checkBox->setChecked(true);
       files->setItemWidget(add, 0, checkBox);

       add->setText(1, item->planDate.toString(tr("dd MMM yyyy")));
       add->setText(2, item->description);
       add->setText(3, QString::number(item->tScore));
       add->setText(4, time_to_string_minutes(item->duration));

       // interval action
       add->setText(5, tr("Download"));

       // hide away the id
       add->setText(6, QString("%1").arg(item->workoutId));
    }
}

void
TodaysPlanWorkoutDownload::downloadFiles()
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
            current->setText(5, tr("Downloading...")); QApplication::processEvents();

            // get the id
            int id = current->text(6).toInt();
            QByteArray content;
            if (!readFile(&content, id)) {
                current->setText(5,tr("Error downloading")); QApplication::processEvents();
                fails++;
                continue;

            }

            QString filename;
            ErgFile *p = ErgFile::fromContent(content, context);
            if (p->Filename != "") {
                filename = workoutDir + "/TP-" + p->Filename.replace("/", "-").simplified();
            } else {
                filename = workoutDir + "/TP-Workout-" + current->text(1).replace(" ", "_") + ".erg";
            }

            // open success?
            if (p->isValid()) {


                if (QFile(filename).exists()) {

                   if (overwrite->isChecked() == false) {
                        // skip existing files
                        current->setText(5,tr("Exists already")); QApplication::processEvents();
                        fails++;
                        delete p; // free memory!
                        continue;

                    } else {

                        // remove existing
                        QFile(filename).remove();
                        current->setText(5, tr("Removing...")); QApplication::processEvents();
                    }

                }

                QFile out(filename);
                if (out.open(QIODevice::WriteOnly) == true) {

                    QTextStream output(&out);
                    output << content;
                    out.close();

                    downloads++;
                    current->setText(5, tr("Saved")); QApplication::processEvents();
                    trainDB->importWorkout(filename, p); // add to library

                } else {

                    fails++;
                    current->setText(5, tr("Write failed")); QApplication::processEvents();
                }

                delete p; // free memory!

            // couldn't parse
            } else {

                delete p; // free memory!
                fails++;
                current->setText(5, tr("Invalid File")); QApplication::processEvents();

            }

        }
    }
    // for library updating, transactional for 10x performance
    trainDB->endLUW();
}

QList<TodaysPlanWorkoutListEntry*>
TodaysPlanWorkoutDownload::getFileList(QString &error, QDateTime from, QDateTime to) {

    QList<TodaysPlanWorkoutListEntry*> returning;

    // do we have a token
    QString token = appsettings->cvalue(context->athlete->cyclist, GC_TODAYSPLAN_TOKEN, "").toString();
    if (token == "") {
        error = tr("You must authorise with Today's Plan first");
        return returning;
    }

    // Do Paginated Access to the Activities List
    const int pageSize = 100;
    int offset = 0;
    int resultCount = INT_MAX;

    while (offset < resultCount) {

        QString url;
        QString searchCommand;
        if (offset == 0) {
            // fist call
            searchCommand = "search";
        } else {
            // subsequent pages
            searchCommand = "page";
        }

        url = QString("%1/rest/users/activities/%2/%3/%4")
                .arg(appsettings->cvalue(context->athlete->cyclist, GC_TODAYSPLAN_URL, "https://whats.todaysplan.com.au").toString())
                .arg(searchCommand)
                .arg(QString::number(offset))
                .arg(QString::number(pageSize));;

        // request using the bearer token
        QNetworkRequest request(url);
        QNetworkReply *reply;
        request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
        if (offset == 0) {

            // Prepare the Search Payload for First Call to Search
            QString userId = appsettings->cvalue(context->athlete->cyclist, GC_TODAYSPLAN_ATHLETE_ID, "").toString();
            // application/json
            QString jsonString;
            jsonString += "{\"criteria\": {";
            if (userId.length()>0)
                jsonString += "\"user\": "+ QString("%1").arg(userId) +", ";
            jsonString += "\"fromTs\": \""+ QString("%1").arg(from.toMSecsSinceEpoch()) +"\", ";
            jsonString += "\"toTs\": \"" + QString("%1").arg(to.addDays(1).addSecs(-1).toMSecsSinceEpoch()) + "\", ";
            jsonString += "\"isNull\": [\"fileId\"]}, ";
            jsonString += "\"order\": [ { \"field\" : \"startTs\",\"order\" : \"asc\" } ], ";
            jsonString += "\"fields\": [\"startTs\", \"user.firstname\",\"name\",\"scheduled.workout\",\"scheduled.time\", \"scheduled.tscorepwr\", \"scheduled.durationSecs\" ], ";
            jsonString += "\"opts\": 1 ";
            jsonString += "}";

            QByteArray jsonStringAsUTF8 = jsonString.toUtf8();

            QByteArray jsonStringDataSize = QByteArray::number(jsonStringAsUTF8.size());

            request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
            request.setRawHeader("Content-Length", jsonStringDataSize);
            reply = nam->post(request, jsonStringAsUTF8);
        } else {
            // get further pages of the Search
            reply = nam->get(request);
        }

        // blocking request
        QEventLoop loop;
        connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();

        // did we get a good response ?
        QByteArray r = reply->readAll();

        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

        // if path was returned all is good, lets set root
        if (parseError.error == QJsonParseError::NoError) {

            // number of Result Items
            if (offset == 0) {
                resultCount = document.object()["cnt"].toInt();
            }

            // results ?
            QJsonObject result = document.object()["result"].toObject();
            QJsonArray results = result["results"].toArray();

            // lets look at that then
            for(int i=0; i<results.size(); i++) {
                QJsonObject each = results.at(i).toObject();
                QJsonObject scheduled = each["scheduled"].toObject();

                // skip "Rest" days
                if (scheduled["workout"].toString() == "rest" && scheduled["tscorepwr"].toDouble() == 0.0) continue;

                // workout details
                TodaysPlanWorkoutListEntry *add = new TodaysPlanWorkoutListEntry();
                add->workoutId = QString("%1").arg(each["workoutId"].toInt());
                add->planDate = QDateTime::fromMSecsSinceEpoch(each["startTs"].toDouble()).date();
                add->duration = scheduled["durationSecs"].toDouble();
                add->tScore = scheduled["tscorepwr"].toDouble();
                add->description = scheduled["_workout-display"].toString() + " : " + each["name"].toString();

                returning << add;
            }
            // next page
            offset += pageSize;
        } else {
            // we had a parsing error - so something is wrong - stop requesting more data by ending the loop
            offset = INT_MAX;
        }
    }

    // check, correct the sort order - TP Api does not consider the requested sort order and may provide
    // the workout latest in future first - since this may change don't sort if order is ok
    // if so, re-sort so that the most recent workout is top of the list
    if (returning.count() > 1) {
        if (returning.first()->planDate > returning.last()->planDate) {
            // re-sort
            int insertAt = returning.count()-1;
            TodaysPlanWorkoutListEntry* entry;

            while (insertAt > 0) {
                entry = returning.takeFirst();
                returning.insert(insertAt, entry);
                insertAt--;
            }
        }
    }

    // all good ?
    return returning;
}

bool
TodaysPlanWorkoutDownload::readFile(QByteArray *data, int workoutId)
{
    // this must be performed asyncronously and call made
    // to notifyReadComplete(QByteArray &data, QString remotename, QString message) when done

    // do we have a token ?
    QString token = appsettings->cvalue(context->athlete->cyclist, GC_TODAYSPLAN_TOKEN, "").toString();
    if (token == "") return false;

    // lets connect and get basic info on the root directory
    QString url = QString("%1/rest/workouts/download/power/erg/%2")
          .arg(appsettings->cvalue(context->athlete->cyclist, GC_TODAYSPLAN_URL, "https://whats.todaysplan.com.au").toString())
          .arg(workoutId);


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
TodaysPlanWorkoutDownload::onSslErrors(QNetworkReply *reply, const QList<QSslError>&)
{
    reply->ignoreSslErrors();
}

