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

#include "TrainerDayDownloadDialog.h"
#include "MainWindow.h"
#include "TrainDB.h"
#include "HelpWhatsThis.h"

TrainerDayDownloadDialog::TrainerDayDownloadDialog(Context *context) : QDialog(context->mainWindow), context(context)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    setWindowTitle(tr("Download workouts from TrainerDay"));

    // help
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Tools_Download_ERGDB));

    // make the dialog a resonable size
    setMinimumWidth(650 * dpiXFactor);
    setMinimumHeight(400 *dpiYFactor);

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    files = new QTreeWidget;
#ifdef Q_OS_MAC
    files->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    files->headerItem()->setText(0, tr(""));
    files->headerItem()->setText(1, tr("Name"));
    files->headerItem()->setText(2, tr("Type"));
    files->headerItem()->setText(3, tr("Author"));
    files->headerItem()->setText(4, tr("Dated"));
    files->headerItem()->setText(5, tr("Action"));

    files->setColumnCount(6);
    files->setColumnWidth(0, 30*dpiXFactor); // selector
    files->setColumnWidth(1, 140*dpiXFactor); // filename
    files->setColumnWidth(2, 100*dpiXFactor); // Type
    files->setColumnWidth(3, 120*dpiXFactor); // Author
    files->setColumnWidth(4, 90*dpiXFactor); // dated
    files->setSelectionMode(QAbstractItemView::SingleSelection);
    files->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    files->setUniformRowHeights(true);
    files->setIndentation(0);

    foreach(TrainerDayItem item, ergdb.items()) {

        QTreeWidgetItem *add = new QTreeWidgetItem(files->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        // selector
        QCheckBox *checkBox = new QCheckBox("", this);
        checkBox->setChecked(false);
        files->setItemWidget(add, 0, checkBox);

        add->setText(1, item.name);
        add->setText(2, item.workoutType);
        add->setText(3, item.author);
        add->setText(4, item.added.toString(tr("dd MMM yyyy")));

        // interval action
        add->setText(5, tr("Download"));

        // hide away the id
        add->setText(6, QString("%1").arg(item.id));
    }

    all = new QCheckBox(tr("check/uncheck all"), this);
    all->setChecked(false);

    // buttons
    QHBoxLayout *buttons = new QHBoxLayout;
    status = new QLabel("", this);
    status->hide();
    overwrite = new QCheckBox(tr("Overwrite existing workouts"), this);
    cancel = new QPushButton(tr("Cancel"), this);
    ok = new QPushButton(tr("Download"), this);
    buttons->addWidget(overwrite);
    buttons->addWidget(status);
    buttons->addStretch();
    buttons->addWidget(cancel);
    buttons->addWidget(ok);

    layout->addWidget(all);
    layout->addWidget(files);
    layout->addLayout(buttons);

    downloads = fails = 0;

    // connect signals and slots up..
    connect(ok, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(all, SIGNAL(stateChanged(int)), this, SLOT(allClicked()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
}

void
TrainerDayDownloadDialog::allClicked()
{
    // set/uncheck all rides according to the "all"
    bool checked = all->isChecked();

    for(int i=0; i<files->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *current = files->invisibleRootItem()->child(i);
        static_cast<QCheckBox*>(files->itemWidget(current,0))->setChecked(checked);
    }
}

void
TrainerDayDownloadDialog::okClicked()
{
    if (ok->text() == tr("Download")) {
        aborted = false;

        overwrite->hide();
        status->setText(tr("Download..."));
        status->show();
        cancel->hide();
        ok->setText(tr("Abort"));
        downloadFiles();
        status->setText(QString(tr("%1 workouts downloaded, %2 failed or skipped.")).arg(downloads).arg(fails));
        ok->setText(tr("Finish"));

        context->notifyWorkoutsChanged();

    } else if (ok->text() == tr("Abort")) {
        aborted = true;
    } else if (ok->text() == tr("Finish")) {
        accept(); // our work is done!
    }
}

void
TrainerDayDownloadDialog::cancelClicked()
{
    reject();
}

void
TrainerDayDownloadDialog::downloadFiles()
{
    // where to place them
    QString workoutDir = appsettings->value(this, GC_WORKOUTDIR).toString();

    // what format to export as?
    QString type = RideFileFactory::instance().writeSuffixes().at(0);

    // for library updating, transactional for 10x performance
    trainDB->startLUW();

    // loop through the table and export all selected
    for(int i=0; i<files->invisibleRootItem()->childCount(); i++) {

        // give user a chance to abort..
        QApplication::processEvents();

        // did they?
        if (aborted == true) {
            trainDB->endLUW(); // need to commit whatever was copied.
            return; // user aborted!
        }

        QTreeWidgetItem *current = files->invisibleRootItem()->child(i);

        // is it selected
        if (static_cast<QCheckBox*>(files->itemWidget(current,0))->isChecked()) {

            files->setCurrentItem(current); QApplication::processEvents();

            // this one then
            current->setText(5, tr("Downloading...")); QApplication::processEvents();

            // get the id
            int id = current->text(6).toInt();
            QString content = ergdb.getWorkout(id);

            QString filename = workoutDir + "/" + current->text(1) + ".erg2";
            ErgFile *p = ErgFile::fromContent2(content, context);

            // open success?
            if (p->isValid()) {


                if (QFile(filename).exists()) {

                    if (overwrite->isChecked() == false) {
                        // skip existing files
                        current->setText(5,tr("Exists already")); QApplication::processEvents();
                        fails++;
                        delete p; // free memory!
                        continue;

                    } else {

                        // remove existing
                        QFile(filename).remove();
                        current->setText(5, tr("Removing...")); QApplication::processEvents();
                    }

                }

                QFile out(filename);
                if (out.open(QIODevice::WriteOnly) == true) {

                    QTextStream output(&out);
                    output << content;
                    out.close();

                    downloads++;
                    current->setText(5, tr("Saved")); QApplication::processEvents();
                    trainDB->importWorkout(filename, *p); // add to library

                } else {

                    fails++;
                    current->setText(5, tr("Write failed")); QApplication::processEvents();
                }

                delete p; // free memory!

            // couldn't parse
            } else {

                delete p; // free memory!
                fails++;
                current->setText(5, tr("Invalid File")); QApplication::processEvents();

            }

        }
    }
    // for library updating, transactional for 10x performance
    trainDB->endLUW();
}
