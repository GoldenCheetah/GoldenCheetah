
/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _TrainerDayDownloadDialog_h
#define _TrainerDayDownloadDialog_h
#include "GoldenCheetah.h"
#include "Context.h"
#include "Settings.h"
#include "Units.h"

#include "TrainerDay.h" // for interacting with the TrainerDay site
#include "ErgFile.h" // for interacting with the TrainerDay site

#include "TrainSidebar.h"

#include <QtGui>
#include <QTableWidget>
#include <QProgressBar>
#include <QList>
#include <QLabel>
#include <QListIterator>
#include <QDebug>

// Dialog class to show filenames, import progress and to capture user input
// of ride date and time

class TrainerDayDownloadDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


public:
    TrainerDayDownloadDialog(Context *context);

    QTreeWidget *files; // choose files to export

signals:

private slots:
    void cancelClicked();
    void okClicked();
    void downloadFiles();
    void allClicked();

private:
    Context *context;
    bool aborted;

    QCheckBox *all;

    QCheckBox *overwrite;
    QPushButton *cancel, *ok;

    int downloads, fails;
    QLabel *status;

    TrainerDay ergdb;
};
#endif // _TrainerDayDownloadDialog_h

