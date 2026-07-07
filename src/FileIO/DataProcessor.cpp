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


DataProcessor::Automation
DataProcessor::getAutomation
() const
{
    QString automationStr = appsettings->value(nullptr, configKeyAutomation(id()), "Manual").toString();
    Automation automation = Manual;
    if (automationStr == "Auto") {
        automation = Auto;
    } else if (automationStr == "Save") {
        automation = Save;
    }
    return automation;
}


void
DataProcessor::setAutomation
(DataProcessor::Automation automation)
{
    QString automationStr;
    // which mode is selected?
    switch (automation) {
    case Auto:
        automationStr = "Auto";
        break;
    case Save:
        automationStr = "Save";
        break;
    case Manual:
    default:
        automationStr = "Manual";
        break;
    }
    appsettings->setValue(configKeyAutomation(id()), automationStr);
}


bool
DataProcessor::isAutomatedOnly
() const
{
    return appsettings->value(nullptr, configKeyAutomatedOnly(id()), false).toBool();
}

void
DataProcessor::setAutomatedOnly
(bool automatedOnly)
{
    appsettings->setValue(configKeyAutomatedOnly(id()), automatedOnly);
}


QString
DataProcessor::configKeyAutomatedOnly
(const QString &id)
{
    return GC_QSETTINGS_GLOBAL_GENERAL + QString("dp/%1/automatedonly").arg(id);
}


QString
DataProcessor::configKeyAutomation
(const QString &id)
{
    return GC_QSETTINGS_GLOBAL_GENERAL + QString("dp/%1/automation").arg(id);
}


QString
DataProcessor::configKeyApply
(const QString &id)
{
    return GC_QSETTINGS_GLOBAL_GENERAL + QString("dp/%1/apply").arg(id);
}


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
DataProcessorFactory::registerProcessor(DataProcessor *processor)
{
    if (processors.contains(processor->id())) return false; // don't register twice!
    processors.insert(processor->id(), processor);
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


DataProcessor*
DataProcessorFactory::getProcessor
(const QString &id) const
{
#ifdef GC_WANT_PYTHON
    fixPySettings->initialize();
#endif

    DataProcessor *ret = processors.value(id, nullptr);
    if (ret == nullptr) {
        QMapIterator<QString, DataProcessor*> i(processors);
        i.toFront();
        while (i.hasNext()) {
            i.next();
            if (i.value()->legacyId() == id) {
                ret = i.value();
                break;
            }
        }
    }
    return ret;
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


QList<DataProcessor*>
DataProcessorFactory::getProcessorsSorted
(bool coreProcessorsOnly) const
{
    QMap<QString, DataProcessor*> map = getProcessors(coreProcessorsOnly);
    QList<DataProcessor*> processors = map.values();
    std::sort(processors.begin(), processors.end(), [](DataProcessor *l, DataProcessor *r) {
        return QString::localeAwareCompare(l->name(), r->name()) < 0;
    });
    return processors;
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

        // if we're being run manually, run all that are defined
        if (appsettings->value(NULL, i.value()->configKeyAutomation(i.key()), "Manual").toString() == mode)
            i.value()->postProcess(ride, NULL, op);
    }

    return changed;
}

ManualDataProcessorDialog::ManualDataProcessorDialog(Context *context, QString name, RideItem *ride, DataProcessorConfig *conf) : context(context), ride(ride), config(conf)
{
    if (config == nullptr) setAttribute(Qt::WA_DeleteOnClose); // don't destroy received config
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

    if (config == nullptr) config = processor->processorConfig(this, ride ? ride->ride() : nullptr);
    config->readConfig();
    explain = new QTextEdit(this);
    explain->setText(processor->explain());
    explain->setReadOnly(true);

    mainLayout->addWidget(configLabel);
    mainLayout->addWidget(config);
    mainLayout->addWidget(explainLabel);
    mainLayout->addWidget(explain);

    saveAsDefault = new QCheckBox(tr("Save parameters as default"), this);
    saveAsDefault->setChecked(false);
    ok = new QPushButton(tr("OK"), this);
    cancel = new QPushButton(tr("Cancel"), this);
    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addWidget(saveAsDefault);
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

    // Save parameters as default on user request
    if (saveAsDefault->isChecked()) config->saveConfig();

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
