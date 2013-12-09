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

#ifndef _GC_TPUploadDialog_h
#define _GC_TPUploadDialog_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include "RideFile.h"
#include "TPUpload.h"

class Context;

class TPUploadDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:
        TPUploadDialog(QString cyclist, const RideFile *ride, Context *context);

    public slots:
        void cancelClicked();
        void completed(QString);

    private:
        Context *context;
        QString cyclist;
        const RideFile *ride;

        QLabel *statusLabel;
        QProgressBar *progressBar;
        QPushButton *cancelButton;
        bool uploading;

        TPUpload *uploader;
};

#endif // _GC_TPUploadDialog_h
