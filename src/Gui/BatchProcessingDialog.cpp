
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

#ifdef GC_WANT_PYTHON
#include "FixPyScriptsDialog.h"
#endif

#include <QFormLayout>
#include <QButtonGroup>
#include <QMessageBox>

BatchProcessingDialog::BatchProcessingDialog(Context* context) : QDialog(context->mainWindow), context(context),
processed(0), fails(0), numFilesToProcess(0), metadataCompleter(nullptr) {
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

    QVBoxLayout* fileSelection = new QVBoxLayout;
    all = new QCheckBox(tr("check/uncheck all"), this);
    all->setChecked(true);
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
        add->setFlags(add->flags() | Qt::ItemIsSelectable);

        // selector
        QCheckBox* checkBox = new QCheckBox("", this);
        checkBox->setChecked(true);
        files->setItemWidget(add, 0, checkBox);

        add->setText(1, rideItem->fileName);
        add->setText(2, rideItem->dateTime.toString(tr("dd MMM yyyy")));
        add->setText(3, rideItem->dateTime.toString(tr("hh:mm:ss")));
        add->setText(4, tr(""));

        numFilesToProcess++;
    }
    fileSelection->addWidget(all);
    fileSelection->addWidget(files);

    //  --------------- Export menu items -----------------------

    QHBoxLayout* exportGrid = new QHBoxLayout;
    QHBoxLayout* exportGrid1 = new QHBoxLayout;
    QHBoxLayout* exportGrid2 = new QHBoxLayout;

    QRadioButton* exportRadioBox = new QRadioButton(tr("Export"), this);
    radioGroup->addButton(exportRadioBox, int(batchRadioBType::exportB));

    QLabel* formatLabel = new QLabel(tr("As"), this);

    fileFormat = new QComboBox(this);

    const RideFileFactory& rff = RideFileFactory::instance();
    fileFormat->addItem(tr("All data (CSV)"));
    foreach(QString suffix, rff.writeSuffixes()) fileFormat->addItem(rff.description(suffix));
    fileFormat->setCurrentIndex(appsettings->value(this, GC_BE_LASTFMT, "0").toInt());

    QPushButton* selectDir = new QPushButton(tr("Browse"), this);

    QLabel* dirLabel = new QLabel(tr("To  "), this);

    // default to last used
    QString dirDefault = appsettings->value(this, GC_BE_LASTDIR, QDir::home().absolutePath()).toString();
    if (!QDir(dirDefault).exists()) dirDefault = QDir::home().absolutePath();

    dirName = new QLabel(dirDefault, this);
    overwrite = new QCheckBox(tr("Overwrite Files"), this);

    exportGrid1->addWidget(formatLabel);
    exportGrid1->addWidget(fileFormat);
    exportGrid1->addWidget(overwrite);
    exportGrid1->addStretch();

    exportGrid2->addWidget(dirLabel);
    exportGrid2->addWidget(dirName);
    exportGrid2->addWidget(selectDir);
    exportGrid2->addStretch();

    exportContainer = new QWidget;
    QVBoxLayout* exportLayout = new QVBoxLayout(exportContainer);
    exportLayout->addLayout(exportGrid1);
    exportLayout->addLayout(exportGrid2);

    exportGrid->addWidget(exportRadioBox);
    exportGrid->addWidget(exportContainer);

    //  --------------- Data Processing menu items -----------------------

    QHBoxLayout* dpGrid = new QHBoxLayout;
    dpGrid->setContentsMargins(0, 0, 0, 0);

    QRadioButton* dpRadioBox = new QRadioButton(tr("Run Data Processor"), this);
    radioGroup->addButton(dpRadioBox, int(batchRadioBType::dataProcessorB));

    dataProcessorToRun = new QComboBox(this);

    // get the available processors
    const DataProcessorFactory& factory = DataProcessorFactory::instance();
    QMap<QString, DataProcessor*> processors = factory.getProcessors();

    // iterate over all the processors and add an entry for each data processor
    QMapIterator<QString, DataProcessor*> i(processors);
    i.toFront();
    while (i.hasNext()) {
        i.next();
        if (! i.value()->isAutomatedOnly()) {
            dataProcessorToRun->addItem(i.value()->name(), i.key());
        }
    }

    dpButton = new QPushButton(tr("Edit"), this);
    dpButton->setVisible(false);

    dpContainer = new QWidget;
    QHBoxLayout* dpLayout = new QHBoxLayout(dpContainer);
    dpLayout->addWidget(dataProcessorToRun);
    dpLayout->addWidget(dpButton);
    dpLayout->addStretch();
    dpContainer->setEnabled(false);

    dpGrid->addWidget(dpRadioBox);
    dpGrid->addWidget(dpContainer);

    //  --------------- Set Meta Data Processing menu items -----------------------

    QHBoxLayout* setMetadataGrid = new QHBoxLayout;
    setMetadataGrid->setContentsMargins(0, 0, 0, 0);

    QRadioButton* setMetadataRadioBox = new QRadioButton(tr("Update Metadata"), this);
    radioGroup->addButton(setMetadataRadioBox, int(batchRadioBType::metadataSetB));

    metadataFieldToSet = new QComboBox(this);
    metadataEditField = new QLineEdit(this);

    // Now add the ride metadata fields to the comboBox
    foreach(FieldDefinition field, GlobalContext::context()->rideMetadata->getFields()) {

        // display the edit icon for relevant metadata fields
        if ((field.name != "Interval Goal") && // cannot specify which interval
            (field.name != "Interval Notes") && // cannot specify which interval
            specialFields.isUser(field.name)) { // user mutable metadata fields

            metadataFieldToSet->addItem(specialFields.displayName(field.name));
        }
    }

    // Setup the type field to match the selected metadata field
    updateMetadataTypeField();

    metaDataContainer = new QWidget;
    QHBoxLayout* metaDataLayout = new QHBoxLayout(metaDataContainer);
    metaDataLayout->addWidget(metadataFieldToSet);
    metaDataLayout->addSpacing(5);
    metaDataLayout->addWidget(metadataEditField);
    metaDataLayout->addSpacing(5);
    metaDataContainer->setEnabled(false);

    setMetadataGrid->addWidget(setMetadataRadioBox);
    setMetadataGrid->addWidget(metaDataContainer);

    //  --------------- Delete menu items -----------------------

    QHBoxLayout* deleteGrid = new QHBoxLayout;
    deleteGrid->setContentsMargins(0, 0, 0, 0);

    QRadioButton* deleteRadioBox = new QRadioButton(tr("Delete All Selected Activities"), this);
    radioGroup->addButton(deleteRadioBox, int(batchRadioBType::deleteB));

    deleteGrid->addWidget(deleteRadioBox);

    // -------------- Buttons items -----------------------

    // buttons
    QHBoxLayout* buttons = new QHBoxLayout;

    status = new QLabel("", this);
    cancel = new QPushButton(tr("Cancel"), this);
    ok = new QPushButton(tr("Execute"), this);

    buttons->addWidget(status);
    buttons->addStretch();
    buttons->addWidget(cancel);
    buttons->addWidget(ok);

    // --------------- Arrange dialog layout ----------------------

    layout1->addRow(exportGrid);
    layout1->addRow(dpGrid);
    layout1->addRow(setMetadataGrid);
    layout1->addRow(deleteGrid);
    layout->addRow(fileSelection);
    layout->addRow(buttons);

    adjustSize(); // Window to contents

    // -----------  connect signals and slots up ----------------------

    // Add signals for each of the table checkboxes so when they are updated
    // individually the action column information is also updated by fileSelected()
    for (int i = 0; i < files->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem* current = files->invisibleRootItem()->child(i);

        connect(static_cast<QCheckBox*>(files->itemWidget(current, 0)),
            QOverload<Qt::CheckState>::of(&QCheckBox::checkStateChanged),
            [=](int) { this->fileSelected(current); });
    }

    // Data processor signals
    connect(dpButton, SIGNAL(clicked()), this, SLOT(dpButtonClicked()));
    connect(dataProcessorToRun, SIGNAL(currentIndexChanged(int)), this, SLOT(comboSelected()));

    // export file format signals
    connect(fileFormat, SIGNAL(currentIndexChanged(int)), this, SLOT(comboSelected()));
    connect(selectDir, SIGNAL(clicked()), this, SLOT(selectClicked()));

    // metadata signals
    connect(metadataFieldToSet, SIGNAL(currentIndexChanged(int)), this, SLOT(comboSelected()));

    // radio buttons
    connect(radioGroup, SIGNAL(idClicked(int)), this, SLOT(radioClicked(int)));

    connect(all, SIGNAL(stateChanged(int)), this, SLOT(allClicked()));
    connect(ok, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));

    // Set default button to export after signals connected as this is the least destructive option
    exportRadioBox->setChecked(true);
    outputMode = batchRadioBType::exportB;
    comboSelected();
    updateActionColumn();
    updateNumberSelected();
}


