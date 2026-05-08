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

#include "TredictWorkoutDownload.h"
#include "MainWindow.h"
#include "TrainDB.h"
#include "Athlete.h"
#include "CloudService.h"
#include "Colors.h"
#include "ErgFile.h"
#include "Secrets.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QEventLoop>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QDateEdit>
#include <QRegularExpression>
#include <QTimeZone>

static const QString TREDICT_URL = "https://www.tredict.com";

TredictWorkoutDownload::TredictWorkoutDownload(Context *context)
    : QDialog(context->mainWindow), context(context)
{
    nam = new QNetworkAccessManager(this);
    connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> &)),
            this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> &)));

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    setWindowTitle(tr("Download planned workouts from Tredict"));

    setMinimumWidth(700);
    setMinimumHeight(450);

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    QHBoxLayout *topline = new QHBoxLayout;
    all = new QCheckBox(tr("check/uncheck all"), this);
    all->setChecked(true);
    topline->addWidget(all);

    topline->addStretch();
    topline->addWidget(new QLabel(tr("From:"), this));
    fromDate = new QDateEdit(QDate::currentDate(), this);
    fromDate->setCalendarPopup(true);
    fromDate->setDisplayFormat("yyyy-MM-dd");
    topline->addWidget(fromDate);

    topline->addWidget(new QLabel(tr("To:"), this));
    toDate = new QDateEdit(QDate::currentDate().addDays(30), this);
    toDate->setCalendarPopup(true);
    toDate->setDisplayFormat("yyyy-MM-dd");
    topline->addWidget(toDate);

    refreshButton = new QPushButton(tr("Refresh"), this);
    topline->addWidget(refreshButton);

    files = new QTreeWidget;
#ifdef Q_OS_MAC
    files->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    files->headerItem()->setText(0, tr(""));
    files->headerItem()->setText(1, tr("Date"));
    files->headerItem()->setText(2, tr("Sport"));
    files->headerItem()->setText(3, tr("Name"));
    files->headerItem()->setText(4, tr("Duration"));
    files->headerItem()->setText(5, tr("Action"));

    files->setColumnCount(6);
    files->setColumnWidth(0, 30*dpiXFactor);
    files->setColumnWidth(1, 100*dpiXFactor);
    files->setColumnWidth(2, 80*dpiXFactor);
    files->setColumnWidth(3, 220*dpiXFactor);
    files->setColumnWidth(4, 80*dpiXFactor);
    files->setSelectionMode(QAbstractItemView::SingleSelection);
    files->setEditTriggers(QAbstractItemView::SelectedClicked);
    files->setUniformRowHeights(true);
    files->setIndentation(0);

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

    connect(download, SIGNAL(clicked()), this, SLOT(downloadClicked()));
    connect(all, SIGNAL(stateChanged(int)), this, SLOT(allClicked()));
    connect(close, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(refreshButton, SIGNAL(clicked()), this, SLOT(refreshClicked()));

    refreshClicked();
}

QString
TredictWorkoutDownload::getToken()
{
    return appsettings->cvalue(context->athlete->cyclist, GC_TREDICT_TOKEN, "").toString();
}

void
TredictWorkoutDownload::onSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    CloudService::sslErrors(context->mainWindow, reply, errors);
}

void
TredictWorkoutDownload::allClicked()
{
    bool checked = all->isChecked();
    for (int i = 0; i < files->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *current = files->invisibleRootItem()->child(i);
        static_cast<QCheckBox*>(files->itemWidget(current, 0))->setChecked(checked);
    }
}

void
TredictWorkoutDownload::cancelClicked()
{
    reject();
}

