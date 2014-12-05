
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

#include "BatchExportDialog.h"
#include "MainWindow.h"
#include "Context.h"
#include "Athlete.h"
#include "RideCache.h"

BatchExportDialog::BatchExportDialog(Context *context) : QDialog(context->mainWindow), context(context)
{
    setAttribute(Qt::WA_DeleteOnClose);
    //setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint); // must stop using this flag!
    setWindowTitle(tr("Ride Batch Export"));

    // make the dialog a resonable size
    setMinimumWidth(550);
    setMinimumHeight(400);

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    files = new QTreeWidget;
    files->headerItem()->setText(0, tr(""));
    files->headerItem()->setText(1, tr("Filename"));
    files->headerItem()->setText(2, tr("Date"));
    files->headerItem()->setText(3, tr("Time"));
    files->headerItem()->setText(4, tr("Action"));

    files->setColumnCount(5);
    files->setColumnWidth(0, 30); // selector
    files->setColumnWidth(1, 190); // filename
    files->setColumnWidth(2, 95); // date
    files->setColumnWidth(3, 90); // time
    files->setSelectionMode(QAbstractItemView::SingleSelection);
    files->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    files->setUniformRowHeights(true);
    files->setIndentation(0);

    // populate with each ride in the ridelist
    foreach (RideItem *rideItem, context->athlete->rideCache->rides()) {

        QTreeWidgetItem *add = new QTreeWidgetItem(files->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        // selector
        QCheckBox *checkBox = new QCheckBox("", this);
        checkBox->setChecked(true);
        files->setItemWidget(add, 0, checkBox);

        // we will wipe the original file
        add->setText(1, rideItem->fileName);
        add->setText(2, rideItem->dateTime.toString(tr("dd MMM yyyy")));
        add->setText(3, rideItem->dateTime.toString("hh:mm:ss"));

        // interval action
        add->setText(4, tr("Export"));
    }

    // format and directory
    QGridLayout *grid = new QGridLayout;
    formatLabel = new QLabel(tr("Export as"), this);
    format = new QComboBox(this);

    const RideFileFactory &rff = RideFileFactory::instance();
    foreach(QString suffix, rff.writeSuffixes()) format->addItem(rff.description(suffix));
    format->setCurrentIndex(appsettings->value(this, GC_BE_LASTFMT, "0").toInt());

    selectDir = new QPushButton(tr("Browse"), this);
    dirLabel = new QLabel (tr("Export to"), this);

    // default to last used
    QString dirDefault = appsettings->value(this, GC_BE_LASTDIR, QDir::home().absolutePath()).toString();
    if (!QDir(dirDefault).exists()) dirDefault = QDir::home().absolutePath();

    dirName = new QLabel(dirDefault, this);
    all = new QCheckBox(tr("check/uncheck all"), this);
    all->setChecked(true);

    grid->addWidget(formatLabel, 0,0, Qt::AlignLeft);
    grid->addWidget(format, 0,1, Qt::AlignLeft);
    grid->addWidget(dirLabel, 1,0, Qt::AlignLeft);
    grid->addWidget(dirName, 1,1, Qt::AlignLeft);
    grid->addWidget(selectDir, 1,2, Qt::AlignLeft);
    grid->addWidget(all, 2,0, Qt::AlignLeft);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 10);

    // buttons
    QHBoxLayout *buttons = new QHBoxLayout;
    status = new QLabel("", this);
    status->hide();
    overwrite = new QCheckBox(tr("Overwrite existing files"), this);
    cancel = new QPushButton(tr("Cancel"), this);
    ok = new QPushButton(tr("Export"), this);
    buttons->addWidget(overwrite);
    buttons->addWidget(status);
    buttons->addStretch();
    buttons->addWidget(cancel);
    buttons->addWidget(ok);

    layout->addLayout(grid);
    layout->addWidget(files);
    layout->addLayout(buttons);

    exports = fails = 0;

    // connect signals and slots up..
    connect(selectDir, SIGNAL(clicked()), this, SLOT(selectClicked()));
    connect(ok, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(all, SIGNAL(stateChanged(int)), this, SLOT(allClicked()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
}

void
BatchExportDialog::selectClicked()
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
BatchExportDialog::allClicked()
{
    // set/uncheck all rides according to the "all"
    bool checked = all->isChecked();

    for(int i=0; i<files->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *current = files->invisibleRootItem()->child(i);
        static_cast<QCheckBox*>(files->itemWidget(current,0))->setChecked(checked);
    }
}

void
BatchExportDialog::okClicked()
{
    if (ok->text() == "Export" || ok->text() == tr("Export")) {
        aborted = false;

        overwrite->hide();
        status->setText(tr("Exporting..."));
        status->show();
        cancel->hide();
        ok->setText(tr("Abort"));
        appsettings->setValue(GC_BE_LASTDIR, dirName->text());
        appsettings->setValue(GC_BE_LASTFMT, format->currentIndex());
        exportFiles();
        status->setText(QString(tr("%1 rides exported, %2 failed or skipped.")).arg(exports).arg(fails));
        ok->setText(tr("Finish"));

    } else if (ok->text() == "Abort" || ok->text() == tr("Abort")) {
        aborted = true;
    } else if (ok->text() == "Finish" || ok->text() == tr("Finish")) {
        accept(); // our work is done!
    }
}

void
BatchExportDialog::cancelClicked()
{
    reject();
}

void
BatchExportDialog::exportFiles()
{
    // what format to export as?
    QString type = RideFileFactory::instance().writeSuffixes().at(format->currentIndex());

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
                bool success = RideFileFactory::instance().writeRideFile(context, ride, out, type);

                if (success) {
                    exports++;
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
}