BatchProcessingDialog::~BatchProcessingDialog() {
    if (metadataCompleter) delete metadataCompleter;
}

void
BatchProcessingDialog::updateNumberSelected() {
    status->setText(QString(tr("%1 files selected")).arg(numFilesToProcess));
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
BatchProcessingDialog::dpButtonClicked() {
#ifdef GC_WANT_PYTHON
    FixPyScript* pyScript = fixPySettings->getScript(dataProcessorToRun->currentText());
    if (pyScript) {
        EditFixPyScriptDialog editDlg(context, pyScript, this);
        editDlg.exec();
    }
#endif
}

void
BatchProcessingDialog::comboSelected() {

    // ensures the Action column matches the Combobox selections
    updateActionColumn();
#ifdef GC_WANT_PYTHON
    // lookup processor and enable dpButton for Python fixes
    DataProcessor* dp = DataProcessorFactory::instance().getProcessors().value(dataProcessorToRun->currentText(), NULL);
    dpButton->setVisible(dp && !dp->isCoreProcessor());
#endif
    updateMetadataTypeField();
}

void
BatchProcessingDialog::radioClicked(int buttonId) {

    // ensures the Action info column matches the processing option selected
    outputMode = batchRadioBType(buttonId);
    updateActionColumn();

    exportContainer->setEnabled(false);
    dpContainer->setEnabled(false);
    metaDataContainer->setEnabled(false);

    // enable only useful widgets
    switch (outputMode) {
        case batchRadioBType::exportB:
            exportContainer->setEnabled(true);
            break;
        case batchRadioBType::dataProcessorB:
            dpContainer->setEnabled(true);
            break;
        case batchRadioBType::metadataSetB:
             metaDataContainer->setEnabled(true);
             break;
         case batchRadioBType::deleteB:
             break;
    }
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
        numFilesToProcess++;
    }
    else {
        current->setText(4, tr("--"));
        numFilesToProcess--;
    }
    updateNumberSelected();
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
        bpFailureType result = bpFailureType::unknownF;

        QString summaryType(tr("Processed "));

        // call the selected type of batch processing
        switch (outputMode) {
            case batchRadioBType::exportB: {
                result = exportFiles();
                summaryType = tr("Exported ");
            } break;

            case batchRadioBType::dataProcessorB: {
                result = runDataProcessorOnActivities();
            } break;

            case batchRadioBType::metadataSetB: {
                result = setMetadataForActivities();
            } break;

            case batchRadioBType::deleteB: {
                result = deleteFiles();
                summaryType = tr("Deleted ");
            } break;
        }

        switch (result) {

            case bpFailureType::dateFormatF: {
                status->setText(tr("Processing failed due date format error..."));
                break;
            }
            case bpFailureType::timeFormatF: {
                status->setText(tr("Processing failed due time format error..."));
                break;
            }
            case bpFailureType::userF: {
                status->setText(tr("Processing aborted by the user..."));
                break;
            }
            case bpFailureType::noDataProcessorF: {
                status->setText(tr("Processing failed as the data processor cannot be found..."));
                break;
            }
            case bpFailureType::finishedF: {
                status->setText(summaryType + QString(tr("%1 successful, %2 failed or skipped.")).arg(processed).arg(fails));
                ok->setText(tr("Finish"));
                break;
            }
            default: {
                status->setText(tr("Processing failed for an unknown reason..."));
                break;
            }
        }
    } else if (ok->text() == "Abort" || ok->text() == tr("Abort")) {
        aborted = true;
        ok->setText(tr("Finish"));
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
BatchProcessingDialog::updateMetadataTypeField() {

    // find the selected metadata field and update the display with its type
    foreach(FieldDefinition field, GlobalContext::context()->rideMetadata->getFields()) {
        if (metadataFieldToSet->currentText() == field.name) {
            switch (field.type) {
                case FIELD_INTEGER: {
                    metadataEditField->setText(tr("0"));
                    return;
                }
                case FIELD_DOUBLE: {
                    metadataEditField->setText(tr("0.00"));
                    return;
                }
                case FIELD_DATE: {
                    metadataEditField->setText(tr("dd/mm/yyyy"));
                    return;
                }
                case FIELD_TIME: {
                    metadataEditField->setText(tr("hh:mm:ss"));
                    return;
                }
                case FIELD_CHECKBOX: {
                    metadataEditField->setText(tr("1|0"));
                    return;
                }
                case FIELD_TEXT:
                case FIELD_TEXTBOX:
                case FIELD_SHORTTEXT:
                default: {
                    metadataEditField->setText(tr(""));
                    if (metadataCompleter) delete metadataCompleter;
                    metadataCompleter = field.getCompleter(this, context->athlete->rideCache);
                    metadataEditField->setCompleter(metadataCompleter); // Set or clear the completer
                    return;
                }
            }
        }
    }
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
        case batchRadioBType::exportB: {
            return tr("Export as ") + fileFormat->currentText();
        }

        case batchRadioBType::dataProcessorB: {
            return dataProcessorToRun->currentText();
        }

        case batchRadioBType::metadataSetB: {
            return tr("Update Metadata field - ") + metadataFieldToSet->currentText();
        }

        case batchRadioBType::deleteB: {
            return tr("Delete");
        }
    }
    return QString(tr("Undefined"));
}

