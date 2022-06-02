
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

#define bpABORT "Abort"
#define bpFINISH "Finish"
#define bpEXECUTE "Execute"

BatchProcessingDialog::BatchProcessingDialog(Context* context) : QDialog(context->mainWindow), context(context),
processed(0), fails(0), numFilesToProcess(0) {
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

        numFilesToProcess++;
    }

    //  --------------- Export menu items -----------------------

    QHBoxLayout* exportGrid1 = new QHBoxLayout;
    QHBoxLayout* exportGrid2 = new QHBoxLayout;

    QRadioButton* exportRadioBox = new QRadioButton(tr("Export "), this);
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

    QRadioButton* dpRadioBox = new QRadioButton(tr("Run Core Data Processor "), this);
    radioGroup->addButton(dpRadioBox, int(BatchProcessingDialog::dataProcessorB));

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
    QPushButton* dpEditParametersButton = new QPushButton(tr("Configure"), this);
    dpGrid->addSpacing(5);
    dpGrid->addWidget(dpEditParametersButton);
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

    QRadioButton* pyRadioBox = new QRadioButton(tr("Run Python Data Processor "), this);
    radioGroup->addButton(pyRadioBox, int(BatchProcessingDialog::pythonProcessorB));

    pyGrid->addWidget(pyRadioBox);
    pyGrid->addWidget(pythonProcessorToRun);

    // If one or more Python Fixes Data Processors are available then provide
    // quick access to the python scripts editing window.
    QPushButton* pyEditParametersButton = new QPushButton(tr("Edit"), this);
    pyGrid->addSpacing(5);
    pyGrid->addWidget(pyEditParametersButton);
    pyGrid->addStretch();

    // If there are no Python Fixes defined then default the menu item
    if (!pythonFixesDefined) {
        pythonProcessorToRun->addItem(tr("None Defined"));
        pyEditParametersButton->setVisible(false);
        pyRadioBox->setCheckable(false);
        radioGroup->removeButton(pyRadioBox);
    }

    // If Python isn't enabled then hide all relevant widgets
    if (!embedPython) {
        pythonProcessorToRun->setVisible(false);
        pyRadioBox->setVisible(false);
        pyEditParametersButton->setVisible(false);
    }

#endif

    //  --------------- Set Meta Data Processing menu items -----------------------

    QHBoxLayout* setMetadataGrid = new QHBoxLayout;
    setMetadataGrid->setContentsMargins(0, 0, 0, 0);

    QRadioButton* setMetadataRadioBox = new QRadioButton(tr("Update Metadata"), this);
    radioGroup->addButton(setMetadataRadioBox, int(BatchProcessingDialog::metadataSetB));

    metadataFieldToSet = new QComboBox(this);
    metadataEditField =  new QLineEdit(this);

    // now add the ride metadata fields -- should be the same generally
    foreach(FieldDefinition field, GlobalContext::context()->rideMetadata->getFields()) {

        // Remove fundamental key fields from the editing capability, as batch editing these could be very destructive
        if (field.name != "Start Time" && field.name != "Start Date" && field.name != "Data") {

            if (displayNametoRideMetric(field.name) == NULL) metadataFieldToSet->addItem(field.name);
        }
    }

    // Setup the type field to match the selected metadata field
    updateMetadataTypeField();

    setMetadataGrid->addWidget(setMetadataRadioBox);
    setMetadataGrid->addWidget(metadataFieldToSet);
    setMetadataGrid->addSpacing(5);
    setMetadataGrid->addWidget(metadataEditField);
    setMetadataGrid->addSpacing(5);

    //  --------------- Set Metric Override menu items -----------------------

    QHBoxLayout* setMetricOverrideGrid = new QHBoxLayout;
    setMetricOverrideGrid->setContentsMargins(0, 0, 0, 0);

    QRadioButton* setMetricRadioBox = new QRadioButton(tr("Set Metric Override"), this);
    radioGroup->addButton(setMetricRadioBox, int(BatchProcessingDialog::metricSetB));

    metricFieldToSet = new QComboBox(this);
    metricDataEditField = new QLineEdit(this);

    // now add the ride metadata fields -- should be the same generally
    foreach(FieldDefinition field, GlobalContext::context()->rideMetadata->getFields()) {
        if (displayNametoRideMetric(field.name)) metricFieldToSet->addItem(field.name);
    }

    metricUnitsLabel = new QLabel(tr(""), this);

    // Setup the type field to match the selected metadata field
    updateMetricDataTypeField();

    setMetricOverrideGrid->addWidget(setMetricRadioBox);
    setMetricOverrideGrid->addWidget(metricFieldToSet);
    setMetricOverrideGrid->addSpacing(5);
    setMetricOverrideGrid->addWidget(metricDataEditField);
    setMetricOverrideGrid->addSpacing(5);
    setMetricOverrideGrid->addWidget(metricUnitsLabel);
    setMetricOverrideGrid->addStretch();

    //  --------------- Clear Metric Override menu items -----------------------

    QHBoxLayout* clearMetricOverrideGrid = new QHBoxLayout;
    clearMetricOverrideGrid->setContentsMargins(0, 0, 0, 0);

    QRadioButton* clearMetricOverrideRadioBox = new QRadioButton(tr("Clear Metric Override"), this);
    radioGroup->addButton(clearMetricOverrideRadioBox, int(BatchProcessingDialog::metricClearB));

    metricFieldToClear = new QComboBox(this);

    // now add the ride metric fields
    foreach(FieldDefinition field, GlobalContext::context()->rideMetadata->getFields()) {
        if (displayNametoRideMetric(field.name)) metricFieldToClear->addItem(field.name);
    }

    clearMetricOverrideGrid->addWidget(clearMetricOverrideRadioBox);
    clearMetricOverrideGrid->addWidget(metricFieldToClear);
    clearMetricOverrideGrid->addStretch();
    

    //  --------------- Delete menu items -----------------------

    QHBoxLayout* deleteGrid = new QHBoxLayout;
    deleteGrid->setContentsMargins(0, 0, 0, 0);

    QRadioButton* deleteRadioBox = new QRadioButton(tr("Delete All Selected Activities "), this);
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
    ok = new QPushButton(tr(bpEXECUTE), this);

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

    layout1->addRow(setMetadataGrid); 
    layout1->addRow(setMetricOverrideGrid);
    layout1->addRow(clearMetricOverrideGrid);
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
        connect(pyEditParametersButton, SIGNAL(clicked()), this, SLOT(pyEditParametersButtonSelected()));
    }