void
TredictWorkoutDownload::downloadClicked()
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
TredictWorkoutDownload::refreshClicked()
{
    status->setText("");
    downloads = fails = 0;
    download->show();
    close->show();
    files->clear();

    QString error;
    QList<TredictPlannedWorkout> workouts = fetchWorkoutList(error);
    if (!error.isEmpty()) {
        QMessageBox::warning(this, tr("Tredict Workout Download"),
                             QString(tr("Error: %1")).arg(error));
        return;
    }

    foreach (const TredictPlannedWorkout &w, workouts) {
        QTreeWidgetItem *add = new QTreeWidgetItem(files->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        QCheckBox *checkBox = new QCheckBox("", this);
        checkBox->setChecked(true);
        files->setItemWidget(add, 0, checkBox);

        add->setText(1, w.date.toLocalTime().toString("yyyy-MM-dd"));
        add->setText(2, w.sportType);
        add->setText(3, w.title);

        if (w.duration > 0) {
            int mins = (int)(w.duration / 60.0);
            add->setText(4, QString("%1 min").arg(mins));
        }

        add->setText(5, tr("Download"));

        add->setData(0, Qt::UserRole, w.id);
        add->setData(0, Qt::UserRole + 1, w.title);
        add->setData(0, Qt::UserRole + 2, w.duration);
    }
}

QList<TredictPlannedWorkout>
TredictWorkoutDownload::fetchWorkoutList(QString &error)
{
    QList<TredictPlannedWorkout> returning;

    QString token = getToken();
    if (token.isEmpty()) {
        error = tr("You must authorize with Tredict first.");
        return returning;
    }

    QUrl url(TREDICT_URL + "/api/oauth/v2/plannedTrainingList");
    QUrlQuery params;
    params.addQueryItem("startDate",
        QDateTime(fromDate->date(), QTime(0,0,0), QTimeZone::UTC).toString(Qt::ISODate));
    params.addQueryItem("endDate",
        QDateTime(toDate->date(), QTime(23,59,59), QTimeZone::UTC).toString(Qt::ISODate));
    url.setQuery(params);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
    request.setRawHeader("Accept", "application/json;charset=UTF-8");

    QNetworkReply *reply = nam->get(request);

    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        error = reply->errorString();
        reply->deleteLater();
        return returning;
    }

    QByteArray r = reply->readAll();
    reply->deleteLater();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(r, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        error = tr("JSON parser error: %1").arg(parseError.errorString());
        return returning;
    }

    QJsonObject root = doc.object();
    QJsonObject embedded = root["_embedded"].toObject();
    QJsonArray list = embedded["plannedWorkoutList"].toArray();

    for (int i = 0; i < list.size(); i++) {
        QJsonObject each = list.at(i).toObject();
        TredictPlannedWorkout w;
        w.id = each["id"].toString();
        w.title = each["title"].toString();
        w.sportType = each["sportType"].toString();
        w.notes = each["notes"].toString();
        w.date = QDateTime::fromString(each["date"].toString(), Qt::ISODate);
        w.distance = each["distance"].toDouble();
        w.duration = each["duration"].toDouble();
        w.executedTrainingId = each["executedTrainingId"].toString();

        if (w.title.isEmpty())
            w.title = w.sportType;

        returning << w;
    }

    return returning;
}

QByteArray
TredictWorkoutDownload::fetchWorkoutJson(const QString &id, QString &error)
{
    QString token = getToken();
    if (token.isEmpty()) {
        error = tr("Not authorized.");
        return QByteArray();
    }

    QUrl url(QString("%1/api/oauth/v2/plannedTraining/file/json/%2").arg(TREDICT_URL).arg(id));
    QUrlQuery params;
    params.addQueryItem("extraValues", "1");
    params.addQueryItem("language", "en");
    url.setQuery(params);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
    request.setRawHeader("Accept", "application/json;charset=UTF-8");

    QNetworkReply *reply = nam->get(request);

    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        error = reply->errorString();
        reply->deleteLater();
        return QByteArray();
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();
    return data;
}

static double
calcStepDurationMs(const QJsonObject &step)
{
    QString durationType = step["durationType"].toString();
    double durationValue = step["durationValue"].toDouble();

    if (durationType == "TIME") {
        return durationValue * 1000.0;
    } else if (durationType == "DISTANCE") {
        double speed = step.contains("extraValueSpeed") ? step["extraValueSpeed"].toDouble() : 0;
        if (speed > 0)
            return (durationValue / speed) * 1000.0;
        return (durationValue / 4.17) * 1000.0;
    }
    return 0;
}