BatchProcessingDialog::bpFailureType
BatchProcessingDialog::exportFiles()
{
    appsettings->setValue(GC_BE_LASTDIR, dirName->text());
    appsettings->setValue(GC_BE_LASTFMT, fileFormat->currentIndex());

    // what file format to export as?
    QString type = fileFormat->currentIndex() > 0 ? RideFileFactory::instance().writeSuffixes().at(fileFormat->currentIndex()-1) : "csv";

    // loop through the table and export all selected
    for (int i=0; i<files->invisibleRootItem()->childCount(); i++) {

        // give user a chance to abort..
        QApplication::processEvents();

        // did they?
        if (aborted == true) return bpFailureType::userF; // user aborted!

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
            RideFile *rideF = RideFileFactory::instance().openRideFile(context, thisfile, errors, &rides);

            // open success?
            if (rideF) {

                current->setText(4, tr("Writing...")); QApplication::processEvents();
                QFile out(filename);

                bool success = false;
                if (fileFormat->currentIndex() > 0)
                    success = RideFileFactory::instance().writeRideFile(context, rideF, out, type);
                else {
                    CsvFileReader writer;
                    success = writer.writeRideFile(context, rideF, out, CsvFileReader::gc);
                }

                // update the Action info column
                if (success) {
                    processed++;
                    current->setText(4, tr("Exported")); QApplication::processEvents();
                } else {
                    fails++;
                    current->setText(4, tr("Write failed")); QApplication::processEvents();
                }

                delete rideF; // free memory!

            // open failed
            } else {

                current->setText(4, tr("Read error")); QApplication::processEvents();

            }
        }
    }

    return bpFailureType::finishedF;
}

