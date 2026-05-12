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

#ifndef GC_TredictWorkoutDownload_h
#define GC_TredictWorkoutDownload_h

#include "GoldenCheetah.h"
#include "Context.h"
#include "Settings.h"

#include <QtGui>
#include <QTreeWidget>
#include <QLabel>
#include <QDateEdit>
#include <QSslError>

class QNetworkAccessManager;
class QNetworkReply;

struct TredictPlannedWorkout {
    QString id;
    QString title;
    QString sportType;
    QString notes;
    QDateTime date;
    double distance;
    double duration;
    QString executedTrainingId;
};

class TredictWorkoutDownload : public QDialog
{
    Q_OBJECT
    G_OBJECT

public:
    TredictWorkoutDownload(Context *context);

private slots:
    void cancelClicked();
    void downloadClicked();
    void allClicked();
    void refreshClicked();
    void onSslErrors(QNetworkReply *reply, const QList<QSslError> &error);

private:
    QString getToken();
    QList<TredictPlannedWorkout> fetchWorkoutList(QString &error);
    QByteArray fetchWorkoutJson(const QString &id, QString &error);
    QString convertToErg(const QByteArray &json, const QString &title,
                         double totalDurationSecs, QString &error);
    void downloadFiles();

    void linearizeSteps(const QJsonArray &steps, double &timeMs, int &lastWatts,
                        QStringList &lines, QStringList &texts, double openStepMs);
    void processWorkoutStep(const QJsonObject &step, double &timeMs, int &lastWatts,
                            QStringList &lines, QStringList &texts, double openStepMs);

    Context *context;
    QNetworkAccessManager *nam;
    bool aborted;

    QTreeWidget *files;
    QCheckBox *all;
    QCheckBox *overwrite;
    QPushButton *close, *download, *refreshButton;
    QDateEdit *fromDate, *toDate;
    QLabel *status;

    int downloads, fails;
};

#endif
