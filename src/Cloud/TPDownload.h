/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _Gc_Download_h
#define _Gc_Download_h
#include "GoldenCheetah.h"
#include <qtsoap.h>
#include <QString>
#include "RideFile.h"

// get list of accessible athletes
class TPAthlete : public QObject
{
    Q_OBJECT
    G_OBJECT

public:
    TPAthlete(QObject *parent = 0);
    void list(int type, QString user, QString pass);

signals:
    void completed(QString, QList<QMap<QString,QString> >);

private slots:
    void getResponse(const QtSoapMessage &);

private:
    QtSoapHttpTransport http;
    bool waiting;
    QtSoapMessage current;
};

// get workout list
class TPWorkout : public QObject
{
    Q_OBJECT
    G_OBJECT

public:
    TPWorkout(QObject *parent = 0);
    void list(int id, QDate from, QDate to, QString user, QString pass);

signals:
    void completed(QList<QMap<QString,QString> >);

private slots:
    void getResponse(const QtSoapMessage &);

private:
    QtSoapHttpTransport http;
    bool waiting;
    QtSoapMessage current;
};
// uploader to trainingpeaks.com
class TPDownload : public QObject
{
    Q_OBJECT
    G_OBJECT

public:
    TPDownload(QObject *parent = 0);
    void download(QString cyclist, int PersonId, int WorkoutId);

signals:
    void completed(QDomDocument);

private slots:
    void getResponse(const QtSoapMessage &);

private:
    QtSoapHttpTransport http;
    bool downloading;
    QtSoapMessage current;
};
#endif