static void
countStepDurations(const QJsonArray &steps, double &fixedMs, int &openCount)
{
    for (int i = 0; i < steps.size(); i++) {
        QJsonObject step = steps.at(i).toObject();
        QString type = step["type"].toString();

        if (type == "WorkoutRepeatStep") {
            int repeatCount = step["repeatValue"].toInt();
            if (repeatCount < 1) repeatCount = 1;

            double innerFixed = 0;
            int innerOpen = 0;
            countStepDurations(step["steps"].toArray(), innerFixed, innerOpen);
            fixedMs += innerFixed * repeatCount;
            openCount += innerOpen * repeatCount;
        } else if (type == "WorkoutStep") {
            if (step["durationType"].toString() == "OPEN")
                openCount++;
            else
                fixedMs += calcStepDurationMs(step);
        }
    }
}

void
TredictWorkoutDownload::processWorkoutStep(const QJsonObject &step, double &timeMs,
                                            int &lastWatts, QStringList &lines,
                                            QStringList &texts, double openStepMs)
{
    QString durationType = step["durationType"].toString();
    QString targetType = step["targetType"].toString();
    double targetLow = step["targetValueLow"].toDouble();
    double targetHigh = step["targetValueHigh"].toDouble();
    QString progressionType = step["targetProgressionType"].toString();
    QString description = step["description"].toString();

    int extraPower = 0;
    if (step.contains("extraValuePower"))
        extraPower = (int)step["extraValuePower"].toDouble();

    int startWatts, endWatts;

    if (targetType == "POWER") {
        if (progressionType == "ramp") {
            startWatts = (int)targetLow;
            endWatts = (int)targetHigh;
        } else {
            int mid = (int)((targetLow + targetHigh) / 2.0);
            startWatts = endWatts = mid;
        }
    } else if (extraPower > 0) {
        startWatts = endWatts = extraPower;
    } else {
        startWatts = endWatts = (lastWatts > 0 ? lastWatts : 100);
    }

    double durationMs;

    if (durationType == "OPEN") {
        durationMs = openStepMs;
    } else {
        durationMs = calcStepDurationMs(step);
    }

    if (durationMs < 1000.0) durationMs = 1000.0;

    if (!description.isEmpty()) {
        texts << QString("%1\t%2\t%3")
                     .arg((long long)(timeMs / 1000.0))
                     .arg(description)
                     .arg(10);
    }

    lines << QString("%1    %2").arg(timeMs / 60000.0, 0, 'f', 2).arg(startWatts);

    timeMs += durationMs;
    lines << QString("%1    %2").arg(timeMs / 60000.0, 0, 'f', 2).arg(endWatts);

    lastWatts = endWatts;
}

void
TredictWorkoutDownload::linearizeSteps(const QJsonArray &steps, double &timeMs,
                                        int &lastWatts, QStringList &lines,
                                        QStringList &texts, double openStepMs)
{
    for (int i = 0; i < steps.size(); i++) {
        QJsonObject step = steps.at(i).toObject();
        QString type = step["type"].toString();

        if (type == "WorkoutRepeatStep") {
            int repeatCount = step["repeatValue"].toInt();
            if (repeatCount < 1) repeatCount = 1;
            QJsonArray innerSteps = step["steps"].toArray();

            for (int r = 0; r < repeatCount; r++) {
                linearizeSteps(innerSteps, timeMs, lastWatts, lines, texts, openStepMs);
            }
        } else if (type == "WorkoutStep") {
            processWorkoutStep(step, timeMs, lastWatts, lines, texts, openStepMs);
        }
    }
}

