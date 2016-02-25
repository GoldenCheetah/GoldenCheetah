/*
 * Copyright (c) 2011 Damien.Grauser (damien.grauser@pev-geneve.ch)
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

#ifndef TRAININGSTAGEBUCHUPLOADER_H
#define TRAININGSTAGEBUCHUPLOADER_H
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
#include "ShareDialog.h"

class TrainingstagebuchUploader : public ShareDialogUploader
{
    Q_OBJECT
    G_OBJECT

public:
    TrainingstagebuchUploader(Context *context, RideItem *item, ShareDialog *parent = 0);

    virtual void upload();

    virtual bool canUpload( QString &err );
    virtual bool wasUploaded();

private slots:
    void dispatchReply( QNetworkReply *reply );

    void requestSettings();
    void finishSettings(QNetworkReply *reply);

    void requestSession();
    void finishSession(QNetworkReply *reply);

    void requestUpload();
    void finishUpload(QNetworkReply *reply);

private:
    enum requestType {
        reqSettings,
        reqSession,
        reqUpload,
    };

    QEventLoop eventLoop;
    QNetworkAccessManager networkMgr;
    requestType currentRequest;

    bool proMember;
    QString sessionId;
    QString exerciseId;
    bool uploadSuccessful;

};

#endif // TRAININGSTAGEBUCHUPLOADER_H
