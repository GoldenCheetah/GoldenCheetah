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

#ifndef _AddDeviceWizard_h
#define _AddDeviceWizard_h

#include "GoldenCheetah.h"
#include "Context.h"
#include "DeviceTypes.h"
#include "Serial.h"
#include "RealtimeController.h"
#ifdef GC_HAVE_WFAPI
#include "KickrController.h"
#include "BT40Controller.h"
#endif
#ifdef GC_HAVE_LIBUSB
#include "FortiusController.h"
#endif
#include "ComputrainerController.h"
#if QT_VERSION >= 0x050000
#include "MonarkController.h"
#endif
#include "ANTlocalController.h"
#include "ANTChannel.h"
#include "NullController.h"
#include "Settings.h"

#include <QWizard>
#include <QFormLayout>
#include <QHeaderView>
#include <QProgressBar>
#include <QFileDialog>
#include <QCommandLinkButton>

class DeviceScanner;

class AddDeviceWizard : public QWizard
{
    Q_OBJECT

public:
    AddDeviceWizard(Context *context);

    Context *context;
    bool done; // have we finished?
    RealtimeController *controller; // for working with devices

    // supported device types
    DeviceTypes deviceTypes;
    int type;                   // what are we in the process of adding?
    int current;                // index into deviceTypes of current
    bool found;                 // we found one!
    QString portSpec;           // where did we find it?
    QString profile;

    DeviceScanner *scanner;

public slots:

signals:

private slots:

};

class AddType : public QWizardPage
{
    Q_OBJECT

    public:
        AddType(AddDeviceWizard *);
        void initializePage();
        bool validate() const { return false; }
        bool isComplete() const { return false; }
        int nextId() const { return next; }

    public slots:
        void clicked(QString);

    private:
        AddDeviceWizard *wizard;
        QSignalMapper *mapper;
        QLabel *label;
        int next;
};

class AddSearch : public QWizardPage
{
    Q_OBJECT

    public:
        AddSearch(AddDeviceWizard *);

    public slots:
        void initializePage();
        int nextId() const;
        bool validatePage();
        void doScan();
        void scanFinished(bool);
        void cleanupPage();
        void chooseCOMPort();

    private:
        bool isSearching, active;
        AddDeviceWizard *wizard;
        QProgressBar *bar;
        QPushButton *stop;
        QComboBox *manual;
        QLabel *label, *label1, *label2;
        QString specified;
};

class AddFirmware : public QWizardPage
{
    Q_OBJECT

    public:
        AddFirmware(AddDeviceWizard *);
        bool validatePage();
        int nextId() const { return 60; }

    public slots:
        void browseClicked();

    private:
        QCheckBox *copy;
        QPushButton *ok, *cancel;
        QPushButton *browse;
        QLabel *help;
        QLabel *file;
        QLineEdit *name;
        Context *context;
        AddDeviceWizard *wizard;
};

class AddPair : public QWizardPage
{
    Q_OBJECT

    public:
        AddPair(AddDeviceWizard *);
        int nextId() const { return 60; }
        void initializePage();
        bool validatePage();
        void cleanupPage();

    public slots:

        void getChannelValues();
        // we found a device on a channel
        void channelInfo(int channel, int device_number, int device_id);
        // we failed to find a device on the channel
        void searchTimeout(int channel);

        // user interactions
        void sensorChanged(int channel); // sensor selection changed
    private:
        AddDeviceWizard *wizard;
        QTreeWidget *channelWidget;
        QSignalMapper *signalMapper;
        QTimer updateValues;
        QString cyclist;
};

class AddPairBTLE : public QWizardPage
{
    Q_OBJECT

    public:
        AddPairBTLE(AddDeviceWizard *);
        int nextId() const { return 60; }
        void initializePage();
        bool validatePage();
        void cleanupPage();

    public slots:

        void getChannelValues();
        // we found a device on a channel
        void channelInfo(int channel, int device_number, int device_id);
        // we failed to find a device on the channel
        void searchTimeout(int channel);

        // user interactions
        void sensorChanged(int channel); // sensor selection changed
    private:
        AddDeviceWizard *wizard;
        QTreeWidget *channelWidget;
        QSignalMapper *signalMapper;
        QTimer updateValues;
        QString cyclist;

};

class AddFinal : public QWizardPage
{
    Q_OBJECT

    public:
        AddFinal(AddDeviceWizard *);
        void initializePage();
        bool validatePage();
        bool isCommitPage() { return true; }

    private:
        AddDeviceWizard *wizard;

        QLineEdit *name;
        QComboBox *virtualPower;
        QComboBox *rimSizeCombo;
        QComboBox *tireSizeCombo;
        QLineEdit *wheelSizeEdit;
        QLineEdit *port;
        QLineEdit *profile;
        QGroupBox *selectDefault;
        QCheckBox *defWatts, *defBPM, *defKPH, *defRPM;

    private slots:
        void calcWheelSize();
        void resetWheelSize();

};

class DeviceScanner : public QThread
{
    Q_OBJECT

signals:
    void finished(bool); // threaded scan finished with result x

public:
    DeviceScanner(AddDeviceWizard *);
    bool quickScan(bool deep); // non-threaded
    void run();       // threaded
    void stop();      // stop threaded search

private:
    bool active, found;  // is it still looking?
    QString portSpec;    // did it find a port?

    AddDeviceWizard *wizard;
};

#endif // _AddDeviceWizard_h