QString
TredictWorkoutDownload::convertToErg(const QByteArray &json, const QString &title,
                                      double totalDurationSecs, QString &error)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        error = tr("JSON parser error: %1").arg(parseError.errorString());
        return QString();
    }

    QJsonObject root = doc.object();
    QString workoutName = root["workoutName"].toString();
    if (workoutName.isEmpty()) workoutName = title;
    QString description = root["description"].toString();
    QJsonArray steps = root["steps"].toArray();

    if (steps.isEmpty()) {
        error = tr("Workout has no steps.");
        return QString();
    }

    double fixedMs = 0;
    int openCount = 0;
    countStepDurations(steps, fixedMs, openCount);

    double openStepMs = 300000.0; // default 5 min per open step
    if (openCount > 0 && totalDurationSecs > 0) {
        double totalMs = totalDurationSecs * 1000.0;
        if (totalMs > fixedMs)
            openStepMs = (totalMs - fixedMs) / openCount;
    }

    QStringList dataLines;
    QStringList textLines;
    double timeMs = 0;
    int lastWatts = 0;

    linearizeSteps(steps, timeMs, lastWatts, dataLines, textLines, openStepMs);

    QString erg;
    QTextStream out(&erg);

    out << "[COURSE HEADER]\n";
    out << "SOURCE=Tredict\n";
    out << "DESCRIPTION=" << workoutName << "\n";
    out << "MINUTES WATTS\n";
    out << "[END COURSE HEADER]\n";

    out << "[COURSE DATA]\n";
    foreach (const QString &line, dataLines) {
        out << line << "\n";
    }
    out << "[END COURSE DATA]\n";

    if (!textLines.isEmpty()) {
        out << "[COURSE TEXT]\n";
        foreach (const QString &line, textLines) {
            out << line << "\n";
        }
        out << "[END COURSE TEXT]\n";
    }

    return erg;
}

void
TredictWorkoutDownload::downloadFiles()
{
    QString workoutDir = appsettings->value(this, GC_WORKOUTDIR).toString();

    trainDB->startLUW();

    for (int i = 0; i < files->invisibleRootItem()->childCount(); i++) {
        QApplication::processEvents();

        if (aborted) {
            trainDB->endLUW();
            return;
        }

        QTreeWidgetItem *current = files->invisibleRootItem()->child(i);

        if (!static_cast<QCheckBox*>(files->itemWidget(current, 0))->isChecked())
            continue;

        files->setCurrentItem(current);
        QApplication::processEvents();

        current->setText(5, tr("Downloading..."));
        QApplication::processEvents();

        QString id = current->data(0, Qt::UserRole).toString();
        QString title = current->data(0, Qt::UserRole + 1).toString();
        double totalDuration = current->data(0, Qt::UserRole + 2).toDouble();
        QString date = current->text(1);

        QString error;
        QByteArray json = fetchWorkoutJson(id, error);
        if (json.isEmpty()) {
            current->setText(5, tr("Error: %1").arg(error));
            fails++;
            continue;
        }

        QString ergContent = convertToErg(json, title, totalDuration, error);
        if (ergContent.isEmpty()) {
            current->setText(5, tr("Error: %1").arg(error));
            fails++;
            continue;
        }

        QString safeName = title;
        safeName.replace(QRegularExpression("[^a-zA-Z0-9_\\-\\. ]"), "_");
        if (safeName.isEmpty()) safeName = "workout";

        QString filename = workoutDir + "/" + "Tredict-" + date + "-" + safeName + ".erg";

        if (QFile(filename).exists() && !overwrite->isChecked()) {
            current->setText(5, tr("Exists already"));
            fails++;
            continue;
        }

        QFile out(filename);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
            fails++;
            current->setText(5, tr("Write failed"));
            continue;
        }

        QTextStream stream(&out);
        stream << ergContent;
        out.close();

        ErgFile *p = new ErgFile(filename, ErgFileFormat::erg, context);
        if (p->isValid()) {
            downloads++;
            current->setText(5, tr("Saved"));
            trainDB->importWorkout(filename, *p);
        } else {
            QFile::remove(filename);
            fails++;
            current->setText(5, tr("Invalid workout"));
        }

        delete p;
    }

    trainDB->endLUW();
}
