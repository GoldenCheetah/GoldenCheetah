/*
 * Copyright (c) 2015 Mark Liversedge (liversedge@gmail.com)
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

#include "FileStore.h"

#include "Athlete.h"
#include "RideCache.h"
#include "RideItem.h"
#include "MainWindow.h"
#include "JsonRideFile.h"
#include "Units.h"

#include <QIcon>
#include <QFileIconProvider>
#include <QMessageBox>
#include <QHeaderView>

#include "../qzip/zipwriter.h"
#include "../qzip/zipreader.h"

//
// FILESTORE BASE CLASS
//

// nothing doing in base class, for now
FileStore::FileStore(Context *context) : context(context)
{
}

// clean up on delete
FileStore::~FileStore()
{
    foreach(FileStoreEntry *p, list_) delete p;
    list_.clear();
}

// get a new filestore entry
FileStoreEntry *
FileStore::newFileStoreEntry()
{
    FileStoreEntry *p = new FileStoreEntry();
    p->initial = true;
    list_ << p;
    return p;
}

bool
FileStore::upload(QWidget *parent, FileStore *store, RideItem *item)
{

    // open a dialog to do it
    FileStoreUploadDialog uploader(parent, store, item);
    int ret = uploader.exec();

    // was it successfull ?
    if (ret == QDialog::Accepted) return true;
    else return false;
}

void
FileStore::compressRide(RideFile*ride, QByteArray &data, QString name)
{
    // compress via a temporary file
    QTemporaryFile tempfile;
    tempfile.open();
    tempfile.close();

    // write as json
    QFile jsonFile(tempfile.fileName());
    if (RideFileFactory::instance().writeRideFile(NULL, ride, jsonFile, "json") == true) {

        // create a temp zip file
        QTemporaryFile zipFile;
        zipFile.open();
        zipFile.close();

        // add files using zip writer
        QString zipname = zipFile.fileName();
        ZipWriter writer(zipname);

        // read the ride file back and add to zip file
        jsonFile.open(QFile::ReadOnly);
        writer.addFile(name, jsonFile.readAll());
        jsonFile.close();
        writer.close();

        // now read in the zipfile
        QFile zip(zipname);
        zip.open(QFile::ReadOnly);
        data = zip.readAll();
        zip.close();
    }
}

// name is the source name (i.e. what it is called on the file store (xxxxx.json.zip)
RideFile *
FileStore::uncompressRide(QByteArray *data, QString name, QStringList &errors)
{
    // make sure its named as we expect
    if (!name.endsWith(".json.zip")) {
        errors << tr("expected compressed activity file.");
        return NULL;
    }

    // write out to a zip file first
    QTemporaryFile zipfile;
    zipfile.open();
    zipfile.write(*data);
    zipfile.close();

    // open zip
    ZipReader reader(zipfile.fileName());
    ZipReader::FileInfo info = reader.entryInfoAt(0);
    QByteArray jsonData = reader.fileData(info.filePath);

    // uncompress and write to tmp without the .zip
    name = name.mid(0, name.length()-4);
    QString tmp = context->athlete->home->temp().absolutePath() + "/" + QFileInfo(name).baseName() + "." + QFileInfo(name).suffix();
    
    // uncompress and write a file
    QFile file(tmp);
    file.open(QFile::WriteOnly);
    file.write(jsonData);
    file.close();

    // read the file in using the correct ridefile reader
    RideFile *ride = RideFileFactory::instance().openRideFile(context, file, errors);

    // remove temp
    file.remove();

    // return whatever we got
    return ride;
}

FileStoreUploadDialog::FileStoreUploadDialog(QWidget *parent, FileStore *store, RideItem *item) : QDialog(parent), store(store), item(item)
{
    // get a compressed version
    store->compressRide(item->ride(), data, QFileInfo(item->fileName).baseName() + ".json");

    // setup the gui!
    QVBoxLayout *layout = new QVBoxLayout(this);
    info = new QLabel(QString(tr("Uploading %1 bytes...")).arg(data.size()));
    layout->addWidget(info);

    progress = new QProgressBar(this);
    progress->setMaximum(0);
    progress->setValue(0);
    layout->addWidget(progress);

    okcancel = new QPushButton(tr("Cancel"));
    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(okcancel);
    layout->addLayout(buttons);

    // get notification when done
    connect(store, SIGNAL(writeComplete(QString,QString)), this, SLOT(completed(QString,QString)));

    // ok, so now we can kickoff the upload
    store->writeFile(data, QFileInfo(item->fileName).baseName() + ".json.zip");
}

void
FileStoreUploadDialog::completed(QString file, QString message)
{
    info->setText(file + "\n" + message);
    progress->setMaximum(1);
    progress->setValue(1);
    okcancel->setText(tr("OK"));
    connect(okcancel, SIGNAL(clicked()), this, SLOT(accept()));
}

FileStoreDialog::FileStoreDialog(QWidget *parent, FileStore *store, QString title, QString pathname, bool dironly) :
    QDialog(parent), store(store), title(title), pathname(pathname), dironly(dironly)
{
    //setAttribute(Qt::WA_DeleteOnClose);
    setMinimumSize(350, 400);
    setWindowTitle(title + " (" + store->name() + ")");
    QVBoxLayout *layout = new QVBoxLayout(this);

    pathEdit = new QLineEdit(this);
    pathEdit->setText(pathname);
    layout->addWidget(pathEdit);

    splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(1);

    folders = new QTreeWidget(this);
    folders->headerItem()->setText(0, tr("Folder"));
    folders->setColumnCount(1);
    folders->setSelectionMode(QAbstractItemView::SingleSelection);
    folders->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    folders->setIndentation(0);

    files = new QTreeWidget(this);
    files->headerItem()->setText(0, tr("Name"));
    files->headerItem()->setText(1, tr("Type"));
    files->headerItem()->setText(2, tr("Modified"));
    files->setColumnCount(3);
    files->setSelectionMode(QAbstractItemView::SingleSelection);
    files->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    files->setIndentation(0);

    splitter->addWidget(folders);
    splitter->addWidget(files);

    splitter->setStretchFactor(0,30);
    splitter->setStretchFactor(1,70);

    layout->addWidget(splitter);

    QHBoxLayout *buttons = new QHBoxLayout;
    create = new QPushButton(tr("Create Folder"), this);
    cancel = new QPushButton(tr("Cancel"), this);
    open = new QPushButton(tr("Open"), this);

    buttons->addWidget(create);
    buttons->addStretch();
    buttons->addWidget(cancel);
    buttons->addWidget(open);
    layout->addLayout(buttons);

    // want selection or not ?
    connect(create, SIGNAL(clicked()), this, SLOT(createFolderClicked()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(open, SIGNAL(clicked()), this, SLOT(accept()));
    connect(pathEdit, SIGNAL(returnPressed()), this, SLOT(returnPressed()));
    connect(folders, SIGNAL(itemSelectionChanged()), this, SLOT(folderSelectionChanged()));
    connect(files, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(fileDoubleClicked(QTreeWidgetItem*,int)));

    // trap return key pressed for a file dialog
    installEventFilter(this);

    // set path to selected
    setPath(pathname);
}

void
FileStoreDialog::returnPressed()
{
    setPath(pathEdit->text());
}

// set path
void 
FileStoreDialog::setPath(QString path, bool refresh)
{
    QStringList errors; // keep a track of errors
    QString pathing; // keeping a track of the path we have followed

    // get root
    FileStoreEntry *fse = store->root();

    // is it NULL!?
    if (fse == NULL) return;

    // get list of paths to travers
    QStringList paths = path.split("/");

    // remove first and last blanks that are caused
    // by path beginning and ending in a "/"
    if (paths.count() && paths.first() == "") paths.removeAt(0);
    if (paths.count() && paths.last() == "") paths.removeAt(paths.count()-1);

    // start at root
    pathing = "/";
    if (refresh || fse->initial == true) {
        fse->children = store->readdir(pathing, errors);
        if (errors.count() == 0) {
            fse->initial = false;

            // initialise the folders list
            setFolders(fse);
        }
    }

    // traverse the paths to the destination
    foreach(QString directory, paths) {

        // find the directory in children
        int index = fse->child(directory);

        // not found!
        if (index == -1) break;

        // update pathing
        if (!pathing.endsWith("/")) pathing += "/";
        pathing += directory;

        // drop into directory and refresh if needed
        fse = fse->children[index];
        if (refresh || fse->initial == true) {
            fse->children = store->readdir(pathing, errors);
            if (errors.count() == 0) fse->initial = false;
        }
    }

    // reset to where we got
    pathEdit->setText(pathing);
    pathname=pathing;
    setFiles(fse);
}

bool FileStoreDialog::eventFilter(QObject *obj, QEvent *evt)
{
    if (obj != this) return false;

    if(evt->type() == QEvent::KeyPress) {

        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(evt);

        // ignore it !
        if(keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return )
            return true; 
    }

    // do the usual thing.
    return false;
}

void 
FileStoreDialog::folderSelectionChanged()
{
    // is there a selected item?
    if (folders->selectedItems().count()) folderClicked(folders->selectedItems().first(), 0);
}

void
FileStoreDialog::folderClicked(QTreeWidgetItem *item, int)
{
    // user clicked on a folder so set path
    int index = folders->invisibleRootItem()->indexOfChild(item);

    // set folder path to whatever was clicked
    if (index == 0) setPath("/");
    else if (index > 0) setPath("/" + item->text(0));
}

void 
FileStoreDialog::fileDoubleClicked(QTreeWidgetItem*item, int)
{
    // try and set the path to the item double clicked
    if (pathname.endsWith("/")) setPath(pathname + item->text(0));
    else setPath(pathname + "/" + item->text(0));
}

void
FileStoreDialog::setFolders(FileStoreEntry *fse)
{
    // icons
    QFileIconProvider provider;

    // set the folders tree widget
    folders->clear();

    // Add ROOT
    QTreeWidgetItem *rootitem = new QTreeWidgetItem(folders);
    rootitem->setText(0, "/");
    rootitem->setIcon(0, provider.icon(QFileIconProvider::Folder));

    // add each FOLDER from the list
    foreach(FileStoreEntry *p, fse->children) {
        if (p->isDir) {
            QTreeWidgetItem *item = new QTreeWidgetItem(folders);
            item->setText(0, p->name);
            item->setIcon(0, provider.icon(QFileIconProvider::Folder));
        }
    }
}

void
FileStoreDialog::setFiles(FileStoreEntry *fse)
{
    // icons
    QFileIconProvider provider;

    // set the files tree widget
    files->clear();

    // add each FOLDER from the list
    foreach(FileStoreEntry *p, fse->children) {

        QTreeWidgetItem *item = new QTreeWidgetItem(files);

        // if only directories disable files for selection (but show for context)
        if (dironly && !p->isDir) item->setFlags(item->flags() & ~(Qt::ItemIsSelectable|Qt::ItemIsEnabled));


        // type
        if (p->isDir) {
            item->setText(1, tr("Folder"));
            item->setText(0, p->name);
            item->setIcon(0, provider.icon(QFileIconProvider::Folder));
        } else {
            item->setText(0, QFileInfo(p->name).baseName()); // no need for extensions
            item->setText(1, QFileInfo(p->name).suffix().toLower());
            item->setIcon(0, provider.icon(QFileIconProvider::File));
        }

        // modified - time or date?
        if (p->modified.date() == QDate::currentDate())
            item->setText(2, p->modified.toString("hh:mm:ss"));
        else
            item->setText(2, p->modified.toString("d MMM yyyy"));
    }
}

void
FileStoreDialog::createFolderClicked()
{
    FolderNameDialog dialog(this);
    int ret = dialog.exec();
    if (ret == QDialog::Accepted && dialog.name() != "") {
        // go and create it !
        store->createFolder(pathname + "/" + dialog.name());

        // refresh !
        setPath(pathname, true);
    }
}

FolderNameDialog::FolderNameDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Folder Name"));

    QVBoxLayout *layout = new QVBoxLayout(this);

    nameEdit = new QLineEdit(this);
    layout->addWidget(nameEdit);

    cancel = new QPushButton(tr("Cancel"));
    create = new QPushButton(tr("Create"));

    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(cancel);
    buttons->addWidget(create);
    layout->addLayout(buttons);

    connect(cancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(create, SIGNAL(clicked()), this, SLOT(accept()));
}

FileStoreSyncDialog::FileStoreSyncDialog(Context *context, FileStore *store) 
    : QDialog(context->mainWindow, Qt::Dialog), context(context), store(store), downloading(false), aborted(false)
{
    setWindowTitle(tr("Synchronise ") + store->name());
    setMinimumSize(850,450);

    QStringList errors;
    if (store->open(errors) == false) {
        QWidget::hide(); // meh

        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Sync with ") + store->name());
        msgBox.setText(tr("Unable to connect, check your configuration in preferences."));
        msgBox.setDetailedText(errors.join("\n"));

        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();

        QWidget::hide(); // don't show just yet...
        QApplication::processEvents();

        QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);

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

    // notification when upload/download completes
    connect (store, SIGNAL(writeComplete(QString,QString)), this, SLOT(completedWrite(QString,QString)));
    connect (store, SIGNAL(readComplete(QByteArray*,QString,QString)), this, SLOT(completedRead(QByteArray*,QString,QString)));

    // combo box
    athleteCombo = new QComboBox(this);
    athleteCombo->addItem(context->athlete->cyclist);
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
    rideListDown = new QTreeWidget(this);
    rideListDown->headerItem()->setText(0, " ");
    rideListDown->headerItem()->setText(1, tr("Workout Id"));
    rideListDown->headerItem()->setText(2, tr("Date"));
    rideListDown->headerItem()->setText(3, tr("Time"));
    rideListDown->headerItem()->setText(4, tr("Exists"));
    rideListDown->headerItem()->setText(5, tr("Status"));
    rideListDown->setColumnCount(6);
    rideListDown->setSelectionMode(QAbstractItemView::SingleSelection);
    rideListDown->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    rideListDown->setUniformRowHeights(true);
    rideListDown->setIndentation(0);
    rideListDown->header()->resizeSection(0,20);
    rideListDown->header()->resizeSection(1,90);
    rideListDown->header()->resizeSection(2,100);
    rideListDown->header()->resizeSection(3,100);
    rideListDown->header()->resizeSection(6,50);
    rideListDown->setSortingEnabled(true);

    downloadLayout->addWidget(selectAll);
    downloadLayout->addWidget(rideListDown);

    selectAllUp = new QCheckBox(tr("Select all"), this);
    selectAllUp->setChecked(Qt::Unchecked);

    // ride list
    rideListUp = new QTreeWidget(this);
    rideListUp->headerItem()->setText(0, " ");
    rideListUp->headerItem()->setText(1, tr("File"));
    rideListUp->headerItem()->setText(2, tr("Date"));
    rideListUp->headerItem()->setText(3, tr("Time"));
    rideListUp->headerItem()->setText(4, tr("Duration"));
    rideListUp->headerItem()->setText(5, tr("Distance"));
    rideListUp->headerItem()->setText(6, tr("Exists"));
    rideListUp->headerItem()->setText(7, tr("Status"));
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
    syncMode->addItem(tr("Keep %1 but delete Local").arg(store->name()));
    syncMode->addItem(tr("Keep Local but delete %1").arg(store->name()));
    QHBoxLayout *syncList = new QHBoxLayout;
    syncList->addWidget(selectAllSync);
    syncList->addStretch();
    syncList->addWidget(syncMode);


    // ride list
    rideListSync = new QTreeWidget(this);
    rideListSync->headerItem()->setText(0, " ");
    rideListSync->headerItem()->setText(1, tr("Source"));
    rideListSync->headerItem()->setText(2, tr("Date"));
    rideListSync->headerItem()->setText(3, tr("Time"));
    rideListSync->headerItem()->setText(4, tr("Duration"));
    rideListSync->headerItem()->setText(5, tr("Distance"));
    rideListSync->headerItem()->setText(6, tr("Action"));
    rideListSync->headerItem()->setText(7, tr("Status"));
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
    progressLabel = new QLabel(tr("Initial"), this);

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
FileStoreSyncDialog::cancelClicked()
{
    reject();
}

void
FileStoreSyncDialog::refreshClicked()
{
    progressLabel->setText(tr(""));
    progressBar->setMinimum(0);
    progressBar->setMaximum(1);
    progressBar->setValue(0);

    // wipe out current
    foreach (QTreeWidgetItem *curr, rideListDown->invisibleRootItem()->takeChildren()) {
        QCheckBox *check = (QCheckBox*)rideListDown->itemWidget(curr, 0);
        QCheckBox *exists = (QCheckBox*)rideListDown->itemWidget(curr, 4);
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

    // get a list of all rides in the home directory
    QStringList errors;
    QList<FileStoreEntry*> workouts = store->readdir(store->home(), errors);

    // clear current
    rideFiles.clear();

    Specification specification;
    specification.setDateRange(DateRange(from->date(), to->date()));
    foreach(RideItem *item, context->athlete->rideCache->rides()) {
        if (specification.pass(item))
            rideFiles << QFileInfo(item->fileName).baseName().mid(0,14);
    }

    //
    // Setup the upload list
    //
    QChar zero = QLatin1Char('0');
    uploadFiles.clear();
    for(int i=0; i<workouts.count(); i++) {

        QDateTime ridedatetime;

        // skip files that aren't ride files
        if (!RideFile::parseRideFileName(workouts[i]->name, &ridedatetime)) continue;

        // skip files that aren't in range
        if (ridedatetime.date() < from->date() || ridedatetime.date() > to->date()) continue;

        QTreeWidgetItem *add;

        add = new QTreeWidgetItem(rideListDown->invisibleRootItem());
        add->setFlags(add->flags() & ~Qt::ItemIsEditable);

        QCheckBox *check = new QCheckBox("", this);
        connect (check, SIGNAL(stateChanged(int)), this, SLOT(refreshCount()));
        rideListDown->setItemWidget(add, 0, check);

        add->setText(1, workouts[i]->name);
        add->setTextAlignment(1, Qt::AlignCenter);

        add->setText(2, ridedatetime.toString("MMM d, yyyy"));
        add->setTextAlignment(2, Qt::AlignLeft);
        add->setText(3, ridedatetime.toString("hh:mm:ss"));
        add->setTextAlignment(3, Qt::AlignCenter);

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
        rideListDown->setItemWidget(add, 4, exists);
        add->setTextAlignment(4, Qt::AlignCenter);
        if (rideFiles.contains(targetnosuffix.mid(0,14))) exists->setChecked(Qt::Checked);
        else {
            exists->setChecked(Qt::Unchecked);

            // doesn't exist -- add it to the sync list too then
            QTreeWidgetItem *sync = new QTreeWidgetItem(rideListSync->invisibleRootItem());

            QCheckBox *check = new QCheckBox("", this);
            connect (check, SIGNAL(stateChanged(int)), this, SLOT(refreshSyncCount()));
            rideListSync->setItemWidget(sync, 0, check);

            sync->setText(1, workouts[i]->name);
            sync->setTextAlignment(1, Qt::AlignCenter);
            sync->setText(2, ridedatetime.toString("MMM d, yyyy"));
            sync->setTextAlignment(2, Qt::AlignLeft);
            sync->setText(3, ridedatetime.toString("hh:mm:ss"));
            sync->setTextAlignment(3, Qt::AlignCenter);
            sync->setText(6, tr("Download"));
            sync->setTextAlignment(6, Qt::AlignLeft);
            sync->setText(7, "");
        }

        add->setText(5, "");
    }

    //
    // Now setup the upload list
    //
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

        // check if on <FileStore> already
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
FileStoreSyncDialog::tabChanged(int idx)
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
FileStoreSyncDialog::selectAllChanged(int state)
{
    for (int i=0; i<rideListDown->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListDown->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListDown->itemWidget(curr, 0);
        check->setChecked(state);
    }
}

void
FileStoreSyncDialog::selectAllUpChanged(int state)
{
    for (int i=0; i<rideListUp->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListUp->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListUp->itemWidget(curr, 0);
        check->setChecked(state);
    }
}

void
FileStoreSyncDialog::selectAllSyncChanged(int state)
{
    for (int i=0; i<rideListSync->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListSync->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListSync->itemWidget(curr, 0);
        check->setChecked(state);
    }
}

void
FileStoreSyncDialog::refreshUpCount()
{
    int selected = 0;

    for (int i=0; i<rideListUp->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListUp->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListUp->itemWidget(curr, 0);
        if (check->isChecked()) selected++;
    }
    progressLabel->setText(QString(tr("%1 of %2 selected")).arg(selected)
                            .arg(rideListUp->invisibleRootItem()->childCount()));
}

void
FileStoreSyncDialog::refreshSyncCount()
{
    int selected = 0;

    for (int i=0; i<rideListSync->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListSync->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListSync->itemWidget(curr, 0);
        if (check->isChecked()) selected++;
    }
    progressLabel->setText(QString(tr("%1 of %2 selected")).arg(selected)
                            .arg(rideListSync->invisibleRootItem()->childCount()));
}

void
FileStoreSyncDialog::refreshCount()
{
    int selected = 0;

    for (int i=0; i<rideListDown->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListDown->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListDown->itemWidget(curr, 0);
        if (check->isChecked()) selected++;
    }
    progressLabel->setText(QString(tr("%1 of %2 selected")).arg(selected)
                            .arg(rideListDown->invisibleRootItem()->childCount()));
}

void
FileStoreSyncDialog::downloadClicked()
{
    if (downloading == true) {
        rideListDown->setSortingEnabled(true);
        rideListUp->setSortingEnabled(true);
        progressLabel->setText("");
        downloadButton->setText("Download");
        downloading=false;
        aborted=true;
        cancelButton->show();
        return;
    } else {
        rideListDown->setSortingEnabled(false);
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
        case 0 : which = rideListDown; break;
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
FileStoreSyncDialog::syncNext()
{
    // the actual download/upload is kicked off using the uploader / downloader
    // if in sync mode the completedRead / completedWrite functions
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

                QByteArray *data = new QByteArray;
                store->readFile(data, curr->text(1)); // filename
                QApplication::processEvents();

            } else {
                curr->setText(7, tr("Uploading"));
                rideListSync->setCurrentItem(curr);

                // read in the file
                QStringList errors;
                QFile file(context->athlete->home->activities().canonicalPath() + "/" + curr->text(1));
                RideFile *ride = RideFileFactory::instance().openRideFile(context, file, errors);

                if (ride) {

                    // get a compressed version
                    QByteArray data;
                    store->compressRide(ride, data, QFileInfo(curr->text(1)).baseName() + ".json");
                    delete ride; // clean up!

                    store->writeFile(data, QFileInfo(curr->text(1)).baseName() + ".json.zip");
                    QApplication::processEvents();
                    return true;

                } else {
                    curr->setText(7, tr("Parse failure"));
                    QApplication::processEvents();
                }

            }
            return true;
        }
    }

    //
    // Our work is done!
    //
    rideListDown->setSortingEnabled(true);
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

    // save the ride cache, we don't want to lose that if we crash etc.
    context->athlete->rideCache->save();

    return false;
}

bool
FileStoreSyncDialog::downloadNext()
{
    for (int i=listindex; i<rideListDown->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListDown->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListDown->itemWidget(curr, 0);
        QCheckBox *exists = (QCheckBox*)rideListDown->itemWidget(curr, 4);

        // skip existing if overwrite not set
        if (check->isChecked() && exists->isChecked() && !overwrite->isChecked()) {
            curr->setText(5, tr("File exists"));
            progressBar->setValue(++downloadcounter);
            continue;
        }

        if (check->isChecked()) {

            listindex = i+1; // start from the next one
            curr->setText(5, tr("Downloading"));
            rideListDown->setCurrentItem(curr);
            progressLabel->setText(QString(tr("Downloaded %1 of %2")).arg(downloadcounter).arg(downloadtotal));

            QByteArray *data = new QByteArray; // gets deleted when read completes
            store->readFile(data, curr->text(1));
            QApplication::processEvents();
            return true;
        }
    }

    //
    // Our work is done!
    //
    rideListDown->setSortingEnabled(true);
    rideListUp->setSortingEnabled(true);
    progressLabel->setText(tr("Downloads complete"));
    downloadButton->setText("Download");
    downloading=false;
    aborted=false;
    cancelButton->show();
    selectAll->setChecked(Qt::Unchecked);
    for (int i=0; i<rideListDown->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListDown->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListDown->itemWidget(curr, 0);
        check->setChecked(false);
    }
    progressLabel->setText(QString(tr("Downloaded %1 of %2 successfully")).arg(successful).arg(downloadtotal));

    // save the ride cache, we don't want to lose that if we crash etc.
    context->athlete->rideCache->save();

    return false;
}

void
FileStoreSyncDialog::completedRead(QByteArray *data, QString name, QString /*message*/)
{
    QTreeWidget *which = sync ? rideListSync : rideListDown;
    int col = sync ? 7 : 5;

    // was abort pressed?
    if (aborted == true) {
        QTreeWidgetItem *curr = which->invisibleRootItem()->child(listindex-1);
        curr->setText(col, tr("Aborted"));
        return;
    }

    // uncompress and parse
    QStringList errors;
    RideFile *ride = store->uncompressRide(data, name, errors);

    // was allocated in before calling readfile
    delete data;

    progressBar->setValue(++downloadcounter);

    QTreeWidgetItem *curr = which->invisibleRootItem()->child(listindex-1);
    if (ride) {
        if (saveRide(ride, errors) == true) {
            curr->setText(col, tr("Saved"));
            successful++;
        } else {
            curr->setText(col, errors.join(" "));
        }

	// delete once saved
	delete ride;
    } else {
        curr->setText(col, errors.join(" "));
    }
    QApplication::processEvents();

    if (sync)
        syncNext();
    else
        downloadNext();
}

bool
FileStoreSyncDialog::uploadNext()
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

                    // get a compressed version
                    QByteArray data;
                    store->compressRide(ride, data, QFileInfo(curr->text(1)).baseName() + ".json");
                    delete ride; // clean up!

                    store->writeFile(data, QFileInfo(curr->text(1)).baseName() + ".json.zip");
                    QApplication::processEvents();
                    return true;

            } else {
                curr->setText(7, tr("Parse failure"));
                QApplication::processEvents();
            }
        }
    }

    //
    // Our work is done!
    //
    rideListDown->setSortingEnabled(true);
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
FileStoreSyncDialog::completedWrite(QString, QString result)
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
    if (result == tr("Completed.")) successful++;
    QApplication::processEvents();

    if (sync)
        syncNext();
    else
        uploadNext();
}

bool
FileStoreSyncDialog::saveRide(RideFile *ride, QStringList &errors)
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
        errors << tr("File exists");
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