#endif

    // Core data processor signals
    connect(dpEditParametersButton, SIGNAL(clicked()), this, SLOT(dpEditParametersButtonSelected()));
    connect(dataProcessorToRun, SIGNAL(currentIndexChanged(int)), this, SLOT(comboSelected()));

    // export file format signals
    connect(fileFormat, SIGNAL(currentIndexChanged(int)), this, SLOT(comboSelected()));
    connect(selectDir, SIGNAL(clicked()), this, SLOT(selectClicked()));

    // metadata signals
    connect(metadataFieldToSet, SIGNAL(currentIndexChanged(int)), this, SLOT(comboSelected()));
    connect(metricFieldToSet, SIGNAL(currentIndexChanged(int)), this, SLOT(comboSelected()));
    connect(metricFieldToClear, SIGNAL(currentIndexChanged(int)), this, SLOT(comboSelected()));
        
    // radio buttons
    connect(radioGroup, SIGNAL(buttonClicked(int)), this, SLOT(radioClicked(int)));

    connect(all, SIGNAL(stateChanged(int)), this, SLOT(allClicked()));
    connect(ok, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));

    // Set default button to export after signals connected as this is the least destructive option
    exportRadioBox->setChecked(true);
    outputMode = BatchProcessingDialog::exportB;
    updateActionColumn();
    updateNumberSelected();
}

