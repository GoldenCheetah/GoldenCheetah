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
#include "TPDownloadDialog.h"
#include "TPDownload.h"
#include "MainWindow.h"
#include "Context.h"
#include "Athlete.h"
#include "PwxRideFile.h"
#include "JsonRideFile.h"
#include "Settings.h"
#include "Units.h"

TPDownloadDialog::TPDownloadDialog(Context *context) : QDialog(context->mainWindow, Qt::Dialog), context(context), downloading(false), aborted(false)
{
    setWindowTitle(tr("Synchronise TrainingPeaks.com"));

    athleter = new TPAthlete(this);

    connect (athleter, SIGNAL(completed(QString, QList<QMap<QString,QString> >)), this, SLOT(completedAthlete(QString, QList<QMap<QString,QString> >)));
    athleter->list(appsettings->cvalue(context->athlete->cyclist, GC_TPTYPE, "0").toInt(),
                  appsettings->cvalue(context->athlete->cyclist, GC_TPUSER, "null").toString(),
                  appsettings->cvalue(context->athlete->cyclist, GC_TPPASS, "null").toString());

    setMinimumSize(850,450);
    QWidget::hide(); // don't show just yet...
    QApplication::processEvents();
}

void
TPDownloadDialog::completedAthlete(QString errorStr, QList<QMap<QString, QString> >athletes)
{
    // did we get any athletes?
    if (athletes.count() == 0) {
        QWidget::hide(); // meh

        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Download from TrainingPeaks.com"));
        if (errorStr.size() != 0) {   // Something went wrong, so there should be a fault message
            msgBox.setText(errorStr);
        }
        else {
            msgBox.setText(tr("You must be a premium member to download from TrainingPeaks. Please check your cyclist configurations are correct on the Passwords tab."));
        }
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        reject();
        return;
    }

    // setup tabs
    tabs = new QTabWidget(this);
    QWidget * upload = new QWidget(this);
    QWidget * download = new QWidget(this);
    QWidget * sync = new QWidget(this);
    tabs->addTab(download, tr("Download"));
    tabs->addTab(upload, tr("Upload"));
    tabs->addTab(sync, tr("Synchronize"));
    tabs->setCurrentIndex(2);
    QVBoxLayout *downloadLayout = new QVBoxLayout(download);
    QVBoxLayout *uploadLayout = new QVBoxLayout(upload);
    QVBoxLayout *syncLayout = new QVBoxLayout(sync);

    workouter = new TPWorkout(this);
    connect (workouter, SIGNAL(completed(QList<QMap<QString,QString> >)), this,
            SLOT(completedWorkout(QList<QMap<QString,QString> >)));

    downloader = new TPDownload(this);
    connect (downloader, SIGNAL(completed(QDomDocument)), this,
            SLOT(completedDownload(QDomDocument)));

    uploader = new TPUpload(this);
    connect (uploader, SIGNAL(completed(QString)), this,
            SLOT(completedUpload(QString)));

    // OK! Lets build up that dialog box
    athlete = athletes[0];

    // combo box
    athleteCombo = new QComboBox(this);
    for (int i=0; i< athletes.count(); i++) {

        QString name = QString("%1 %2").arg(athletes[i].value("FirstName"))
                                   .arg(athletes[i].value("LastName"));
        int id = athletes[i].value("PersonId").toInt();
        athleteCombo->addItem(name, id);
    }
    athleteCombo->setCurrentIndex(0);

    QLabel *fromLabel = new QLabel(tr("From:"), this);
    QLabel *toLabel = new QLabel(tr("To:"), this);

    from = new QDateEdit(this);
    from->setDate(QDate::currentDate().addMonths(-1));
    from->setCalendarPopup(true);
    to = new QDateEdit(this);
    to->setDate(QDate::currentDate());
    to->setCalendarPopup(true);

    // Buttons
    refreshButton = new QPushButton(tr("Refresh List"), this);
    cancelButton = new QPushButton(tr("Close"),this);
    downloadButton = new QPushButton(tr("Download"),this);

    selectAll = new QCheckBox(tr("Select all"), this);
    selectAll->setChecked(Qt::Unchecked);

    // ride list
    rideList = new QTreeWidget(this);
    rideList->headerItem()->setText(0, " ");
    rideList->headerItem()->setText(1, "Workout Id");
    rideList->headerItem()->setText(2, "Date");
    rideList->headerItem()->setText(3, "Time");
    rideList->headerItem()->setText(4, "Duration");
    rideList->headerItem()->setText(5, "Distance");
    rideList->headerItem()->setText(6, "Exists");
    rideList->headerItem()->setText(7, "Status");
    rideList->setColumnCount(8);
    rideList->setSelectionMode(QAbstractItemView::SingleSelection);
    rideList->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    rideList->setUniformRowHeights(true);
    rideList->setIndentation(0);
    rideList->header()->resizeSection(0,20);
    rideList->header()->resizeSection(1,90);
    rideList->header()->resizeSection(2,100);
    rideList->header()->resizeSection(3,100);
    rideList->header()->resizeSection(4,100);
    rideList->header()->resizeSection(5,70);
    rideList->header()->resizeSection(6,50);
    rideList->setSortingEnabled(true);

    downloadLayout->addWidget(selectAll);
    downloadLayout->addWidget(rideList);

    selectAllUp = new QCheckBox(tr("Select all"), this);
    selectAllUp->setChecked(Qt::Unchecked);

    // ride list
    rideListUp = new QTreeWidget(this);
    rideListUp->headerItem()->setText(0, " ");
    rideListUp->headerItem()->setText(1, "File");
    rideListUp->headerItem()->setText(2, "Date");
    rideListUp->headerItem()->setText(3, "Time");
    rideListUp->headerItem()->setText(4, "Duration");
    rideListUp->headerItem()->setText(5, "Distance");
    rideListUp->headerItem()->setText(6, "Exists");
    rideListUp->headerItem()->setText(7, "Status");
    rideListUp->setColumnCount(8);
    rideListUp->setSelectionMode(QAbstractItemView::SingleSelection);
    rideListUp->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    rideListUp->setUniformRowHeights(true);
    rideListUp->setIndentation(0);
    rideListUp->header()->resizeSection(0,20);
    rideListUp->header()->resizeSection(1,200);
    rideListUp->header()->resizeSection(2,100);
    rideListUp->header()->resizeSection(3,100);
    rideListUp->header()->resizeSection(4,100);
    rideListUp->header()->resizeSection(5,70);
    rideListUp->header()->resizeSection(6,50);
    rideListUp->setSortingEnabled(true);

    uploadLayout->addWidget(selectAllUp);
    uploadLayout->addWidget(rideListUp);

    selectAllSync = new QCheckBox(tr("Select all"), this);
    selectAllSync->setChecked(Qt::Unchecked);
    syncMode = new QComboBox(this);
    syncMode->addItem(tr("Keep all do not delete"));
    syncMode->addItem(tr("Keep TP.com but delete Local"));
    syncMode->addItem(tr("Keep Local but delete TP.com"));
    QHBoxLayout *syncList = new QHBoxLayout;
    syncList->addWidget(selectAllSync);
    syncList->addStretch();
    syncList->addWidget(syncMode);


    // ride list
    rideListSync = new QTreeWidget(this);
    rideListSync->headerItem()->setText(0, " ");
    rideListSync->headerItem()->setText(1, "Source");
    rideListSync->headerItem()->setText(2, "Date");
    rideListSync->headerItem()->setText(3, "Time");
    rideListSync->headerItem()->setText(4, "Duration");
    rideListSync->headerItem()->setText(5, "Distance");
    rideListSync->headerItem()->setText(6, "Action");
    rideListSync->headerItem()->setText(7, "Status");
    rideListSync->setColumnCount(8);
    rideListSync->setSelectionMode(QAbstractItemView::SingleSelection);
    rideListSync->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    rideListSync->setUniformRowHeights(true);
    rideListSync->setIndentation(0);
    rideListSync->header()->resizeSection(0,20);
    rideListSync->header()->resizeSection(1,200);
    rideListSync->header()->resizeSection(2,100);
    rideListSync->header()->resizeSection(3,100);
    rideListSync->header()->resizeSection(4,100);
    rideListSync->header()->resizeSection(5,70);
    rideListSync->header()->resizeSection(6,100);
    rideListSync->setSortingEnabled(true);

    syncLayout->addLayout(syncList);
    syncLayout->addWidget(rideListSync);

    // show progress
    progressBar = new QProgressBar(this);
    progressLabel = new QLabel("Initial", this);

    overwrite = new QCheckBox(tr("Overwrite existing files"), this);

    // layout the widget now...
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *topline = new QHBoxLayout;
    topline->addWidget(athleteCombo);
    topline->addStretch();
    topline->addWidget(fromLabel);
    topline->addWidget(from);
    topline->addWidget(toLabel);
    topline->addWidget(to);
    topline->addStretch();
    topline->addWidget(refreshButton);


    QHBoxLayout *botline = new QHBoxLayout;
    botline->addWidget(progressLabel);
    botline->addStretch();
    botline->addWidget(overwrite);
    botline->addWidget(cancelButton);
    botline->addWidget(downloadButton);

    mainLayout->addLayout(topline);
    mainLayout->addWidget(tabs);
    mainLayout->addWidget(progressBar);
    mainLayout->addLayout(botline);


    connect (cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect (refreshButton, SIGNAL(clicked()), this, SLOT(refreshClicked()));
    connect (selectAll, SIGNAL(stateChanged(int)), this, SLOT(selectAllChanged(int)));
    connect (selectAllUp, SIGNAL(stateChanged(int)), this, SLOT(selectAllUpChanged(int)));
    connect (selectAllSync, SIGNAL(stateChanged(int)), this, SLOT(selectAllSyncChanged(int)));
    connect (downloadButton, SIGNAL(clicked()), this, SLOT(downloadClicked()));
    connect (tabs, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
    QWidget::show();

    // refresh anyway
    refreshClicked();
}

void
TPDownloadDialog::cancelClicked()
{
    reject();
}

void
TPDownloadDialog::refreshClicked()
{
    progressLabel->setText(tr("Downloading list..."));
    progressBar->setMinimum(0);
    progressBar->setMaximum(1);
    progressBar->setValue(0);

    // wipe out current
    foreach (QTreeWidgetItem *curr, rideList->invisibleRootItem()->takeChildren()) {
        QCheckBox *check = (QCheckBox*)rideList->itemWidget(curr, 0);
        QCheckBox *exists = (QCheckBox*)rideList->itemWidget(curr, 6);
        delete check;
        delete exists;
        delete curr;
    }
    foreach (QTreeWidgetItem *curr, rideListUp->invisibleRootItem()->takeChildren()) {
        QCheckBox *check = (QCheckBox*)rideListUp->itemWidget(curr, 0);
        QCheckBox *exists = (QCheckBox*)rideListUp->itemWidget(curr, 6);
        delete check;
        delete exists;
        delete curr;
    }
    foreach (QTreeWidgetItem *curr, rideListSync->invisibleRootItem()->takeChildren()) {
        QCheckBox *check = (QCheckBox*)rideListSync->itemWidget(curr, 0);
        delete check;
        delete curr;
    }

    // First lets kick off a download of ridefile lookups
    // since that can take a while
    workouter->list(
                  athleteCombo->itemData(athleteCombo->currentIndex()).toInt(),
                  from->date(),
                  to->date(),
                  appsettings->cvalue(context->athlete->cyclist, GC_TPUSER, "null").toString(),
                  appsettings->cvalue(context->athlete->cyclist, GC_TPPASS, "null").toString());

    // Whilst we wait for the results lets fill the map of existing rideFiles
    // (but ignore seconds since they aren't reliable)
    rideFiles.clear();

    Specification specification;
    specification.setDateRange(DateRange(from->date(), to->date()));
    foreach(RideItem *item, context->athlete->rideCache->rides()) {
        if (specification.pass(item))
            rideFiles << QFileInfo(item->fileName).baseName().mid(0,14);
    }

}

void
TPDownloadDialog::tabChanged(int idx)
{
    if (downloadButton->text() == "Abort") return;

    switch (idx) {

    case 0 : // download
        downloadButton->setText(tr("Download"));
        refreshCount();
        break;
    case 1 : // upload
        downloadButton->setText(tr("Upload"));
        refreshUpCount();
        break;
    case 2 : // synchronise
        downloadButton->setText(tr("Synchronize"));
        refreshSyncCount();
        break;
    }
}

void
TPDownloadDialog::completedWorkout(QList<QMap<QString, QString> >workouts)
{
    useMetricUnits = context->athlete->useMetricUnits;

    //
    // Setup the upload list
    //
    uploadFiles.clear();
    for(int i=0; i<workouts.count(); i++) {
        QTreeWidgetItem *add;

        add = new QTreeWidgetItem(rideList->invisibleRootItem());
        add->setFlags(add->flags() & ~Qt::ItemIsEditable);

        QCheckBox *check = new QCheckBox("", this);
        connect (check, SIGNAL(stateChanged(int)), this, SLOT(refreshCount()));
        rideList->setItemWidget(add, 0, check);

        add->setText(1, workouts[i].value("WorkoutId"));
        add->setTextAlignment(1, Qt::AlignCenter);

        QDateTime ridedatetime(QDateTime::fromString(workouts[i].value("WorkoutDay"), Qt::ISODate).date(),
                               QDateTime::fromString(workouts[i].value("StartTime"), Qt::ISODate).time());

        add->setText(2, ridedatetime.toString("MMM d, yyyy"));
        add->setTextAlignment(2, Qt::AlignLeft);
        add->setText(3, ridedatetime.toString("hh:mm:ss"));
        add->setTextAlignment(3, Qt::AlignCenter);

        long secs = workouts[i].value("TimeTotalInSeconds").toInt();
        QChar zero = QLatin1Char ( '0' );
        QString duration = QString("%1:%2:%3").arg(secs/3600,2,10,zero)
                                          .arg(secs%3600/60,2,10,zero)
                                          .arg(secs%60,2,10,zero);
        add->setText(4, duration);
        add->setTextAlignment(4, Qt::AlignCenter);

        double distance = workouts[i].value("DistanceInMeters").toDouble() / 1000.00;
	if (useMetricUnits) {
	  add->setText(5, QString("%1 km").arg(distance, 0, 'f', 1));
	  } else {
	  add->setText(5, QString("%1 mi").arg(distance*MILES_PER_KM, 0, 'f', 1));
	  }
	  add->setTextAlignment(5, Qt::AlignRight);

        QString targetnosuffix = QString ( "%1_%2_%3_%4_%5_%6" )
                           .arg ( ridedatetime.date().year(), 4, 10, zero )
                           .arg ( ridedatetime.date().month(), 2, 10, zero )
                           .arg ( ridedatetime.date().day(), 2, 10, zero )
                           .arg ( ridedatetime.time().hour(), 2, 10, zero )
                           .arg ( ridedatetime.time().minute(), 2, 10, zero )
                           .arg ( ridedatetime.time().second(), 2, 10, zero );

        uploadFiles << targetnosuffix.mid(0,14);

        // exists? - we ignore seconds, since TP seems to do odd
        //           things to date times and loses seconds (?)
        QCheckBox *exists = new QCheckBox("", this);
        exists->setEnabled(false);
        rideList->setItemWidget(add, 6, exists);
        add->setTextAlignment(6, Qt::AlignCenter);
        if (rideFiles.contains(targetnosuffix.mid(0,14))) exists->setChecked(Qt::Checked);
        else {
            exists->setChecked(Qt::Unchecked);

            // doesn't exist -- add it to the sync list too then
            QTreeWidgetItem *sync = new QTreeWidgetItem(rideListSync->invisibleRootItem());

            QCheckBox *check = new QCheckBox("", this);
            connect (check, SIGNAL(stateChanged(int)), this, SLOT(refreshSyncCount()));
            rideListSync->setItemWidget(sync, 0, check);

            sync->setText(1, workouts[i].value("WorkoutId"));
            sync->setTextAlignment(1, Qt::AlignCenter);
            sync->setText(2, ridedatetime.toString("MMM d, yyyy"));
            sync->setTextAlignment(2, Qt::AlignLeft);
            sync->setText(3, ridedatetime.toString("hh:mm:ss"));
            sync->setTextAlignment(3, Qt::AlignCenter);
            sync->setText(4, duration);
            sync->setTextAlignment(4, Qt::AlignCenter);
            sync->setText(5, QString("%1 km").arg(distance, 0, 'f', 1));
            sync->setTextAlignment(5, Qt::AlignRight);
            sync->setText(6, tr("Download"));
            sync->setTextAlignment(6, Qt::AlignLeft);
            sync->setText(7, "");
        }

        add->setText(7, "");
    }

    //
    // Now setup the upload list
    //
    Specification specification;
    specification.setDateRange(DateRange(from->date(), to->date()));
    for(int i=0; i<context->athlete->rideCache->rides().count(); i++) {

        RideItem *ride = context->athlete->rideCache->rides().at(i);
        if (!specification.pass(ride)) continue;

        QTreeWidgetItem *add;

        add = new QTreeWidgetItem(rideListUp->invisibleRootItem());
        add->setFlags(add->flags() & ~Qt::ItemIsEditable);

        QCheckBox *check = new QCheckBox("", this);
        connect (check, SIGNAL(stateChanged(int)), this, SLOT(refreshUpCount()));
        rideListUp->setItemWidget(add, 0, check);

        add->setText(1, ride->fileName);
        add->setTextAlignment(1, Qt::AlignLeft);
        add->setText(2, ride->dateTime.toString("MMM d, yyyy"));
        add->setTextAlignment(2, Qt::AlignLeft);
        add->setText(3, ride->dateTime.toString("hh:mm:ss"));
        add->setTextAlignment(3, Qt::AlignCenter);

        long secs = ride->getForSymbol("workout_time");
        QChar zero = QLatin1Char ( '0' );
        QString duration = QString("%1:%2:%3").arg(secs/3600,2,10,zero)
                                          .arg(secs%3600/60,2,10,zero)
                                          .arg(secs%60,2,10,zero);
        add->setText(4, duration);
        add->setTextAlignment(4, Qt::AlignCenter);

        double distance = ride->getForSymbol("total_distance");
        add->setText(5, QString("%1 km").arg(distance, 0, 'f', 1));
        add->setTextAlignment(5, Qt::AlignRight);

        // exists? - we ignore seconds, since TP seems to do odd
        //           things to date times and loses seconds (?)
        QCheckBox *exists = new QCheckBox("", this);
        exists->setEnabled(false);
        rideListUp->setItemWidget(add, 6, exists);
        add->setTextAlignment(6, Qt::AlignCenter);

        QString targetnosuffix = QString ( "%1_%2_%3_%4_%5_%6" )
                           .arg ( ride->dateTime.date().year(), 4, 10, zero )
                           .arg ( ride->dateTime.date().month(), 2, 10, zero )
                           .arg ( ride->dateTime.date().day(), 2, 10, zero )
                           .arg ( ride->dateTime.time().hour(), 2, 10, zero )
                           .arg ( ride->dateTime.time().minute(), 2, 10, zero )
                           .arg ( ride->dateTime.time().second(), 2, 10, zero );

        // check if on TP.com already
        if (uploadFiles.contains(targetnosuffix.mid(0,14))) exists->setChecked(Qt::Checked);
        else {
            exists->setChecked(Qt::Unchecked);

            // doesn't exist -- add it to the sync list too then
            QTreeWidgetItem *sync = new QTreeWidgetItem(rideListSync->invisibleRootItem());

            QCheckBox *check = new QCheckBox("", this);
            connect (check, SIGNAL(stateChanged(int)), this, SLOT(refreshSyncCount()));
            rideListSync->setItemWidget(sync, 0, check);

            sync->setText(1, ride->fileName);
            sync->setTextAlignment(1, Qt::AlignCenter);
            sync->setText(2, ride->dateTime.toString("MMM d, yyyy"));
            sync->setTextAlignment(2, Qt::AlignLeft);
            sync->setText(3, ride->dateTime.toString("hh:mm:ss"));
            sync->setTextAlignment(3, Qt::AlignCenter);
            sync->setText(4, duration);
            sync->setTextAlignment(4, Qt::AlignCenter);
            sync->setText(5, QString("%1 km").arg(distance, 0, 'f', 1));
            sync->setTextAlignment(5, Qt::AlignRight);
            sync->setText(6, tr("Upload"));
            sync->setTextAlignment(6, Qt::AlignLeft);
            sync->setText(7, "");
        }
        add->setText(7, "");
    }

    // refresh the progress label
    tabChanged(tabs->currentIndex());
}

void
TPDownloadDialog::selectAllChanged(int state)
{
    for (int i=0; i<rideList->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideList->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideList->itemWidget(curr, 0);
        check->setChecked(state);
    }
}

void
TPDownloadDialog::selectAllUpChanged(int state)
{
    for (int i=0; i<rideListUp->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListUp->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListUp->itemWidget(curr, 0);
        check->setChecked(state);
    }
}

void
TPDownloadDialog::selectAllSyncChanged(int state)
{
    for (int i=0; i<rideListSync->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListSync->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListSync->itemWidget(curr, 0);
        check->setChecked(state);
    }
}

void
TPDownloadDialog::refreshUpCount()
{
    int selected = 0;

    for (int i=0; i<rideListUp->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListUp->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListUp->itemWidget(curr, 0);
        if (check->isChecked()) selected++;
    }
    progressLabel->setText(QString("%1 of %2 selected").arg(selected)
                            .arg(rideListUp->invisibleRootItem()->childCount()));
}

void
TPDownloadDialog::refreshSyncCount()
{
    int selected = 0;

    for (int i=0; i<rideListSync->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListSync->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListSync->itemWidget(curr, 0);
        if (check->isChecked()) selected++;
    }
    progressLabel->setText(QString("%1 of %2 selected").arg(selected)
                            .arg(rideListSync->invisibleRootItem()->childCount()));
}

void
TPDownloadDialog::refreshCount()
{
    int selected = 0;

    for (int i=0; i<rideList->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideList->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideList->itemWidget(curr, 0);
        if (check->isChecked()) selected++;
    }
    progressLabel->setText(QString("%1 of %2 selected").arg(selected)
                            .arg(rideList->invisibleRootItem()->childCount()));
}

void
TPDownloadDialog::downloadClicked()
{
    if (downloading == true) {
        rideList->setSortingEnabled(true);
        rideListUp->setSortingEnabled(true);
        progressLabel->setText("");
        downloadButton->setText("Download");
        downloading=false;
        aborted=true;
        cancelButton->show();
        return;
    } else {
        rideList->setSortingEnabled(false);
        rideListUp->setSortingEnabled(true);
        downloading=true;
        aborted=false;
        downloadButton->setText("Abort");
        cancelButton->hide();
    }

    // keeping track of progress...
    downloadcounter = 0;
    successful = 0;
    downloadtotal = 0;
    listindex = 0;

    QTreeWidget *which = NULL;
    switch(tabs->currentIndex()) {
        case 0 : which = rideList; break;
        case 1 : which = rideListUp; break;
        default:
        case 2 : which = rideListSync; break;
    }

    for (int i=0; i<which->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = which->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)which->itemWidget(curr, 0);
        if (check->isChecked()) {
            downloadtotal++;
        }
    }

    if (downloadtotal) {
        progressBar->setMaximum(downloadtotal);
        progressBar->setMinimum(0);
        progressBar->setValue(0);
    }

    // even if nothing to download this
    // cleans up variables et al
    sync = false;
    switch(tabs->currentIndex()) {
        case 0 : downloadNext(); break;
        case 1 : uploadNext(); break;
        case 2 : sync = true; syncNext(); break;
    }
}

bool
TPDownloadDialog::syncNext()
{
    // the actual download/upload is kicked off using the uploader / downloader
    // if in sync mode the completedDownload / completedUpload functions
    // just call completedSync to get the next Sync done
    for (int i=listindex; i<rideListSync->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListSync->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListSync->itemWidget(curr, 0);

        if (check->isChecked()) {

            listindex = i+1; // start from the next one

            progressLabel->setText(QString(tr("Processed %1 of %2")).arg(downloadcounter).arg(downloadtotal));
            if (curr->text(6) == "Download") {
                curr->setText(7, tr("Downloading"));
                rideListSync->setCurrentItem(curr);
                downloader->download(
                    context->athlete->cyclist,
                    athleteCombo->itemData(athleteCombo->currentIndex()).toInt(),
                    curr->text(1).toInt()
                    );
                QApplication::processEvents();
            } else {
                curr->setText(7, tr("Uploading"));
                rideListSync->setCurrentItem(curr);

                // read in the file
                QStringList errors;
                QFile file(context->athlete->home->activities().canonicalPath() + "/" + curr->text(1));
                RideFile *ride = RideFileFactory::instance().openRideFile(context, file, errors);

                if (ride) {
                    uploader->upload(context, ride);
                    delete ride; // clean up!
                    QApplication::processEvents();
                    return true;
                } else {
                    curr->setText(7, "Parse failure");
                    QApplication::processEvents();
                }

            }
            return true;
        }
    }

    //
    // Our work is done!
    //
    rideList->setSortingEnabled(true);
    rideListUp->setSortingEnabled(true);
    rideListSync->setSortingEnabled(true);
    progressLabel->setText(tr("Sync complete"));
    downloadButton->setText("Synchronize");
    downloading=false;
    aborted=false;
    sync=false;
    cancelButton->show();
    selectAllSync->setChecked(Qt::Unchecked);
    for (int i=0; i<rideListSync->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListSync->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListSync->itemWidget(curr, 0);
        check->setChecked(false);
    }
    progressLabel->setText(QString(tr("Processed %1 of %2 successfully")).arg(successful).arg(downloadtotal));
    return false;
}

bool
TPDownloadDialog::downloadNext()
{
    for (int i=listindex; i<rideList->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideList->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideList->itemWidget(curr, 0);
        QCheckBox *exists = (QCheckBox*)rideList->itemWidget(curr, 6);

        // skip existing if overwrite not set
        if (check->isChecked() && exists->isChecked() && !overwrite->isChecked()) {
            curr->setText(7, tr("File exists"));
            progressBar->setValue(++downloadcounter);
            continue;
        }

        if (check->isChecked()) {

            listindex = i+1; // start from the next one
            curr->setText(7, tr("Downloading"));
            rideList->setCurrentItem(curr);
            progressLabel->setText(QString(tr("Downloaded %1 of %2")).arg(downloadcounter).arg(downloadtotal));
            downloader->download(
                  context->athlete->cyclist,
                  athleteCombo->itemData(athleteCombo->currentIndex()).toInt(),
                  curr->text(1).toInt()
                  );
            QApplication::processEvents();
            return true;
        }
    }

    //
    // Our work is done!
    //
    rideList->setSortingEnabled(true);
    rideListUp->setSortingEnabled(true);
    progressLabel->setText(tr("Downloads complete"));
    downloadButton->setText("Download");
    downloading=false;
    aborted=false;
    cancelButton->show();
    selectAll->setChecked(Qt::Unchecked);
    for (int i=0; i<rideList->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideList->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideList->itemWidget(curr, 0);
        check->setChecked(false);
    }
    progressLabel->setText(QString(tr("Downloaded %1 of %2 successfully")).arg(successful).arg(downloadtotal));
    return false;
}

void
TPDownloadDialog::completedDownload(QDomDocument pwx)
{
    QTreeWidget *which = sync ? rideListSync : rideList;

    // was abort pressed?
    if (aborted == true) {
        QTreeWidgetItem *curr = which->invisibleRootItem()->child(listindex-1);
        curr->setText(7, tr("Aborted"));
        return;
    }

    // update status...
    // validate (parse)
    QStringList errors;
    PwxFileReader reader;
    RideFile *ride = reader.PwxFromDomDoc(pwx, errors);

    progressBar->setValue(++downloadcounter);

    QTreeWidgetItem *curr = which->invisibleRootItem()->child(listindex-1);
    if (ride) {
        if (saveRide(ride, pwx, errors) == true) {
            curr->setText(7, tr("Saved"));
            successful++;
        } else {
            curr->setText(7, errors.join(","));
        }
    } else {
        QString err = "Failed:" + errors.join(",");
        curr->setText(7, err);
    }
    QApplication::processEvents();

    if (sync)
        syncNext();
    else
        downloadNext();
}

bool
TPDownloadDialog::uploadNext()
{
    for (int i=listindex; i<rideListUp->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListUp->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListUp->itemWidget(curr, 0);
        QCheckBox *exists = (QCheckBox*)rideListUp->itemWidget(curr, 6);

        // skip existing if overwrite not set
        if (check->isChecked() && exists->isChecked() && !overwrite->isChecked()) {
            curr->setText(7, tr("File exists"));
            progressBar->setValue(++downloadcounter);
            continue;
        }

        if (check->isChecked()) {

            listindex = i+1; // start from the next one
            curr->setText(7, tr("Uploading"));
            rideListUp->setCurrentItem(curr);
            progressLabel->setText(QString(tr("Uploaded %1 of %2")).arg(downloadcounter).arg(downloadtotal));

            // read in the file
            QStringList errors;
            QFile file(context->athlete->home->activities().canonicalPath() + "/" + curr->text(1));
            RideFile *ride = RideFileFactory::instance().openRideFile(context, file, errors);

            if (ride) {
                uploader->upload(context, ride);
                delete ride; // clean up!
                QApplication::processEvents();
                return true;
            } else {
                curr->setText(7, "Parse failure");
                QApplication::processEvents();
            }
        }
    }

    //
    // Our work is done!
    //
    rideList->setSortingEnabled(true);
    rideListUp->setSortingEnabled(true);
    progressLabel->setText(tr("Uploads complete"));
    downloadButton->setText("Upload");
    downloading=false;
    aborted=false;
    cancelButton->show();
    selectAllUp->setChecked(Qt::Unchecked);
    for (int i=0; i<rideListUp->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListUp->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListUp->itemWidget(curr, 0);
        check->setChecked(false);
    }
    progressLabel->setText(QString(tr("Uploaded %1 of %2 successfully")).arg(successful).arg(downloadtotal));
    return false;
}

void
TPDownloadDialog::completedUpload(QString result)
{
    QTreeWidget *which = sync ? rideListSync : rideListUp;

    // was abort pressed?
    if (aborted == true) {
        QTreeWidgetItem *curr = which->invisibleRootItem()->child(listindex-1);
        curr->setText(7, tr("Aborted"));
        return;
    }

    progressBar->setValue(++downloadcounter);

    QTreeWidgetItem *curr = which->invisibleRootItem()->child(listindex-1);
    curr->setText(7, result);
    if (result == tr("Upload successful")) successful++;
    QApplication::processEvents();

    if (sync)
        syncNext();
    else
        uploadNext();
}

bool
TPDownloadDialog::saveRide(RideFile *ride, QDomDocument &, QStringList &errors)
{
    QDateTime ridedatetime = ride->startTime();

    QChar zero = QLatin1Char ( '0' );
    QString targetnosuffix = QString ( "%1_%2_%3_%4_%5_%6" )
                           .arg ( ridedatetime.date().year(), 4, 10, zero )
                           .arg ( ridedatetime.date().month(), 2, 10, zero )
                           .arg ( ridedatetime.date().day(), 2, 10, zero )
                           .arg ( ridedatetime.time().hour(), 2, 10, zero )
                           .arg ( ridedatetime.time().minute(), 2, 10, zero )
                           .arg ( ridedatetime.time().second(), 2, 10, zero );

    QString filename = context->athlete->home->activities().canonicalPath() + "/" + targetnosuffix + ".json";

    // exists?
    QFileInfo fileinfo(filename);
    if (fileinfo.exists() && overwrite->isChecked() == false) {
        errors << "File exists";
        return false;
    }

    JsonFileReader reader;
    QFile file(filename);
    reader.writeRideFile(context, ride, file);

    // add to the ride list
    rideFiles<<targetnosuffix;
    context->athlete->addRide(fileinfo.fileName(), true);

    return true;
}
