
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
#ifdef GC_WANT_PYTHON
#include "FixPyScriptsDialog.h"
#endif

#include <QFormLayout>
#include <QButtonGroup>
#include <QMessageBox>

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
    fileSelection->addWidget(all);
    fileSelection->addWidget(files);

    //  --------------- Export menu items -----------------------

    QHBoxLayout* exportGrid = new QHBoxLayout;
    QHBoxLayout* exportGrid1 = new QHBoxLayout;
    QHBoxLayout* exportGrid2 = new QHBoxLayout;

    QRadioButton* exportRadioBox = new QRadioButton(tr("Export"), this);
    radioGroup->addButton(exportRadioBox, int(BatchProcessingDialog::exportB));

    QLabel* formatLabel = new QLabel(tr("As"), this);

    fileFormat = new QComboBox(this);

    const RideFileFactory& rff = RideFileFactory::instance();
    fileFormat->addItem(tr("All data (CSV)"));
    foreach(QString suffix, rff.writeSuffixes()) fileFormat->addItem(rff.description(suffix));
    fileFormat->setCurrentIndex(appsettings->value(this, GC_BE_LASTFMT, "0").toInt());

    QPushButton* selectDir = new QPushButton(tr("Browse"), this);

    QLabel* dirLabel = new QLabel(tr("To"), this);

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
    radioGroup->addButton(dpRadioBox, int(BatchProcessingDialog::dataProcessorB));

    dataProcessorToRun = new QComboBox(this);

    // get the available processors
    const DataProcessorFactory& factory = DataProcessorFactory::instance();
    QMap<QString, DataProcessor*> processors = factory.getProcessors();

    // iterate over all the processors and add an entry for each data processor
    QMapIterator<QString, DataProcessor*> i(processors);
    i.toFront();
    while (i.hasNext()) {
        i.next();
        dataProcessorToRun->addItem(i.value()->name(), i.key());
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

    //  --------------- Delete menu items -----------------------

    QHBoxLayout* deleteGrid = new QHBoxLayout;
    deleteGrid->setContentsMargins(0, 0, 0, 0);

    QRadioButton* deleteRadioBox = new QRadioButton(tr("Delete All Selected Activities"), this);
    radioGroup->addButton(deleteRadioBox, int(BatchProcessingDialog::deleteB));

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
            QOverload<int>::of(&QCheckBox::stateChanged),
            [=](int) { this->fileSelected(current); });
    }

    // Data processor signals
    connect(dpButton, SIGNAL(clicked()), this, SLOT(dpButtonClicked()));
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
    comboSelected();
    updateNumberSelected();
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
}

void
BatchProcessingDialog::radioClicked(int buttonId) {

    // ensures the Action info column matches the processing option selected
    outputMode = batchRadioBType(buttonId);
    updateActionColumn();

    // enable only useful widgets
    switch (outputMode) {
    case BatchProcessingDialog::exportB:
        exportContainer->setEnabled(true);
        dpContainer->setEnabled(false);
	break;
    case BatchProcessingDialog::dataProcessorB:
        exportContainer->setEnabled(false);
        dpContainer->setEnabled(true);
	break;
    case BatchProcessingDialog::deleteB:
        exportContainer->setEnabled(false);
        dpContainer->setEnabled(false);
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
        bpFailureType result = BatchProcessingDialog::unknownF;

        QString summaryType(tr("Processed "));

        // call the selected type of batch processing
        switch (outputMode) {
            case BatchProcessingDialog::exportB: {
                result = exportFiles();
                summaryType = tr("Exported ");
            } break;

            case BatchProcessingDialog::dataProcessorB: {
                result = runDataProcessorOnActivities(dataProcessorToRun->currentData().toString());
            } break;

            case BatchProcessingDialog::deleteB: {
                result = deleteFiles();
                summaryType = tr("Deleted ");
            } break;
        }
    
        switch (result) {

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
        return tr("Export as ") + fileFormat->currentText();
    }

    case BatchProcessingDialog::dataProcessorB: {
        return dataProcessorToRun->currentText();
    }

    case BatchProcessingDialog::deleteB: {
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

    QMessageBox msgBox;
    msgBox.setText(tr("Are you sure you want to delete all selected activities?"));
    QPushButton *deleteButton = msgBox.addButton(tr("Delete"),QMessageBox::YesRole);
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

    // get processor config
    DataProcessorConfig *config = dp->processorConfig(this);

    // Allow the user to set parameters and confirm or cancel
    ManualDataProcessorDialog *p = new ManualDataProcessorDialog(context, processorName, nullptr, config);
    p->setWindowModality(Qt::ApplicationModal); // don't allow select other ride or it all goes wrong!
    bool ok = p->exec();

    // loop through the table and run the data processor on each selected activity
    for (int i = 0; ok && i < files->invisibleRootItem()->childCount(); i++) {

        // give user a chance to abort..
        QApplication::processEvents();

        // did they?
        if (aborted == true) return BatchProcessingDialog::userF; // user aborted!

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

    return BatchProcessingDialog::finishedF;
}

void
BatchProcessingDialog::failedToProcessEntry(QTreeWidgetItem* current) {
    current->setText(4, tr("Failed to process activity"));
    fails++;
}