void
BatchProcessingDialog::updateNumberSelected() {
    status->setText(QString(tr("           %1 files selected")).arg(numFilesToProcess));
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
BatchProcessingDialog::dpEditParametersButtonSelected() {

    CoreDpOptionsDialog dpOptionsDialog(context);
}

#ifdef GC_WANT_PYTHON
void
BatchProcessingDialog::pyEditParametersButtonSelected() {

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

    updateMetadataTypeField();
    updateMetricDataTypeField();
}

void
BatchProcessingDialog::radioClicked(int buttonId) {

    // ensures the Action info column matches the processing option selected
    outputMode = batchRadioBType(buttonId);
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
    if (ok->text() == bpEXECUTE || ok->text() == tr(bpEXECUTE)) {
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
        ok->setText(tr(bpABORT));
        bpFailureType result = BatchProcessingDialog::unknownF;

        QString summaryType("Processed ");

        // call the selected type of batch processing
        switch (outputMode) {
            case BatchProcessingDialog::exportB: {
                result = exportFiles();
                summaryType = "Exported ";
            } break;

            case BatchProcessingDialog::dataProcessorB: {
                result = runDataProcessorOnActivities(dataProcessorToRun->currentText());
            } break;

#ifdef GC_WANT_PYTHON
            case BatchProcessingDialog::pythonProcessorB: {
                result = runDataProcessorOnActivities(pythonProcessorToRun->currentText());
            } break;
#endif

            case BatchProcessingDialog::metadataSetB: {
                result = setMetadataForActivities(metadataFieldToSet->currentText(), metadataEditField->text());
            } break;

            case BatchProcessingDialog::metricSetB: {
                result = setMetricFieldForActivities(metricFieldToSet->currentText(), metricDataEditField->text());
            } break;

            case BatchProcessingDialog::metricClearB: {
                result = clearMetricFieldForActivities(metricFieldToClear->currentText());
            } break;

            case BatchProcessingDialog::deleteB: {
                result = deleteFiles();
                summaryType = "Deleted ";
            } break;
        }
    
        switch (result) {

            case BatchProcessingDialog::dateFormatF: {
                status->setText(tr("Processing failed due date format error..."));
                break;
            }
            case BatchProcessingDialog::timeFormatF: {
                status->setText(tr("Processing failed due time format error..."));
                break;
            }
            case BatchProcessingDialog::noRideMFoundF: {
                status->setText(tr("Processing failed as the ride metric cannot be found..."));
                break;
            }
            case BatchProcessingDialog::userF: {
                status->setText(tr("Processing aborted by the user..."));
                break;
            }
            case BatchProcessingDialog::noDataProcessorF: {
                status->setText(tr("Processing failed as the data processor cannot be found..."));
                break;
            }
            case BatchProcessingDialog::finishedF: {
                status->setText(summaryType + QString(tr("%1 activities successfully, %2 failed or skipped.")).arg(processed).arg(fails));
                ok->setText(tr(bpFINISH));
                break;
            }
            default: {
                status->setText(tr("Processing failed for an unknown reason..."));
                break;
            }
        }
    } else if (ok->text() == bpABORT || ok->text() == tr(bpABORT)) {
        aborted = true;
        ok->setText(tr(bpFINISH));
    } else if (ok->text() == bpFINISH || ok->text() == tr(bpFINISH)) {
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
                    metadataEditField->setText("0");
                    return;
                }
                case FIELD_DOUBLE: {
                    metadataEditField->setText("0.00");
                    return;
                }
                case FIELD_DATE: {
                    metadataEditField->setText("dd/mm/yyyy");
                    return;
                }
                case FIELD_TIME: {
                    metadataEditField->setText("hh:mm:ss");
                    return;
                }
                case FIELD_CHECKBOX: {
                    metadataEditField->setText("1|0");
                    return;
                }
                case FIELD_TEXT:
                case FIELD_TEXTBOX:
                case FIELD_SHORTTEXT:
                default: {
                    metadataEditField->setText("");
                    return;
                }
            }
        }
    }
}

void
BatchProcessingDialog::updateMetricDataTypeField() {

    RideMetric* rideM = displayNametoRideMetric(metricFieldToSet->currentText());

    // Display metric fields format and units
    if(rideM->isTime()) {
        metricDataEditField->setText("hh:mm:ss");
        metricUnitsLabel->setText("");
    } else if (rideM->isDate()) {
        metricDataEditField->setText("dd/mm/yyyy");
        metricUnitsLabel->setText("");
    } else {
        metricDataEditField->setText("0.00");
        metricUnitsLabel->setText((rideM) ? rideM->units(GlobalContext::context()->useMetricUnits) : "");
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
    case BatchProcessingDialog::exportB: {
        return "Export as " + fileFormat->currentText();
    }

    case BatchProcessingDialog::dataProcessorB: {
        return dataProcessorToRun->currentText();
    }

#ifdef GC_WANT_PYTHON
    case BatchProcessingDialog::pythonProcessorB: {
        return pythonProcessorToRun->currentText();
    }
#endif

    case BatchProcessingDialog::metadataSetB: {
        return "Update Metadata field - " + metadataFieldToSet->currentText();
    }

    case BatchProcessingDialog::metricSetB: {
        return "Set Metric Override - " + metricFieldToSet->currentText();
    }

    case BatchProcessingDialog::metricClearB: {
        return "Clear Metric Override - " + metricFieldToClear->currentText();
    }

    case BatchProcessingDialog::deleteB: {
        return "Delete";
    }
    }
    return QString("Undefined");
}

