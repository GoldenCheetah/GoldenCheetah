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

#ifndef _Gc_TPUpload_h
#define _Gc_TPUpload_h
#include "GoldenCheetah.h"
#include <qtsoap.h>
#include <QString>
#include "RideFile.h"

// uploader to trainingpeaks.com
class TPUpload : public QObject
{
    Q_OBJECT
    G_OBJECT

public:
    TPUpload(QObject *parent = 0);
    int upload(Context *context, const RideFile *ride);

signals:
    void completed(QString);

private slots:
    void getResponse(const QtSoapMessage &);

private:
    QtSoapHttpTransport http;
    bool uploading;
    QtSoapMessage current;
};

#endif
