/*
 * Copyright (c) 2012 Rainer Clasen <bj@zuto.de>
 * Copyright (c) 2014 Nils Knieling copied from TtbDialog.h
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

#ifndef VELOHERODIALOG_H
#define VELOHERODIALOG_H
#include "GoldenCheetah.h"

#include <QObject>
#include <QtGui>
#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QUrl>
#include <QHttpMultiPart>
#include <QXmlInputSource>
#include <QXmlSimpleReader>
#include <QXmlDefaultHandler>
#include <QNetworkReply>

// QUrl split into QUrlQuerty in QT5
#if QT_VERSION > 0x050000
#include <QUrlQuery>
#endif

#include "Context.h"
#include "RideItem.h"

class VeloHeroDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT

public:
    VeloHeroDialog(Context *context, RideItem *item);

signals:

public slots:
    void uploadToVeloHero();

private slots:
    void uploadProgress(qint64 sent, qint64 total);

    void dispatchReply( QNetworkReply *reply );

    void requestSession();
    void finishSession(QNetworkReply *reply);

    void requestUpload();
    void finishUpload(QNetworkReply *reply);

    void closeClicked();

private:
    enum requestType {
        reqSession,
        reqUpload,
    };

    Context *context;
    RideItem *ride;

    QProgressBar *progressBar;
    QLabel *progressLabel;
    QPushButton *closeButton;

    QNetworkAccessManager networkMgr;
    requestType currentRequest;

    QString sessionId;
    QString exerciseId;

};

#endif // VELOHERODIALOG_H
