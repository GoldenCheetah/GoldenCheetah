/*
 * Copyright (c) 2020 Mark Liversedge (liversedge@gmail.com)
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

#ifndef ATHLETECONFIGDIALOG_H
#define ATHLETECONFIGDIALOG_H
#include "GoldenCheetah.h"

#include <QDialog>
#include <QSettings>
#include <QMainWindow>
#include <QStackedWidget>

#include "AthletePages.h"
#include "Context.h"


// ATHLETE PAGE
class AthleteConfig : public QWidget
{
    Q_OBJECT

    public:
        AthleteConfig(QDir home, Context *context);
        AboutRiderPage *athletePage;
        AboutModelPage *modelPage;
        CredentialsPage *credentialsPage;
        QVector<MeasuresPage*> measuresPages;

    public slots:
        qint32 saveClicked();
    
    private:
        QDir home;
        Context *context;

        // about me, power ones and hr zones
        ZonePage *zonePage;
        HrZonePage *hrZonePage;
        PaceZonePage *paceZonePage;
        AutoImportPage *autoImportPage;
        BackupPage *backupPage;
};

class AthleteConfigDialog : public QDialog
{
    Q_OBJECT

    public:
        AthleteConfigDialog(QDir home, Context *context);
        ~AthleteConfigDialog();

    public slots:
        void saveClicked(); // collects state
        void closeClicked();

    private:
        QSettings *settings;
        QDir home;
        Context *context;

        QPushButton *saveButton;
	    QPushButton *closeButton;

        // the config pages
        AthleteConfig *athlete;
};
#endif
