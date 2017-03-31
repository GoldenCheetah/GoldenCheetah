/*
 * Copyright (c) 2017 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _AddCloudWizard_h
#define _AddCloudWizard_h

#include "GoldenCheetah.h"
#include "Context.h"
#include "CloudService.h"
#include "Serial.h"
#include "Settings.h"

#include <QWizard>
#include <QFormLayout>
#include <QHeaderView>
#include <QProgressBar>
#include <QFileDialog>
#include <QCommandLinkButton>
#include <QScrollArea>

class AddCloudWizard : public QWizard
{
    Q_OBJECT

public:
    AddCloudWizard(Context *context);
    QSize sizeHint() const { return QSize(600,650); }

    Context *context;
    bool done; // have we finished?

    // what type of services did we say we wanted
    int type;

    // which service have we selected?
    QString service;

    // this is cloned for our context
    CloudService *cloudService;

    // settings we are currently updating - initially set
    // from the current values, but modified by the user
    // as they work through the wizard
    QHash<QString, QString> settings;

public slots:

signals:

private slots:

};

class AddClass : public QWizardPage
{
    Q_OBJECT

    public:
        AddClass(AddCloudWizard *);
        bool validate() const { return false; }
        bool isComplete() const { return false; }
        int nextId() const { return 10; }

    public slots:
        void clicked(int);

    private:
        AddCloudWizard *wizard;
        QSignalMapper *mapper;
};

class AddService : public QWizardPage
{
    Q_OBJECT

    public:
        AddService(AddCloudWizard *);
        void initializePage();
        bool validate() const { return false; }
        bool isComplete() const { return false; }
        int nextId() const { return 20; }

    public slots:
        void clicked(QString);

    private:
        AddCloudWizard *wizard;
        QSignalMapper *mapper;
        QWidget *buttons;
        QVBoxLayout *buttonlayout;
        QScrollArea *scrollarea;
};

class AddAuth : public QWizardPage
{
    Q_OBJECT

    public:
        AddAuth(AddCloudWizard *);

    public slots:
        void initializePage();
        bool validatePage();
        int nextId() const { return 30; } //XXX might go to end if no settings left to configure

        //void doAuth();

    private:
        AddCloudWizard *wizard;

        // all laid out in a formlayout of rows
        QLabel *urlLabel;
        QLineEdit *url;
        QLabel *keyLabel;
        QLineEdit *key;
        QLabel *userLabel;
        QLineEdit *user;
        QLabel *passLabel;
        QLineEdit *pass;
        QLabel *authLabel;
        QPushButton *auth;
        QLabel *tokenLabel;
        QLabel *token;
};

class AddSettings : public QWizardPage
{
    Q_OBJECT

    public:
        AddSettings(AddCloudWizard *);
        void initializePage();
        bool validatePage(); //XXX needs to save away entered values
        int nextId() const { return 90; }

    public slots:
        void browseFolder();

    private:
        AddCloudWizard *wizard;
        QLabel *folderLabel;
        QLineEdit *folder;
        QPushButton *browse;
        QCheckBox *syncStartup, *syncImport;
};

class AddFinish : public QWizardPage
{
    Q_OBJECT

    public:
        AddFinish(AddCloudWizard *);
        void initializePage();
        bool validatePage();
        bool isCommitPage() { return true; }

    private:
        AddCloudWizard *wizard;
        QFormLayout *layout;

};

#endif // _AddCloudWizard_h
