/*
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
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

#ifndef _GC_ChooseCyclistDialog_h
#define _GC_ChooseCyclistDialog_h 1
#include "GoldenCheetah.h"

#include <QDialog>
#include <QDir>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QDialog>

class QListWidget;
class QListWidgetItem;
class QPushButton;

class ChooseCyclistDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:
        ChooseCyclistDialog(const QDir &home, bool allowNew);
        QString choice();
        void getList();
        static QString newCyclistDialog(QDir &homeDir, QWidget *parent);
        static bool deleteAthlete(QDir &homeDir, QString name, QWidget *parent);

    private slots:
        void newClicked();
        void cancelClicked();
        void enableOkDelete(QListWidgetItem *item);
        void deleteClicked();

    private:

        QDir home;
        QListWidget *listWidget;
        QPushButton *okButton, *newButton, *cancelButton, *deleteButton;
};

#endif // _GC_ChooseCyclistDialog_h

