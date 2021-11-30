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

#ifndef _DataProcessor_h
#define _DataProcessor_h
#include "GoldenCheetah.h"

#include "RideFile.h"
#include "RideFileCommand.h"
#include "RideItem.h"

#include <QtGui>
#include <QWidget>
#include <QDialog>
#include <QDate>
#include <QDir>
#include <QLabel>
#include <QFile>
#include <QList>
#include <QTextEdit>
#include <QLineEdit>
#include <QMap>
#include <QVector>

// This file defines four classes:
//
// DataProcessorConfig is a base QWidget that must be supplied by the
// DataProcessor to enable the user to configure its options
//
// DataProcessor is an abstract base class for function-objects that take a
// rideFile and manipulate it. Examples include fixing gaps in recording or
// creating the .notes or .cpi file
//
// DataProcessorFactory is a singleton that maintains a mapping of
// all DataProcessor objects that can be applied to rideFiles
//
// ManualDataProcessorDialog is a dialog box to manually execute a
// dataprocessor on the current ride and is called from the mainWindow menus
//

// every data processor must supply a configuration Widget
// when its processorConfig member is called
class DataProcessorConfig : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        DataProcessorConfig(QWidget *parent=0) : QWidget(parent) {}
        virtual ~DataProcessorConfig() {}
        virtual void readConfig() = 0;
        virtual void saveConfig() = 0;
        virtual QString explain() = 0;
};

// the data processor abstract base class
class DataProcessor
{
    public:
        DataProcessor() {}
        virtual ~DataProcessor() {}
        virtual bool postProcess(RideFile *, DataProcessorConfig*settings=0, QString op="") = 0;
        virtual DataProcessorConfig *processorConfig(QWidget *parent, const RideFile* ride = NULL) = 0;
        virtual QString name() = 0; // Localized Name for user interface
        virtual bool isCoreProcessor() { return true; }
};

// all data processors
class DataProcessorFactory {

    private:

        static DataProcessorFactory *instance_;
        static bool autoprocess;
        QMap<QString,DataProcessor*> processors;
        DataProcessorFactory() {}


    public:

        ~DataProcessorFactory();
        static DataProcessorFactory &instance();
        bool registerProcessor(QString name, DataProcessor *processor);
        void unregisterProcessor(QString name);
        QMap<QString,DataProcessor*> getProcessors(bool coreProcessorsOnly = false) const;
        bool autoProcess(RideFile *, QString mode, QString op); // run auto processes (after open rideFile)
        void setAutoProcessRule(bool b) { autoprocess = b; } // allows to switch autoprocess off (e.g. for Upgrades)
};

class Context;
class ManualDataProcessorDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:
        ManualDataProcessorDialog(Context *, QString, RideItem *);

    private slots:
        void cancelClicked();
        void okClicked();

    private:

        Context *context;
        RideItem *ride;
        DataProcessor *processor;
        DataProcessorConfig *config;
        QTextEdit *explain;
        QPushButton *ok, *cancel;
};
#endif // _DataProcessor_h