RideMetric*
BatchProcessingDialog::displayNametoRideMetric(const QString& fieldDisplayName) {

    // lookup metrics (we override them)
    RideMetricFactory& factory = RideMetricFactory::instance();
    QHash<QString, RideMetric*> metrics = factory.metricHash();

    // find the metric from the display name
    QHashIterator<QString, RideMetric*> itr(metrics);
    while (itr.hasNext()) {
        itr.next();
        if (itr.value()->name() == fieldDisplayName) {
            return itr.value();
        }
    }
    return NULL;
}

BatchProcessingDialog::bpFailureType
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
        if (aborted == true) return BatchProcessingDialog::userF; // user aborted!

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

    return BatchProcessingDialog::finishedF;
}

BatchProcessingDialog::bpFailureType
BatchProcessingDialog::deleteFiles() {

    // loop through the table and delete all selected
    for (int i = 0; i < files->invisibleRootItem()->childCount(); i++) {

        // give user a chance to abort..
        QApplication::processEvents();

        // did they?
        if (aborted == true) return BatchProcessingDialog::userF; // user aborted!

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

    return BatchProcessingDialog::finishedF;
}

BatchProcessingDialog::bpFailureType
BatchProcessingDialog::runDataProcessorOnActivities(const QString& processorName) {

    // lookup processor
    DataProcessor* dp = DataProcessorFactory::instance().getProcessors().value(processorName, NULL);

    if (!dp) return BatchProcessingDialog::noDataProcessorF; // No such data processor

    // Save the currently selected ride.
    // To achieve batch processing we are going to need to change this in the context temporarily
    // because the python processing is structured that the currently selected RideItem is the one
    // that gets processed with its associated RideFile. See #4095 for consequences on "import".
    RideItem* rideISafe = context->ride;

    // loop through the table and run the data processor on each selected activity
    for (int i = 0; i < files->invisibleRootItem()->childCount(); i++) {

        // give user a chance to abort..
        QApplication::processEvents();

        // did they?
        if (aborted == true) return BatchProcessingDialog::userF; // user aborted!

        QTreeWidgetItem* current = files->invisibleRootItem()->child(i);

        // is it selected for delete ?
        if (static_cast<QCheckBox*>(files->itemWidget(current, 0))->isChecked()) {

            RideItem *rideI = context->athlete->rideCache->getRide(current->text(1));

            if (!rideI) { failedToProcessEntry(current); continue; } // eek!

            // Set the current ride to the one being processed for the duration of the processing.
            context->ride = rideI;

            // ack ! we need to autoprocess, so open the ride
            RideFile* rideF = rideI->ride();

            if (!rideF) { failedToProcessEntry(current); continue; } // eek!

            files->setCurrentItem(current);

            // now run the data processor
            if (dp->postProcess(rideF, NULL, "PYTHON")) {
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

    // Restore the selected ride before the batch processing
    context->ride = rideISafe;

    return BatchProcessingDialog::finishedF;
}

BatchProcessingDialog::bpFailureType
BatchProcessingDialog::setMetadataForActivities(const QString& metadataFieldName,
                                                QString metadataValue) {

    // find the selected metadata field and update the display with its type
    foreach(FieldDefinition field, GlobalContext::context()->rideMetadata->getFields()) {
        if (metadataFieldName == field.name) {

            if (field.type == FIELD_TIME) {
                metadataValue.simplified().remove(' ');
                QRegularExpression re("([0-1]?[0-9]|2[0-3]):([0-5]?[0-9]):([0-5]?[0-9])");
                if (!re.match(metadataValue).hasMatch()) return BatchProcessingDialog::timeFormatF;
            }

            if (field.type == FIELD_DATE) {
                metadataValue.simplified().remove(' ');
                QRegularExpression re("([0]?[1-9]|[1][0-9]|3[0-1])/([0]?[1-9]|[1][0-2])/([0-9]{4})");
                if (!re.match(metadataValue).hasMatch()) return BatchProcessingDialog::dateFormatF;
            }
            break;
        }
    }

    // loop through the table and set the metdata on each selected activity
    for (int i = 0; i < files->invisibleRootItem()->childCount(); i++) {

        // give user a chance to abort..
        QApplication::processEvents();

        // did they?
        if (aborted == true) return BatchProcessingDialog::userF; // user aborted!

        QTreeWidgetItem* current = files->invisibleRootItem()->child(i);

        // is it selected for metadata update ?
        if (static_cast<QCheckBox*>(files->itemWidget(current, 0))->isChecked()) {

            RideItem* rideI = context->athlete->rideCache->getRide(current->text(1));

            if (!rideI) { failedToProcessEntry(current); continue; } // eek!

            // ack ! we need to autoprocess, so open the ride
            RideFile* rideF = rideI->ride();

            if (!rideF) { failedToProcessEntry(current); continue; } // eek!

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

    return BatchProcessingDialog::finishedF;
}

BatchProcessingDialog::bpFailureType
BatchProcessingDialog::setMetricFieldForActivities(const QString& metricDataFieldName,
                                                    QString metricDataValue) {

    RideMetric* rideM = displayNametoRideMetric(metricDataFieldName);

    if (!rideM) return BatchProcessingDialog::noRideMFoundF; // eek!

    // check time format and calculate number of seconds to simplify the data input for the user.
    if (rideM->isTime()) {

        metricDataValue.simplified().remove(' ');
        QRegularExpression re("([0-1]?[0-9]|2[0-3]):([0-5]?[0-9]):([0-5]?[0-9])");
        QRegularExpressionMatch rem = re.match(metricDataValue);

        if (rem.hasMatch()) {
            int hours = rem.captured(1).toInt();
            int mins = rem.captured(2).toInt();
            int secs = rem.captured(3).toInt();

            metricDataValue = QString("%1").arg((hours * 3600) + (mins * 60) + secs);

        } else {
            return BatchProcessingDialog::timeFormatF;
        }
    }

    // No metric date fields are available for batch processing so this shouldn't occur
    if (rideM->isDate()) return BatchProcessingDialog::dateFormatF;

    // loop through the table and set the metric override on each selected activity
    for (int i = 0; i < files->invisibleRootItem()->childCount(); i++) {

        // give user a chance to abort..
        QApplication::processEvents();

        // did they?
        if (aborted == true) return BatchProcessingDialog::userF; // user aborted!

        QTreeWidgetItem* current = files->invisibleRootItem()->child(i);

        // is it selected for metric override ?
        if (static_cast<QCheckBox*>(files->itemWidget(current, 0))->isChecked()) {

            RideItem* rideI = context->athlete->rideCache->getRide(current->text(1));

            if (!rideI) { failedToProcessEntry(current); continue; } // eek!

            // ack ! we need to autoprocess, so open the ride
            RideFile* rideF = rideI->ride();

            if (!rideF) { failedToProcessEntry(current); continue; } // eek!

            // set metric override
            QMap<QString, QString> override;
            override.insert("value", QString("%1").arg(metricDataValue.toDouble() / rideM->conversion()));
            rideF->metricOverrides.insert(rideM->symbol(), override);

            // rideFile is now dirty!
            rideI->setDirty(true);

            // get refresh done, coz overrides state has changed
            rideI->notifyRideMetadataChanged();

            // Update the files entry
            current->setText(4, tr("Metric Override Set"));
            processed++;
        }
    }

    return BatchProcessingDialog::finishedF;
}

BatchProcessingDialog::bpFailureType
BatchProcessingDialog::clearMetricFieldForActivities(const QString& metricDataFieldName) {

    RideMetric* rideM = displayNametoRideMetric(metricDataFieldName);

    if (!rideM) return BatchProcessingDialog::noRideMFoundF; // eek!

    // loop through the table and clear the metdata on each selected activity
    for (int i = 0; i < files->invisibleRootItem()->childCount(); i++) {

        // give user a chance to abort..
        QApplication::processEvents();

        // did they?
        if (aborted == true) return BatchProcessingDialog::userF; // user aborted!

        QTreeWidgetItem* current = files->invisibleRootItem()->child(i);

        // is it selected for metadata clearing ?
        if (static_cast<QCheckBox*>(files->itemWidget(current, 0))->isChecked()) {

            RideItem* rideI = context->athlete->rideCache->getRide(current->text(1));

            if (!rideI) { failedToProcessEntry(current); continue; } // eek!

            // ack ! we need to autoprocess, so open the ride
            RideFile* rideF = rideI->ride();

            if (!rideF) { failedToProcessEntry(current); continue; } // eek!

            // now clear the override 
            if (rideM) {

                rideF->metricOverrides.remove(rideM->symbol());

                // rideFile is now dirty!
                rideI->setDirty(true);

                // get refresh done, coz overrides state has changed
                rideI->notifyRideMetadataChanged();

                // Update the files entry
                current->setText(4, tr("Metric Override Removed"));
                processed++;

            } else {
                failedToProcessEntry(current);
            }
        }
    }

    return BatchProcessingDialog::finishedF;
}

void
BatchProcessingDialog::failedToProcessEntry(QTreeWidgetItem* current) {
    current->setText(4, tr("Failed to process activity"));
    fails++;
}