BatchProcessingDialog::bpFailureType
BatchProcessingDialog::deleteFiles() {

    QMessageBox msgBox;
    msgBox.setText(tr("Are you sure you want to delete all selected activities?"));
    QPushButton *deleteButton = msgBox.addButton(tr("Delete"), QMessageBox::YesRole);
    msgBox.setStandardButtons(QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.exec();
    bool ok = msgBox.clickedButton() == deleteButton;

    // loop through the table and delete all selected
    for (int i = 0; ok && i < files->invisibleRootItem()->childCount(); i++) {

        // give user a chance to abort..
        QApplication::processEvents();

        // did they?
        if (aborted == true) return bpFailureType::userF; // user aborted!

        QTreeWidgetItem* current = files->invisibleRootItem()->child(i);

        // is it selected for delete ?
        if (static_cast<QCheckBox*>(files->itemWidget(current, 0))->isChecked()) {

            files->setCurrentItem(current);

            if (context->athlete->rideCache->removeRide(current->text(1))) {
                current->setText(4, tr("Deleted"));
                processed++;
            } else {
                failedToProcessEntry(current);
            }

            QApplication::processEvents();
        }
    }

    return bpFailureType::finishedF;
}

BatchProcessingDialog::bpFailureType
BatchProcessingDialog::runDataProcessorOnActivities() {

    // lookup processor
    QString processorName(dataProcessorToRun->currentData().toString());
    DataProcessor *dp = DataProcessorFactory::instance().getProcessors().value(processorName, NULL);

    if (!dp) return bpFailureType::noDataProcessorF; // No such data processor

    // get processor config
    DataProcessorConfig* config = dp->processorConfig(this);

    // Allow the user to set parameters and confirm or cancel
    ManualDataProcessorDialog* p = new ManualDataProcessorDialog(context, processorName, nullptr, config);
    p->setWindowModality(Qt::ApplicationModal); // don't allow select other ride or it all goes wrong!
    bool ok = p->exec();

    // loop through the table and run the data processor on each selected activity
    for (int i = 0; ok && i < files->invisibleRootItem()->childCount(); i++) {

        // give user a chance to abort..
        QApplication::processEvents();

        // did they?
        if (aborted == true) return bpFailureType::userF; // user aborted!

        QTreeWidgetItem* current = files->invisibleRootItem()->child(i);

        // is it selected for processing ?
        if (static_cast<QCheckBox*>(files->itemWidget(current, 0))->isChecked()) {

            RideItem *rideI = context->athlete->rideCache->getRide(current->text(1));

            if (!rideI) { failedToProcessEntry(current); continue; } // eek!

            // ack ! we need to autoprocess, so open the ride
            RideFile* rideF = rideI->ride();

            if (!rideF) { failedToProcessEntry(current); continue; } // eek!

            files->setCurrentItem(current);

            // now run the data processor
            if (dp->postProcess(rideF, config, "UPDATE")) {
                // rideFile is now dirty!
                rideI->setDirty(true);
                current->setText(4, tr("Processed"));
                processed++;
            } else {
                failedToProcessEntry(current);
            }

            QApplication::processEvents();
        }
    }

    delete p; // no parent and WA_DeleteOnClose not set, lets delete it.

    return bpFailureType::finishedF;
}

BatchProcessingDialog::bpFailureType
BatchProcessingDialog::setMetadataForActivities() {

    // Convert from metdata diosplay name to internal name
    QString metadataFieldName(specialFields.internalName(metadataFieldToSet->currentText()));
    QString metadataValue(metadataEditField->text());


    // find the selected metadata field and validate the format of time & date fields
    foreach(FieldDefinition field, GlobalContext::context()->rideMetadata->getFields()) {
        if (metadataFieldName == field.name) {

            if (field.type == FIELD_TIME) {
                metadataValue.simplified().remove(' ');
                QRegularExpression re("([0-1]?[0-9]|2[0-3]):([0-5]?[0-9]):([0-5]?[0-9])");
                if (!re.match(metadataValue).hasMatch()) return bpFailureType::timeFormatF;
            }

            if (field.type == FIELD_DATE) {
                metadataValue.simplified().remove(' ');
                QRegularExpression re("([0]?[1-9]|[1][0-9]|3[0-1])/([0]?[1-9]|[1][0-2])/([0-9]{4})");
                if (!re.match(metadataValue).hasMatch()) return bpFailureType::dateFormatF;
            }
            break;
        }
    }

    // loop through the table and set the metdata on each selected activity
    for (int i = 0; i < files->invisibleRootItem()->childCount(); i++) {

        // give user a chance to abort..
        QApplication::processEvents();

        // did they?
        if (aborted == true) return bpFailureType::userF; // user aborted!

        QTreeWidgetItem* current = files->invisibleRootItem()->child(i);

        // is it selected for metadata update ?
        if (static_cast<QCheckBox*>(files->itemWidget(current, 0))->isChecked()) {

            RideItem* rideI = context->athlete->rideCache->getRide(current->text(1));

            if (!rideI) { failedToProcessEntry(current); continue; } // eek!

            // ack ! we need to autoprocess, so open the ride
            RideFile* rideF = rideI->ride();

            if (!rideF) { failedToProcessEntry(current); continue; } // eek!

            // Set the new metadata value
            rideF->setTag(metadataFieldName, metadataValue);

            // rideFile is now dirty!
            rideI->setDirty(true);

            // get refresh done, coz overrides state has changed
            rideI->notifyRideMetadataChanged();

            // Update the files entry
            current->setText(4, tr("Metadata Tag Set"));
            processed++;
        }
    }

    return bpFailureType::finishedF;
}

void
BatchProcessingDialog::failedToProcessEntry(QTreeWidgetItem* current) {
    current->setText(4, tr("Failed to process activity"));
    fails++;
}

