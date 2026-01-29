/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_SaveDialogs_h
#define _GC_SaveDialogs_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <QDialog>
#include <QCheckBox>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>

#include "RideCache.h"
#include "RideItem.h"
#include "Context.h"

class MainWindow;

class SaveSingleDialogWidget : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:
        SaveSingleDialogWidget(MainWindow *, Context *context, RideItem *);

    public slots:
        void saveClicked();
        void abandonClicked();
        void cancelClicked();
        void warnSettingClicked();

    private:

        MainWindow *mainWindow;
        Context *context;
        RideItem   *rideItem;
        QPushButton *saveButton, *abandonButton, *cancelButton;
        QCheckBox *warnCheckBox;
        QLabel *warnText;
};

class SaveOnExitDialogWidget : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:
        SaveOnExitDialogWidget(MainWindow *, Context *context, QList<RideItem*>);

    public slots:
        void saveClicked();
        void abandonClicked();
        void cancelClicked();
        void warnSettingClicked();

    private:
        MainWindow *mainWindow;
        Context *context;
        QList<RideItem *>dirtyList;
        QPushButton *saveButton, *abandonButton, *cancelButton;
        QCheckBox *exitWarnCheckBox;
        QLabel *warnText;

        QTableWidget *dirtyFiles;
};


extern bool proceedDialog(Context *context, const RideCache::OperationPreCheck &check);
extern void relinkRideItems(Context *context, RideItem *rideItem, QList<RideItem*> &activities);


#endif // _GC_SaveDialogs_h
