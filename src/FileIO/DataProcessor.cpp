/*
 * Copyright (c) 2010 mark Liversedge )liversedge@gmail.com)
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

#include <QApplication>
#include "DataProcessor.h"
#include "Context.h"
#include "AllPlot.h"
#include "Settings.h"
#include "Units.h"
#include "Colors.h"
#ifdef GC_WANT_PYTHON
#include "PythonEmbed.h"
#include "FixPySettings.h"
#endif

DataProcessorFactory *DataProcessorFactory::instance_;
DataProcessorFactory &DataProcessorFactory::instance()
{
    if (!instance_) instance_ = new DataProcessorFactory();
    return *instance_;
}

bool DataProcessorFactory::autoprocess = true;

DataProcessorFactory::~DataProcessorFactory()
{
    qDeleteAll(processors);
}

bool
DataProcessorFactory::registerProcessor(QString name, DataProcessor *processor)
{
    if (processors.contains(name)) return false; // don't register twice!
    processors.insert(name, processor);
    return true;
}

void
DataProcessorFactory::unregisterProcessor(QString name)
{
    if (!processors.contains(name)) return;
    DataProcessor *processor = processors.value(name);
    processors.remove(name);
    delete processor;
}

QMap<QString, DataProcessor *>
DataProcessorFactory::getProcessors(bool coreProcessorsOnly) const
{
#ifdef GC_WANT_PYTHON
    fixPySettings->initialize();
#endif

    if (!coreProcessorsOnly) return processors;

    QMap<QString, DataProcessor *> coreProcessors;
    QMapIterator<QString, DataProcessor*> i(processors);
    i.toFront();
    while (i.hasNext()) {
        i.next();
        if (i.value()->isCoreProcessor()) coreProcessors.insert(i.key(), i.value());
    }

    return coreProcessors;
}

bool
DataProcessorFactory::autoProcess(RideFile *ride, QString mode, QString op)
{
    // mode will be either "Auto" for automatically run (at import, or data filter)
    // or it will be "Save" for running just before we save

    // check if autoProcess is allow at all
    if (!autoprocess) return false;

#ifdef GC_WANT_PYTHON
    fixPySettings->initialize();
#endif

    bool changed = false;

    // run through the processors and execute them!
    QMapIterator<QString, DataProcessor*> i(processors);
    i.toFront();
    while (i.hasNext()) {
        i.next();

        if (!i.value()->isCoreProcessor()) continue; // Python DP are not supported in automatic mode

        QString configsetting = QString("dp/%1/apply").arg(i.key());

        // if we're being run manually, run all that are defined
        if (appsettings->value(NULL, GC_QSETTINGS_GLOBAL_GENERAL+configsetting, "Manual").toString() == mode)
            i.value()->postProcess(ride, NULL, op);
    }

    return changed;
}

ManualDataProcessorDialog::ManualDataProcessorDialog(Context *context, QString name, RideItem *ride) : context(context), ride(ride)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(name);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    setMinimumSize(QSize(300 *dpiXFactor, 300 *dpiYFactor));

    // find our processor
    const DataProcessorFactory &factory = DataProcessorFactory::instance();
    QMap<QString, DataProcessor*> processors = factory.getProcessors();
    processor = processors.value(name, NULL);

    if (processor == NULL) reject();

    // Change window title to Localized Name
    setWindowTitle(processor->name());

    QFont font;
    font.setWeight(QFont::Black);
    QLabel *configLabel = new QLabel(tr("Settings"), this);
    configLabel->setFont(font);
    QLabel *explainLabel = new QLabel(tr("Description"), this);
    explainLabel->setFont(font);

    config = processor->processorConfig(this, ride->ride());
    config->readConfig();
    explain = new QTextEdit(this);
    explain->setText(config->explain());
    explain->setReadOnly(true);

    mainLayout->addWidget(configLabel);
    mainLayout->addWidget(config);
    mainLayout->addWidget(explainLabel);
    mainLayout->addWidget(explain);

    ok = new QPushButton(tr("OK"), this);
    cancel = new QPushButton(tr("Cancel"), this);
    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addStretch();
    buttons->addWidget(cancel);
    buttons->addWidget(ok);

    mainLayout->addLayout(buttons);

    connect(ok, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
}

void
ManualDataProcessorDialog::okClicked()
{
    // stop ok button from being clickable and show waiting cursor
    ok->setEnabled(false);
    cancel->setEnabled(false);

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    if (ride && ride->ride() && processor->postProcess((RideFile *)ride->ride(), config, "UPDATE") == true) {
        context->notifyRideSelected(ride);     // to remain compatible with rest of GC for now
    }

    // reset cursor and wait
    QApplication::restoreOverrideCursor();

    // and we're done
    accept();
}

void
ManualDataProcessorDialog::cancelClicked()
{
    reject();
}
