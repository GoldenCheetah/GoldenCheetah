/*
 * Copyright (c) 2009 Ned Harding (ned@hardinggroup.com)
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

#ifndef SPLITRIDEDIALOG_H
#define SPLITRIDEDIALOG_H
#include "GoldenCheetah.h"

#include <QtGui>
class MainWindow;
class RideFile;

class SplitRideDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:
        SplitRideDialog(MainWindow *mainWindow);

    private slots:
        void okClicked();
        void cancelClicked();
        void selectionChanged();

    private:

        MainWindow *mainWindow;
        QListWidget *listWidget;
        QPushButton *okButton, *cancelButton;

        void CreateNewRideFile(const RideFile *ride, int nRecStart, int nRecEnd);
};

#endif // SPLITRIDEDIALOG_H
