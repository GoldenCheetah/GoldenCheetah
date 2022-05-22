
/*
 * Batch Export Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
 * Batch Processing Copyright (c) 2022 Paul Johnson (paulj49457@gmail.com)
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

#include "BatchProcessingDialog.h"
#include "MainWindow.h"
#include "Context.h"
#include "Athlete.h"
#include "Colors.h"
#include "RideCache.h"
#include "HelpWhatsThis.h"
#include "CsvRideFile.h"
#include "DataProcessor.h"
#include "RideMetadata.h"
#include "DataProcessor.h"
#include "ConfigDialog.h"

extern ConfigDialog* configdialog_ptr;

#ifdef GC_WANT_PYTHON
#include "FixPyScriptsDialog.h"
#endif

#include <QFormLayout>
#include <QButtonGroup>

Q_DECLARE_METATYPE(DataProcessor*)

CoreDpOptionsDialog::CoreDpOptionsDialog(Context* context) :  ProcessorPage(context) {

    dialog = new QDialog(this);

    QVBoxLayout* lp = new QVBoxLayout(this);
    lp->addWidget(processorTree);
    
    QHBoxLayout* buttonFrame = new QHBoxLayout;
    buttonFrame->addStretch();
    QPushButton* cancelButton = new QPushButton(tr("Cancel"), this);
    buttonFrame->addWidget(cancelButton);
    buttonFrame->addSpacing(10);
    QPushButton* saveButton = new QPushButton(tr("Save"), this);
    buttonFrame->addWidget(saveButton);
    lp->addItem(buttonFrame);

    dialog->setMinimumWidth(850 * dpiXFactor);
    dialog->setMinimumHeight(570 * dpiYFactor);
    dialog->setLayout(lp);

    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelDpEdits()));
    connect(saveButton, SIGNAL(clicked()), this, SLOT(saveDpEdits()));

    dialog->exec();
}

CoreDpOptionsDialog::~CoreDpOptionsDialog() {
    delete dialog;
}
void
CoreDpOptionsDialog::saveDpEdits() {
    saveClicked();
    dialog->done(0);
}

void
CoreDpOptionsDialog::cancelDpEdits() {
    dialog->done(0);
}


BatchProcessingDialog::BatchProcessingDialog(Context* context) : QDialog(context->mainWindow), context(context),
processed(0), fails(0), processedFiles(0) {
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("Activity Batch Processing"));

    HelpWhatsThis* help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Activity_BatchProcessing));

    // make the dialog a resonable size
    setMinimumWidth(650 * dpiXFactor);
    setMinimumHeight(400 * dpiYFactor);

    QFormLayout* layout = new QFormLayout(this);

    // Place a widget in the hieracrchy that can disable all options after processing
    disableContainer = new QWidget;
    layout->addWidget(disableContainer);

    QFormLayout* layout1 = new QFormLayout(disableContainer);
    QButtonGroup* radioGroup = new QButtonGroup(this);

    //  --------------- File/Activity Window items -----------------------

    files = new QTreeWidget;
    files->headerItem()->setText(0, tr(""));
    files->headerItem()->setText(1, tr("Filename"));
    files->headerItem()->setText(2, tr("Date"));
    files->headerItem()->setText(3, tr("Time"));
    files->headerItem()->setText(4, tr("Action"));

    files->setColumnCount(5);
    files->setColumnWidth(0, 20 * dpiXFactor); // selector
    files->setColumnWidth(1, 170 * dpiXFactor); // filename
    files->setColumnWidth(2, 85 * dpiXFactor); // date
    files->setColumnWidth(3, 75 * dpiXFactor); // time
    files->setSelectionMode(QAbstractItemView::SingleSelection);
    files->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    files->setUniformRowHeights(true);
    files->setIndentation(0);

    // honor the context filter
    FilterSet fs;
    fs.addFilter(context->isfiltered, context->filters);
    Specification spec;
    spec.setFilterSet(fs);

    // populate with each ride in the ridelist
    foreach(RideItem * rideItem, context->athlete->rideCache->rides()) {

        // does it match ?
        if (!spec.pass(rideItem)) continue;

        QTreeWidgetItem* add = new QTreeWidgetItem(files->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        // selector
        QCheckBox* checkBox = new QCheckBox("", this);
        checkBox->setChecked(true);
        files->setItemWidget(add, 0, checkBox);

        add->setText(1, rideItem->fileName);
        add->setText(2, rideItem->dateTime.toString(tr("dd MMM yyyy")));
        add->setText(3, rideItem->dateTime.toString("hh:mm:ss"));
        add->setText(4, tr(""));

        processedFiles++;
    }

    //  --------------- Export menu items -----------------------

    QHBoxLayout* exportGrid1 = new QHBoxLayout;
    QHBoxLayout* exportGrid2 = new QHBoxLayout;

    QCheckBox* exportRadioBox = new QCheckBox(tr("Export "), this);
    radioGroup->addButton(exportRadioBox, int(BatchProcessingDialog::exportB));

    fileFormat = new QComboBox(this);

    const RideFileFactory& rff = RideFileFactory::instance();
    fileFormat->addItem(tr("All data (CSV)"));
    foreach(QString suffix, rff.writeSuffixes()) fileFormat->addItem(rff.description(suffix));
    fileFormat->setCurrentIndex(appsettings->value(this, GC_BE_LASTFMT, "0").toInt());

    QPushButton* selectDir = new QPushButton(tr("Browse"), this);
    QLabel* dirLabel = new QLabel(tr("Export to  "), this);

    // default to last used
    QString dirDefault = appsettings->value(this, GC_BE_LASTDIR, QDir::home().absolutePath()).toString();
    if (!QDir(dirDefault).exists()) dirDefault = QDir::home().absolutePath();

    dirName = new QLabel(dirDefault, this);
    overwrite = new QCheckBox(tr("Overwrite Files"), this);

    exportGrid1->addWidget(exportRadioBox);
    exportGrid1->addWidget(fileFormat);
    exportGrid1->addSpacing(10);
    exportGrid1->addWidget(selectDir);
    exportGrid1->addSpacing(10);
    exportGrid1->addWidget(overwrite);
    exportGrid1->addStretch();

    exportGrid2->addSpacing(20);
    exportGrid2->addWidget(dirLabel);
    exportGrid2->addWidget(dirName);
    exportGrid2->addStretch();

    //  --------------- Data Processing menu items -----------------------

    QHBoxLayout* dpGrid = new QHBoxLayout;
    dpGrid->setContentsMargins(0, 0, 0, 0);

    QCheckBox* dpRadioBox = new QCheckBox(tr("Run Core Data Processor "), this);
    radioGroup->addButton(dpRadioBox, int(BatchProcessingDialog::dataB));

    dataProcessorToRun = new QComboBox(this);

    // get the available core processors
    const DataProcessorFactory& factory = DataProcessorFactory::instance();
    QMap<QString, DataProcessor*> coreProcessors = factory.getProcessors(true);

    // iterate over all the processors and add an entry for each data processor
    foreach(DataProcessor * i, coreProcessors) {
        dataProcessorToRun->addItem(i->name());
    }

    dpGrid->addWidget(dpRadioBox);
    dpGrid->addWidget(dataProcessorToRun);

    // If one or more Python Fixes Data Processors are available then provide
    // quick access to the python scripts editing window.
    QPushButton* dpButton = new QPushButton(tr("Configure"), this);
    dpGrid->addSpacing(5);
    dpGrid->addWidget(dpButton);
    dpGrid->addStretch();

    //  --------------- Python Data Processing menu items -----------------------

#ifdef GC_WANT_PYTHON

    pythonProcessorToRun = new QComboBox(this);

    // Initialise the Py Scripts in case they haven't been
    QList<FixPyScript*> fixPyScripts = fixPySettings->getScripts();

    // get all the available processors
    QMap<QString, DataProcessor*> allProcessors = factory.getProcessors(false);

    // Search to see if python processor has been defined
    foreach(DataProcessor * k, allProcessors) {
        // add an entry for each python data processor
        if (!(k->isCoreProcessor())) pythonProcessorToRun->addItem(k->name());
    }

    bool pythonFixesDefined = pythonProcessorToRun->count();
    bool embedPython = appsettings->value(nullptr, GC_EMBED_PYTHON, true).toBool();

    QHBoxLayout* pyGrid = new QHBoxLayout;
    pyGrid->setContentsMargins(0, 0, 0, 0);

    QCheckBox* pyRadioBox = new QCheckBox(tr("Run Python Data Processor "), this);
    radioGroup->addButton(pyRadioBox, int(BatchProcessingDialog::pythonB));

    pyGrid->addWidget(pyRadioBox);
    pyGrid->addWidget(pythonProcessorToRun);

    // If one or more Python Fixes Data Processors are available then provide
    // quick access to the python scripts editing window.
    QPushButton* pyButton = new QPushButton(tr("Edit"), this);
    pyGrid->addSpacing(5);
    pyGrid->addWidget(pyButton);
    pyGrid->addStretch();

    // If there are no Python Fixes defined then default the menu item
    if (!pythonFixesDefined) {
        pythonProcessorToRun->addItem(tr("None Defined"));
        pyButton->setVisible(false);
        pyRadioBox->setCheckable(false);
        radioGroup->removeButton(pyRadioBox);
    }

    // If Python isn't enabled then hide all relevant widgets
    if (!embedPython) {
        pythonProcessorToRun->setVisible(false);
        pyRadioBox->setVisible(false);
        pyButton->setVisible(false);
    }

#endif

    //  --------------- Meta Data Processing menu items -----------------------

    QHBoxLayout* metaGrid = new QHBoxLayout;
    metaGrid->setContentsMargins(0, 0, 0, 0);

    QCheckBox* metaRadioBox = new QCheckBox(tr("Run Metadata Processor"), this);
    radioGroup->addButton(metaRadioBox, int(BatchProcessingDialog::metaB));

    metaDataFieldToUpdate = new QComboBox(this);
    metaDataProcessing = new QComboBox(this);

    // now add the ride metadata fields -- should be the same generally
    foreach(FieldDefinition field, GlobalContext::context()->rideMetadata->getFields()) {
        QString underscored = field.name;
        if (!GlobalContext::context()->specialFields.isMetric(underscored)) {

            // translate to internal name if name has non Latin1 characters
            //underscored = GlobalContext::context()->specialFields.internalName(underscored);
            //field.name = GlobalContext::context()->specialFields.internalName((field.name));

            metaDataFieldToUpdate->addItem(field.name);
            metaDataProcessing->addItem(underscored);

            //rt.lookupMap.insert(underscored.replace(" ", "_"), field.name);
            //rt.lookupType.insert(underscored.replace(" ", "_"), (field.type > 2)); // true if is number
        }
    }

    QLabel* metaLabel = new QLabel(tr(" to "), this);

    metaGrid->addWidget(metaRadioBox);
    metaGrid->addWidget(metaDataFieldToUpdate);
    metaGrid->addWidget(metaLabel);
    metaGrid->addWidget(metaDataProcessing);
    metaGrid->addStretch();

    //  --------------- Delete menu items -----------------------

    QHBoxLayout* deleteGrid = new QHBoxLayout;
    deleteGrid->setContentsMargins(0, 0, 0, 0);

    QCheckBox* deleteRadioBox = new QCheckBox(tr("Delete All Selected "), this);
    radioGroup->addButton(deleteRadioBox, int(BatchProcessingDialog::deleteB));

    deleteGrid->addWidget(deleteRadioBox);

    // -------------- Buttons items -----------------------

    // buttons
    QHBoxLayout* buttons = new QHBoxLayout;

    all = new QCheckBox(tr("check/uncheck all"), this);
    all->setChecked(true);
    status = new QLabel("", this);
    //status->hide();
    cancel = new QPushButton(tr("Cancel"), this);
    ok = new QPushButton(tr("Execute"), this);

    buttons->addWidget(all);
    buttons->addWidget(status);
    buttons->addStretch();
    buttons->addWidget(cancel);
    buttons->addSpacing(5);
    buttons->addWidget(ok);

    // --------------- Arrange dialog layout ----------------------

    layout1->addRow(exportGrid1);
    layout1->addRow(exportGrid2);
    layout1->addRow(dpGrid);

#ifdef GC_WANT_PYTHON
    // If Python is enabled then add the relevant widgets
    if (embedPython) layout1->addRow(pyGrid);
#endif

    layout1->addRow(metaGrid); 
    layout1->addRow(deleteGrid);
    layout->addRow(files);
    layout->addRow(buttons);

    layout->setHorizontalSpacing(0); // between columns
    layout->setVerticalSpacing(10); // between rows
    layout1->setVerticalSpacing(10); // between rows

    adjustSize(); // Window to contents

    // -----------  connect signals and slots up ----------------------

    // Add signals for each of the table checkboxes so when they are updated
    // individually the action column information is also updated by fileSelected()
    for (int i = 0; i < files->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem* current = files->invisibleRootItem()->child(i);

        connect(static_cast<QCheckBox*>(files->itemWidget(current, 0)),
            QOverload<int>::of(&QCheckBox::stateChanged),
            [=](int) { this->fileSelected(current); });
    }

#ifdef GC_WANT_PYTHON
    // If Python is enabled and Python Fixes are defined then connect up the relevant signals
    if ((embedPython) && (pythonFixesDefined)) {
        connect(pythonProcessorToRun, SIGNAL(currentIndexChanged(int)), this, SLOT(comboSelected()));
        connect(pyButton, SIGNAL(clicked()), this, SLOT(dpPyButtonSelected()));
    }
#endif

    // Core data processor signals
    connect(dpButton, SIGNAL(clicked()), this, SLOT(dpButtonSelected()));
    connect(dataProcessorToRun, SIGNAL(currentIndexChanged(int)), this, SLOT(comboSelected()));

    // export file format signals
    connect(fileFormat, SIGNAL(currentIndexChanged(int)), this, SLOT(comboSelected()));
    connect(selectDir, SIGNAL(clicked()), this, SLOT(selectClicked()));

    // radio buttons
    connect(radioGroup, SIGNAL(buttonClicked(int)), this, SLOT(radioClicked(int)));

    connect(all, SIGNAL(stateChanged(int)), this, SLOT(allClicked()));
    connect(ok, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));

    // Set default button to export after signals connected as this is the least destructive option
    exportRadioBox->setChecked(true);
    outputMode = BatchProcessingDialog::exportB;
    updateActionColumn();
    updateNumSelected();
}

void
BatchProcessingDialog::updateNumSelected() {
    status->setText(QString(tr("           %1 files selected")).arg(processedFiles));
}

void
BatchProcessingDialog::selectClicked()
{
    QString before = dirName->text();
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Target Directory"),
                                                 before,
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);
    if (dir!="") {
        dirName->setText(dir);
    }
    return;
}

void
BatchProcessingDialog::dpButtonSelected() {

    CoreDpOptionsDialog dpOptionsDialog(context);
}

#ifdef GC_WANT_PYTHON
void
BatchProcessingDialog::dpPyButtonSelected() {

    FixPyScript* pyScript = fixPySettings->getScript(pythonProcessorToRun->currentText());
    if (pyScript) {
        EditFixPyScriptDialog editDlg(context, pyScript, this);
        editDlg.exec();
    }
}
#endif

void
BatchProcessingDialog::comboSelected() {

    // ensures the Action column matches the Combobox selections
    updateActionColumn();
}

void
BatchProcessingDialog::radioClicked(int buttonId) {

    // ensures the Action info column matches the processing option selected
    outputMode = bpRadioButtonsType(buttonId);
    updateActionColumn();
}

void
BatchProcessingDialog::allClicked()
{
    // set/uncheck all rides according to the "all", this will signal fileSelected()
    // to update the action info column for each entry.
    bool checked = all->isChecked();

    for (int i = 0; i < files->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem* current = files->invisibleRootItem()->child(i);
        static_cast<QCheckBox*>(files->itemWidget(current, 0))->setChecked(checked);
    }
}

void
BatchProcessingDialog::fileSelected(QTreeWidgetItem* current) {

    // ensures individual Action info columns match whether the file is to be processed or not
    if (static_cast<QCheckBox*>(files->itemWidget(current, 0))->isChecked()) {
        current->setText(4, getActionColumnText());
        processedFiles++;
    }
    else {
        current->setText(4, tr("--"));
        processedFiles--;
    }
    updateNumSelected();
}

void
BatchProcessingDialog::updateActionColumn() {

    QString colText = getActionColumnText();

    // Update the files Action info column for entries in files
    for (int i = 0; i < files->invisibleRootItem()->childCount(); i++) {

        QTreeWidgetItem* current = files->invisibleRootItem()->child(i);

        // is it selected
        if (static_cast<QCheckBox*>(files->itemWidget(current, 0))->isChecked()) {
            current->setText(4, colText);
        }
        else {
            current->setText(4, tr("--"));
        }
    }
}

QString
BatchProcessingDialog::getActionColumnText() {

    // provides the Action info text for the various processing options
    switch (outputMode) {
        case BatchProcessingDialog::exportB: {
            return "Export as " + fileFormat->currentText();
        }

        case BatchProcessingDialog::dataB:
        {
            return dataProcessorToRun->currentText();
        }

#ifdef GC_WANT_PYTHON
        case BatchProcessingDialog::pythonB: {
            return pythonProcessorToRun->currentText();
        }
#endif

        case BatchProcessingDialog::metaB:
        {
            return metaDataFieldToUpdate->currentText();
        }

        case BatchProcessingDialog::deleteB: {
            return "Delete";
        }
    }
    return QString("Undefined");
}

void
BatchProcessingDialog::okClicked()
{
    if (ok->text() == "Execute" || ok->text() == tr("Execute")) {
        aborted = false;

        // hide or disable editing in widgets
        all->hide();
        cancel->hide();
        disableContainer->setEnabled(false);
        for (int i = 0; i < files->invisibleRootItem()->childCount(); i++) {
            QTreeWidgetItem* current = files->invisibleRootItem()->child(i);
            static_cast<QCheckBox*>(files->itemWidget(current, 0))->setEnabled(false);
        }

        status->setText(tr("Processing..."));
        ok->setText(tr("Abort"));

        // call the selected type of batch processing
        switch (outputMode) {
            case BatchProcessingDialog::exportB: {
                exportFiles();
            } break;

            case BatchProcessingDialog::dataB: {
                applyDPtoActivities(dataProcessorToRun->currentText());
            } break;

#ifdef GC_WANT_PYTHON
            case BatchProcessingDialog::pythonB: {
                applyDPtoActivities(pythonProcessorToRun->currentText());
            } break;
#endif

            case BatchProcessingDialog::metaB: {
                applyMPtoActivities(metaDataFieldToUpdate->currentText());
            } break;

            case BatchProcessingDialog::deleteB: {
                deleteFiles();
            } break;

            default: {
                aborted = true;
            }
        }
    } else if (ok->text() == "Abort" || ok->text() == tr("Abort")) {
        aborted = true;
    } else if (ok->text() == "Finish" || ok->text() == tr("Finish")) {
        accept(); // our work is done!
    }
}

void
BatchProcessingDialog::cancelClicked()
{
    reject();
}

void
BatchProcessingDialog::exportFiles()
{
    appsettings->setValue(GC_BE_LASTDIR, dirName->text());
    appsettings->setValue(GC_BE_LASTFMT, fileFormat->currentIndex());

    // what file format to export as?
    QString type = fileFormat->currentIndex() > 0 ? RideFileFactory::instance().writeSuffixes().at(fileFormat->currentIndex()-1) : "csv";

    // loop through the table and export all selected
    for(int i=0; i<files->invisibleRootItem()->childCount(); i++) {

        // give user a chance to abort..
        QApplication::processEvents();

        // did they?
        if (aborted == true) return; // user aborted!

        QTreeWidgetItem *current = files->invisibleRootItem()->child(i);

        // is it selected
        if (static_cast<QCheckBox*>(files->itemWidget(current,0))->isChecked()) {

            files->setCurrentItem(current); QApplication::processEvents();

            QString filename = dirName->text() + "/" + QFileInfo(current->text(1)).baseName() + "." + type;

            if (QFile(filename).exists()) {
                if (overwrite->isChecked() == false) {
                    // skip existing files
                    current->setText(4, tr("Exists - not exported")); QApplication::processEvents();
                    fails++;
                    continue;

                } else {

                    // remove existing
                    QFile(filename).remove();
                    current->setText(4, tr("Removing...")); QApplication::processEvents();
                }

            }
            // this one then
            current->setText(4, tr("Reading...")); QApplication::processEvents();

            // open it..
            QStringList errors;
            QList<RideFile*> rides;
            QFile thisfile(QString(context->athlete->home->activities().absolutePath()+"/"+current->text(1)));
            RideFile *ride = RideFileFactory::instance().openRideFile(context, thisfile, errors, &rides);

            // open success?
            if (ride) {

                current->setText(4, tr("Writing...")); QApplication::processEvents();
                QFile out(filename);

                bool success = false;
                if (fileFormat->currentIndex() > 0)
                    success = RideFileFactory::instance().writeRideFile(context, ride, out, type);
                else {
                    CsvFileReader writer;
                    success = writer.writeRideFile(context, ride, out, CsvFileReader::gc);
                }

                // update the Action info column
                if (success) {
                    processed++;
                    current->setText(4, tr("Exported")); QApplication::processEvents();
                } else {
                    fails++;
                    current->setText(4, tr("Write failed")); QApplication::processEvents();
                }

                delete ride; // free memory!

            // open failed
            } else {

                current->setText(4, tr("Read error")); QApplication::processEvents();

            }
        }
    }

    // provide summary of export processing
    status->setText(QString(tr("%1 activities exported, %2 failed or skipped.")).arg(processed).arg(fails));
    ok->setText(tr("Finish"));
}

void
BatchProcessingDialog::deleteFiles() {

    // loop through the table and delete all selected
    for (int i = 0; i < files->invisibleRootItem()->childCount(); i++) {

        // give user a chance to abort..
        QApplication::processEvents();

        // did they?
        if (aborted == true) return; // user aborted!

        QTreeWidgetItem* current = files->invisibleRootItem()->child(i);

        // is it selected for delete ?
        if (static_cast<QCheckBox*>(files->itemWidget(current, 0))->isChecked()) {

            files->setCurrentItem(current);

            if (context->athlete->rideCache->removeRide(current->text(1))) {
                current->setText(4, tr("Deleted"));
                processed++;
            } else {
                current->setText(4, tr("Failed to deleted"));
                fails++;
            }

            QApplication::processEvents();
        }
    }

    // provide summary of delete processing
    status->setText(QString(tr("%1 activities deleted, %2 failed or skipped.")).arg(processed).arg(fails));
    ok->setText(tr("Finish"));
}

void
BatchProcessingDialog::applyDPtoActivities(const QString& processorName) {

    // lookup processor
    DataProcessor* dp = DataProcessorFactory::instance().getProcessors().value(processorName, NULL);

    if (!dp) return; // No such data processor

    // Save the currently selected ride.
    // To achieve batch processing we are going to need to change this in the context temporarily
    // because the python processing is structured that the currently selected RideItem is the one
    // that gets processed with its associated RideFile. If they are not related the processing fails
    // and this is the reason why "import" python processors don't work correctly as the RideItem is that
    // should be associated with the RideFile is not created until after the "import", whilst in the "save"
    // they are both created and associated.
    RideItem* rideISafe = context->ride;

    // loop through the table and run the data processor on each selected activity
    for (int i = 0; i < files->invisibleRootItem()->childCount(); i++) {

        // give user a chance to abort..
        QApplication::processEvents();

        // did they?
        if (aborted == true) return; // user aborted!

        QTreeWidgetItem* current = files->invisibleRootItem()->child(i);

        // is it selected for delete ?
        if (static_cast<QCheckBox*>(files->itemWidget(current, 0))->isChecked()) {

            RideItem *rideI = context->athlete->rideCache->getRide(current->text(1));

            if (!rideI) continue; // eek!

            // Set the current ride to the one being processed for the duration of the processing.
            context->ride = rideI;

            // ack ! we need to autoprocess, so open the ride
            RideFile* rideF = rideI->ride();

            if (!rideF) continue; // eek!

            files->setCurrentItem(current);

            // now run the data processor
            if (dp->postProcess(rideF, NULL, "PYTHON")) {
                // rideFile is now dirty!
                rideI->setDirty(true);
                current->setText(4, tr("Processed"));
                processed++;
            } else {
                current->setText(4, tr("Failed to process"));
                fails++;
            }

            QApplication::processEvents();
        }
    }

    // Restore the selected ride before the batch processing
    context->ride = rideISafe;

    // provide summary of data processing
    status->setText(QString(tr("%1 activities processed, %2 failed or skipped.")).arg(processed).arg(fails));
    ok->setText(tr("Finish"));

}

void
BatchProcessingDialog::applyMPtoActivities(const QString& processorName) {

 /*
    // lookup processor
    DataProcessor* dp = DataProcessorFactory::instance().getProcessors().value(processorName, NULL);

    if (!dp) return; // No such data processor

    // loop through the table and run the data processor on each selected activity
    for (int i = 0; i < files->invisibleRootItem()->childCount(); i++) {

        // give user a chance to abort..
        QApplication::processEvents();

        // did they?
        if (aborted == true) return; // user aborted!

        QTreeWidgetItem* current = files->invisibleRootItem()->child(i);

        // is it selected for delete ?
        if (static_cast<QCheckBox*>(files->itemWidget(current, 0))->isChecked()) {

            RideItem* rideI = context->athlete->rideCache->getRide(current->text(1));

            if (!rideI) continue; // eek!

            // ack ! we need to autoprocess, so open the ride
            RideFile* rideF = rideI->ride();

            if (!rideF) continue; // eek!

            files->setCurrentItem(current);

            // now run the data processor
            if (dp->postProcess(rideF, NULL, "PYTHON")) {
                // rideFile is now dirty!
                rideI->setDirty(true);
                current->setText(4, tr("Processed"));
                processed++;
            }
            else {
                current->setText(4, tr("Failed to process"));
                fails++;
            }

            QApplication::processEvents();
        }
    }

 */

    // provide summary of data processing
    status->setText(QString(tr("%1 activities processed, %2 failed or skipped.")).arg(processed).arg(fails));
    ok->setText(tr("Finish"));

}


