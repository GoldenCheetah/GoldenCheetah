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
#include "TPUploadDialog.h"
#include "Context.h"
#include "GcRideFile.h"
#include "RideItem.h"
#include "RideFile.h"
#include "TPUpload.h"
#include "Settings.h"

TPUploadDialog::TPUploadDialog(QString cyclist, const RideFile *ride, Context *context) :
context(context), cyclist(cyclist), ride(ride)
{
    setWindowTitle(tr("Upload to TrainingPeaks.com"));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *button = new QHBoxLayout;

    statusLabel = new QLabel(tr("Upload in progress..."));
    statusLabel->setWordWrap(true);

    progressBar = new QProgressBar(this);
    progressBar->setMinimum(0);
    progressBar->setMaximum(0);

    cancelButton = new QPushButton(tr("Abort"));

    button->addStretch();
    button->addWidget(cancelButton);
    button->addStretch();

    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(progressBar);
    mainLayout->addLayout(button);

    uploader = new TPUpload(this);

    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(uploader, SIGNAL(completed(QString)),this, SLOT(completed(QString)));

    uploading = true;
    int size = uploader->upload(context, ride);

    statusLabel->setText(QString(tr("Uploading (%1 bytes)...")).arg(size));

    setMinimumWidth(250);
}

void
TPUploadDialog::cancelClicked()
{
    delete uploader;
    reject();
}

void
TPUploadDialog::completed(QString results)
{
    uploading = false;
    progressBar->setMaximum(1);
    progressBar->setValue(1);
    statusLabel->setText(results);
    cancelButton->setText(tr("Close"));
}
