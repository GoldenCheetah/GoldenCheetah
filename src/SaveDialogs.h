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

#include <QtGui>
#include <QDialog>
#include "RideItem.h"

class MainWindow;

class SaveSingleDialogWidget : public QDialog
{
    Q_OBJECT

    public:
        SaveSingleDialogWidget(QWidget *, RideItem *);
//        QWidget *parent;

    public slots:
        void saveClicked();
        void abandonClicked();
        void cancelClicked();
        void warnSettingClicked();

    private:

        RideItem   *rideItem;
        QPushButton *saveButton, *abandonButton, *cancelButton;
        QCheckBox *warnCheckBox;
        QLabel *warnText;
};

class SaveOnExitDialogWidget : public QDialog
{
    Q_OBJECT

    public:
        SaveOnExitDialogWidget(QWidget *, QList<RideItem*>);

    public slots:
        void saveClicked();
        void abandonClicked();
        void cancelClicked();
        void warnSettingClicked();

    private:
        QList<RideItem *>dirtyList;
        QPushButton *saveButton, *abandonButton, *cancelButton;
        QCheckBox *exitWarnCheckBox;
        QLabel *warnText;

        QTableWidget *dirtyFiles;
};

#endif // _GC_SaveDialogs_h
