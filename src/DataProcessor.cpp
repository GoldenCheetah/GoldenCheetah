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

#include "DataProcessor.h"
#include "MainWindow.h"
#include "AllPlot.h"
#include "Settings.h"
#include "Units.h"
#include <assert.h>

DataProcessorFactory *DataProcessorFactory::instance_;

DataProcessorFactory &DataProcessorFactory::instance()
{
    if (!instance_) instance_ = new DataProcessorFactory();
    return *instance_;
}

bool
DataProcessorFactory::registerProcessor(QString name, DataProcessor *processor)
{
    assert(!processors.contains(name));
    processors.insert(name, processor);
    return true;
}

bool
DataProcessorFactory::autoProcess(RideFile *ride)
{
    boost::shared_ptr<QSettings> settings = GetApplicationSettings();
    bool changed = false;

    // run through the processors and execute them!
    QMapIterator<QString, DataProcessor*> i(processors);
    i.toFront();
    while (i.hasNext()) {
        i.next();
        QString configsetting = QString("dp/%1/apply").arg(i.key());
        if (settings->value(configsetting, "Manual").toString() == "Auto")
            i.value()->postProcess(ride);
    }

    return changed;
}

ManualDataProcessorDialog::ManualDataProcessorDialog(MainWindow *main, QString name, RideItem *ride) : main(main), ride(ride)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(name);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

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

    config = processor->processorConfig(this);
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
    if (processor->postProcess((RideFile *)ride->ride(), config) == true) {
        main->notifyRideSelected();     // XXX to remain compatible with rest of GC for now
    }
    accept();
}

void
ManualDataProcessorDialog::cancelClicked()
{
    reject();
}
