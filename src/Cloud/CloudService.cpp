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

#include "CloudService.h"

#include "Athlete.h"
#include "RideCache.h"
#include "RideItem.h"
#include "MainWindow.h"
#include "JsonRideFile.h"
#include "CsvRideFile.h"
#include "Colors.h"
#include "Units.h"
#include "DataProcessor.h"  // to run auto data processors
#include "RideMetadata.h"   // for linked defaults processing

#include <QIcon>
#include <QFileIconProvider>
#include <QMessageBox>
#include <QHeaderView>

#include "../qzip/zipwriter.h"
#include "../qzip/zipreader.h"

#ifdef Q_CC_MSVC
#include <QtZlib/zlib.h>
#else
#include <zlib.h>
#endif

//
// CLOUDSERVICE BASE CLASS
//

CloudServiceFactory *CloudServiceFactory::instance_;

// nothing doing in base class, for now
CloudService::CloudService(Context *context) :
    uploadCompression(zip), downloadCompression(zip),
    filetype(JSON), useMetric(false), useEndDate(false), context(context)
{
}

// clean up on delete
CloudService::~CloudService()
{
    foreach(CloudServiceEntry *p, list_) delete p;
    list_.clear();
}

// get a new filestore entry
CloudServiceEntry *
CloudService::newCloudServiceEntry()
{
    CloudServiceEntry *p = new CloudServiceEntry();
    p->initial = true;
    list_ << p;
    return p;
}

bool
CloudService::upload(QWidget *parent, Context *context, CloudService *store, RideItem *item)
{

    // open a dialog to do it
    CloudServiceUploadDialog uploader(parent, context, store, item);
    int ret = uploader.exec();

    // was it successfull ?
    if (ret == QDialog::Accepted) return true;
    else return false;
}

//
// Utility function to create a QByteArray of data in GZIP format
// This is essentially the same as qCompress but creates it in
// GZIP format (with requisite headers) instead of ZLIB's format
// which has less filename info in the header
//
static QByteArray gCompress(const QByteArray &source)
{
    // int size is source.size()
    // const char *data is source.data()
    z_stream strm;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    // note that (15+16) below means windowbits+_16_ adds the gzip header/footer
    deflateInit2(&strm, Z_BEST_COMPRESSION, Z_DEFLATED, (15+16), 8, Z_DEFAULT_STRATEGY);

    // input data
    strm.avail_in = source.size();
    strm.next_in = (Bytef *)source.data();

    // output data - on stack not heap, will be released
    QByteArray dest(source.size()/2, '\0'); // should compress by 50%, if not don't bother

    strm.avail_out = source.size()/2;
    strm.next_out = (Bytef *)dest.data();

    // now compress!
    deflate(&strm, Z_FINISH);

    // return byte array on the stack
    return QByteArray(dest.data(), (source.size()/2) - strm.avail_out);
}

static QByteArray gUncompress(const QByteArray &data)
{
    if (data.size() <= 4) {
        qWarning("gUncompress: Input data is truncated");
        return QByteArray();
    }

    QByteArray result;

    int ret;
    z_stream strm;
    static const int CHUNK_SIZE = 1024;
    char out[CHUNK_SIZE];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = data.size();
    strm.next_in = (Bytef*)(data.data());

    ret = inflateInit2(&strm, 15 +  16); // gzip decoding
    if (ret != Z_OK)
        return QByteArray();

    // run inflate()
    do {
        strm.avail_out = CHUNK_SIZE;
        strm.next_out = (Bytef*)(out);

        ret = inflate(&strm, Z_NO_FLUSH);
        Q_ASSERT(ret != Z_STREAM_ERROR);  // state not clobbered

        switch (ret) {
        case Z_NEED_DICT:
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            (void)inflateEnd(&strm);
            return QByteArray();
        }

        result.append(out, CHUNK_SIZE - strm.avail_out);
    } while (strm.avail_out == 0);

    // clean up and return
    inflateEnd(&strm);
    return result;
}

void
CloudService::compressRide(RideFile*ride, QByteArray &data, QString name)
{
    // compress via a temporary file
    QTemporaryFile tempfile;
    tempfile.open();
    tempfile.close();

    // write as file type requested
    QString spec;
    switch(filetype) {
        default:
        case JSON: spec="json"; break;
        case TCX: spec="tcx"; break;
        case PWX: spec="pwx"; break;
        case FIT: spec="fit"; break;
        case CSV: spec="csv"; break;
    }

    QFile jsonFile(tempfile.fileName());
    bool result;

    if (spec == "csv") {
        CsvFileReader writer;
        result = writer.writeRideFile(ride->context, ride, jsonFile, CsvFileReader::gc);
    } else {
        result = RideFileFactory::instance().writeRideFile(ride->context, ride, jsonFile, spec);
    }

    if (result == true) {
        // read the ride file
        jsonFile.open(QFile::ReadOnly);
        data = jsonFile.readAll();
        jsonFile.close();

        if (uploadCompression == zip) {
            // create a temp zip file
            QTemporaryFile zipFile;
            zipFile.open();
            zipFile.close();

            // add files using zip writer
            QString zipname = zipFile.fileName();
            ZipWriter writer(zipname);

            // add the ride file to the zip file
            writer.addFile(name, data);
            writer.close();

            // now read in the zipfile
            QFile zip(zipname);
            zip.open(QFile::ReadOnly);
            data = zip.readAll();
            zip.close();
        } else if (uploadCompression == gzip) {
            data = gCompress(data);
        }
    }
}

// name is the source name (i.e. what it is called on the file store (xxxxx.json.zip)
RideFile *
CloudService::uncompressRide(QByteArray *data, QString name, QStringList &errors)
{
    // make sure its named as we expect
    if ((downloadCompression== zip && !name.endsWith(".zip")) ||
        (downloadCompression== gzip && !name.endsWith(".gz"))) {
        errors << tr("expected compressed activity file.");
        return NULL;
    }

    QByteArray jsonData;

    // some services will offer file as compressed or uncompressed
    // data. In which case they must add .zip or .gz to the end of the
    // filename to indicate it. The file format must still be included
    // in the name e.g. .pwx.gz or .fit.zip
    if (name.endsWith(".zip")) {
        // write out to a zip file first
        QTemporaryFile zipfile;
        zipfile.open();
        zipfile.write(*data);
        zipfile.close();

        // open zip
        ZipReader reader(zipfile.fileName());
        ZipReader::FileInfo info = reader.entryInfoAt(0);
        jsonData = reader.fileData(info.filePath);
        // name without the .zip
        name = name.mid(0, name.length()-4);
    } else if (name.endsWith(".gz")) {
        jsonData = gUncompress(*data);
        // name without the .gz
        name = name.mid(0, name.length()-3);
    } else {
        jsonData = *data;
    }

    // uncompress and write to tmp preserviing the file extension
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

void
CloudService::sslErrors(QWidget* parent, QNetworkReply* reply ,QList<QSslError> errors)
{
    QString errorString = "";
    foreach (const QSslError e, errors ) {
        if (!errorString.isEmpty())
            errorString += ", ";
        errorString += e.errorString();
    }
    QMessageBox::warning(parent, tr("HTTP"), tr("SSL error(s) has occurred: %1").arg(errorString));
    //reply->ignoreSslErrors(); // disabled for security reasons
}

QString
CloudService::uploadExtension() {
    QString spec;
    switch (filetype) {
        default:
        case JSON: spec = ".json"; break;
        case TCX: spec = ".tcx"; break;
        case PWX: spec = ".pwx"; break;
        case FIT: spec = ".fit"; break;
        case CSV: spec = ".csv"; break;
    }

    switch (uploadCompression) {
        case zip: spec += ".zip"; break;
        case gzip: spec += ".gz"; break;
        default:
        case none: break;
    }
    return spec;
}

CloudServiceUploadDialog::CloudServiceUploadDialog(QWidget *parent, Context *context, CloudService *store, RideItem *item) : QDialog(parent), context(context), store(store), item(item)
{
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

    // lets open the store
    QStringList errors;
    status = store->open(errors);

    // compress and upload if opened successfully.
    if (status == true) {

        // check for unsaved changes
        if (item->isDirty()) {

               QMessageBox msgBox;
                msgBox.setWindowTitle(tr("Upload to ") + store->uiName());
                msgBox.setText(tr("The activity you want to upload has unsaved changes."));
                msgBox.setDetailedText(tr("Unsaved changes in activities will be uploaded as well. \n\n"
                                          "This may lead to inconsistencies between your local activities "
                                          "and the uploaded activities if you do not save the activity in GoldenCheetah. "
                                          "We recommend to save the changed activity before proceeding."));
                msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Ignore | QMessageBox::Cancel);
                msgBox.setIcon(QMessageBox::Question);
                int ret = msgBox.exec();
                switch (ret) {
                case QMessageBox::Save:
                    // save
                    context->notifyMetadataFlush();
                    context->ride->notifyRideMetadataChanged();
                    context->mainWindow->saveSilent(context, item);
                    break;
                case QMessageBox::Ignore:
                    // just proceed
                    break;
                case QMessageBox::Cancel:
                    QApplication::processEvents();
                    QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
                    return;
                default:
                    // should never be reached
                    break;
                }

        }

        // get a compressed version
        store->compressRide(item->ride(), data, QFileInfo(item->fileName).baseName() + ".json");

        // ok, so now we can kickoff the upload
        status = store->writeFile(data, QFileInfo(item->fileName).baseName() + store->uploadExtension(), item->ride());
    }

    // if the upload failed in any way, bail out
    if (status == false) {

        // didn't work dude
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Upload Failed") + store->uiName());
        msgBox.setText(tr("Unable to upload, check your configuration in preferences."));

        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();

        QWidget::hide(); // don't show just yet...
        QApplication::processEvents();

        return;
    }

    // get notification when done
    connect(store, SIGNAL(writeComplete(QString,QString)), this, SLOT(completed(QString,QString)));

}

int
CloudServiceUploadDialog::exec()
{
    if (status) return QDialog::exec();
    else {
        QDialog::accept();
        return 0;
    }
}

void
CloudServiceUploadDialog::completed(QString file, QString message)
{
    info->setText(file + "\n" + message);
    progress->setMaximum(1);
    progress->setValue(1);
    okcancel->setText(tr("OK"));
    connect(okcancel, SIGNAL(clicked()), this, SLOT(accept()));
}

CloudServiceDialog::CloudServiceDialog(QWidget *parent, CloudService *store, QString title, QString pathname, bool dironly) :
    QDialog(parent), store(store), title(title), pathname(pathname), dironly(dironly)
{
    //setAttribute(Qt::WA_DeleteOnClose);
    setMinimumSize(350*dpiXFactor, 400*dpiYFactor);
    setWindowTitle(title + " (" + store->uiName() + ")");
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
CloudServiceDialog::returnPressed()
{
    setPath(pathEdit->text());
}

// set path
void 
CloudServiceDialog::setPath(QString path, bool refresh)
{
    QStringList errors; // keep a track of errors
    QString pathing; // keeping a track of the path we have followed

    // get root
    CloudServiceEntry *fse = store->root();

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

bool CloudServiceDialog::eventFilter(QObject *obj, QEvent *evt)
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
CloudServiceDialog::folderSelectionChanged()
{
    // is there a selected item?
    if (folders->selectedItems().count()) folderClicked(folders->selectedItems().first(), 0);
}

void
CloudServiceDialog::folderClicked(QTreeWidgetItem *item, int)
{
    // user clicked on a folder so set path
    int index = folders->invisibleRootItem()->indexOfChild(item);

    // set folder path to whatever was clicked
    if (index == 0) setPath("/");
    else if (index > 0) setPath("/" + item->text(0));
}

void 
CloudServiceDialog::fileDoubleClicked(QTreeWidgetItem*item, int)
{
    // try and set the path to the item double clicked
    if (pathname.endsWith("/")) setPath(pathname + item->text(0));
    else setPath(pathname + "/" + item->text(0));
}

void
CloudServiceDialog::setFolders(CloudServiceEntry *fse)
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
    foreach(CloudServiceEntry *p, fse->children) {
        if (p->isDir) {
            QTreeWidgetItem *item = new QTreeWidgetItem(folders);
            item->setText(0, p->name);
            item->setIcon(0, provider.icon(QFileIconProvider::Folder));
        }
    }
}

void
CloudServiceDialog::setFiles(CloudServiceEntry *fse)
{
    // icons
    QFileIconProvider provider;

    // set the files tree widget
    files->clear();

    // add each FOLDER from the list
    foreach(CloudServiceEntry *p, fse->children) {

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
            item->setText(2, p->modified.toString(tr("d MMM yyyy")));
    }
}

void
CloudServiceDialog::createFolderClicked()
{
    FolderNameDialog dialog(this);
    int ret = dialog.exec();
    if (ret == QDialog::Accepted && dialog.name() != "") {
        // go and create it ! special treatment for / root
        if (pathname == "/") {
            store->createFolder(pathname + dialog.name());
        } else {
            store->createFolder(pathname + "/" + dialog.name());
        }

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

CloudServiceSyncDialog::CloudServiceSyncDialog(Context *context, CloudService *store)
    : QDialog(context->mainWindow, Qt::Dialog), context(context), store(store), downloading(false), aborted(false)
{
    setWindowTitle(tr("Synchronise ") + store->uiName());
    setMinimumSize(850 *dpiXFactor,450 *dpiYFactor);

    QStringList errors;
    if (store->open(errors) == false) {
        QWidget::hide(); // meh

        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Sync with ") + store->uiName());
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
    if (store->capabilities() & CloudService::Upload) tabs->addTab(upload, tr("Upload"));
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
    rideListDown->headerItem()->setText(1, tr("Workout Name"));
    rideListDown->headerItem()->setText(2, tr("Date"));
    rideListDown->headerItem()->setText(3, tr("Time"));
    rideListDown->headerItem()->setText(4, tr("Exists"));
    rideListDown->headerItem()->setText(5, tr("Status"));
    rideListDown->headerItem()->setText(6, tr("Workout Id"));
    rideListDown->setColumnCount(6);
    rideListDown->setSelectionMode(QAbstractItemView::SingleSelection);
    rideListDown->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    rideListDown->setUniformRowHeights(true);
    rideListDown->setIndentation(0);
    rideListDown->header()->resizeSection(0,20*dpiXFactor);
    rideListDown->header()->resizeSection(1,90*dpiXFactor);
    rideListDown->header()->resizeSection(2,100*dpiXFactor);
    rideListDown->header()->resizeSection(3,100*dpiXFactor);
    rideListDown->header()->resizeSection(6,50*dpiXFactor);
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
    rideListUp->header()->resizeSection(0,20*dpiXFactor);
    rideListUp->header()->resizeSection(1,200*dpiXFactor);
    rideListUp->header()->resizeSection(2,100*dpiXFactor);
    rideListUp->header()->resizeSection(3,100*dpiXFactor);
    rideListUp->header()->resizeSection(4,100*dpiXFactor);
    rideListUp->header()->resizeSection(5,70*dpiXFactor);
    rideListUp->header()->resizeSection(6,50*dpiXFactor);
    rideListUp->setSortingEnabled(true);

    uploadLayout->addWidget(selectAllUp);
    uploadLayout->addWidget(rideListUp);

    selectAllSync = new QCheckBox(tr("Select all"), this);
    selectAllSync->setChecked(Qt::Unchecked);
    syncMode = new QComboBox(this);
    syncMode->addItem(tr("Keep all do not delete"));
    syncMode->addItem(tr("Keep %1 but delete Local").arg(store->uiName()));
    syncMode->addItem(tr("Keep Local but delete %1").arg(store->uiName()));
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
    rideListSync->headerItem()->setText(8, tr("Workout Id"));
    rideListSync->setColumnCount(8);
    rideListSync->setSelectionMode(QAbstractItemView::SingleSelection);
    rideListSync->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    rideListSync->setUniformRowHeights(true);
    rideListSync->setIndentation(0);
    rideListSync->header()->resizeSection(0,20*dpiXFactor);
    rideListSync->header()->resizeSection(1,200*dpiXFactor);
    rideListSync->header()->resizeSection(2,100*dpiXFactor);
    rideListSync->header()->resizeSection(3,100*dpiXFactor);
    rideListSync->header()->resizeSection(4,100*dpiXFactor);
    rideListSync->header()->resizeSection(5,70*dpiXFactor);
    rideListSync->header()->resizeSection(6,100*dpiXFactor);
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

    // check for any unsaved rides - since synchronize takes data from the stored files,
    // any unstored changes would not be uploaded
    QList<RideItem*> dirtyList;
    foreach (RideItem *rideItem, context->athlete->rideCache->rides())
        if (rideItem->isDirty() == true)
            dirtyList.append(rideItem);

    if (dirtyList.count() > 0 ) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Sync with ") + store->uiName());
        if (dirtyList.count() == 1) {
            msgBox.setText(tr("One of your activities has unsaved changes."));
        } else {
            msgBox.setText(tr("%1 of your activities have unsaved changes.").arg(dirtyList.count()));
        }
        msgBox.setDetailedText(tr("Changes in activities which are not saved, will not be synchronized. \n\n"
                                  "This may lead to inconsistencies between your local GoldenCheetah activities "
                                  "and the uploaded activities. We recommend to save the changed activities "
                                  "before proceeding."));
        msgBox.setStandardButtons(QMessageBox::SaveAll | QMessageBox::Ignore | QMessageBox::Cancel);
        msgBox.setIcon(QMessageBox::Question);
        int ret = msgBox.exec();
        switch (ret) {
        case QMessageBox::SaveAll:
            context->notifyMetadataFlush();
            context->ride->notifyRideMetadataChanged();
            // save
            if (dirtyList.count() > 0) {
                for (int i=0; i<dirtyList.count(); i++) {
                    context->mainWindow->saveSilent(context, dirtyList.at(i));
                }
            }
            break;
        case QMessageBox::Ignore:
            // just proceed
            break;
        case QMessageBox::Cancel:
            QApplication::processEvents();
            QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
            return;
        default:
            // should never be reached
            break;
        }

    }

    // refresh anyway
    refreshClicked();

}

void
CloudServiceSyncDialog::cancelClicked()
{
    reject();
}

void
CloudServiceSyncDialog::refreshClicked()
{
    double distanceFactor = GlobalContext().useMetricUnits ? 1.0 : MILES_PER_KM;
    QString distanceUnits = GlobalContext().useMetricUnits ? tr("km") : tr("mi");

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
    workouts = store->readdir(store->home(), errors, from->dateTime(), to->dateTime());

    // clear current
    rideFiles.clear();

    Specification specification;
    specification.setDateRange(DateRange(from->date(), to->date()));
    foreach(RideItem *item, context->athlete->rideCache->rides()) {
        if (specification.pass(item))
            rideFiles << QFileInfo(item->fileName).baseName().mid(0,14);
    }

    //
    // Setup the Download list
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

        add->setText(2, ridedatetime.toString(tr("MMM d, yyyy")));
        add->setTextAlignment(2, Qt::AlignLeft | Qt::AlignVCenter);
        add->setText(3, ridedatetime.toString("hh:mm:ss"));
        add->setTextAlignment(3, Qt::AlignCenter);

        QString targetnosuffix = QString ( "%1_%2_%3_%4_%5_%6" )
                           .arg ( ridedatetime.date().year(), 4, 10, zero )
                           .arg ( ridedatetime.date().month(), 2, 10, zero )
                           .arg ( ridedatetime.date().day(), 2, 10, zero )
                           .arg ( ridedatetime.time().hour(), 2, 10, zero )
                           .arg ( ridedatetime.time().minute(), 2, 10, zero )
                           .arg ( ridedatetime.time().second(), 2, 10, zero );

        // if the filestore uses enddate we need to compare date the ride finished
        // rather than date the ride started!

        if (store->useEndDate) {

            // this is fucking painful, we need to look at every ride we have
            // and add on the duration - if it ends at the same time as this
            // then adjust the target no suffix to the start time
            foreach(RideItem *item, context->athlete->rideCache->rides()) {

                QDateTime end = item->dateTime.addSecs(item->getForSymbol("workout_time"));
                long diff = end.toMSecsSinceEpoch() - ridedatetime.toMSecsSinceEpoch();

                // account for rounding so +/- 2 seconds is close enough
                if (diff < 2000 && diff > -2000) {

                    targetnosuffix = QString ( "%1_%2_%3_%4_%5_%6" )
                           .arg ( item->dateTime.date().year(), 4, 10, zero )
                           .arg ( item->dateTime.date().month(), 2, 10, zero )
                           .arg ( item->dateTime.date().day(), 2, 10, zero )
                           .arg ( item->dateTime.time().hour(), 2, 10, zero )
                           .arg ( item->dateTime.time().minute(), 2, 10, zero )
                           .arg ( item->dateTime.time().second(), 2, 10, zero );
                     break;
                 }
             }
        }
        uploadFiles << targetnosuffix.mid(0,14);

        // exists? - we ignore seconds, since TP seems to do odd
        //           things to date times and loses seconds (?)
        QCheckBox *exists = new QCheckBox("", this);
        exists->setEnabled(false);
        rideListDown->setItemWidget(add, 4, exists);
        add->setTextAlignment(4, Qt::AlignCenter);
        add->setText(6, workouts[i]->id); // download_id

        if (rideFiles.contains(targetnosuffix.mid(0,14))) exists->setChecked(true);
        else {
            exists->setChecked(Qt::Unchecked);

            // doesn't exist -- add it to the sync list too then
            QTreeWidgetItem *sync = new QTreeWidgetItem(rideListSync->invisibleRootItem());

            QCheckBox *check = new QCheckBox("", this);
            connect (check, SIGNAL(stateChanged(int)), this, SLOT(refreshSyncCount()));
            rideListSync->setItemWidget(sync, 0, check);

            sync->setText(1, workouts[i]->name);
            sync->setTextAlignment(1, Qt::AlignCenter);
            sync->setText(2, ridedatetime.toString(tr("MMM d, yyyy")));
            sync->setTextAlignment(2, Qt::AlignLeft | Qt::AlignVCenter);
            sync->setText(3, ridedatetime.toString("hh:mm:ss"));
            sync->setTextAlignment(3, Qt::AlignCenter);

            if (store->useMetric) { // Only for Today's Plan
                long secs = workouts[i]->duration;
                QChar zero = QLatin1Char ( '0' );
                QString duration = QString("%1:%2:%3").arg(secs/3600,2,10,zero)
                                                  .arg(secs%3600/60,2,10,zero)
                                                  .arg(secs%60,2,10,zero);
                sync->setText(4, duration);
                sync->setTextAlignment(4, Qt::AlignCenter);

                double distance = workouts[i]->distance;
                sync->setText(5, QString("%1 %2").arg(distance*distanceFactor, 0, 'f', 1).arg(distanceUnits));
                sync->setTextAlignment(5, Qt::AlignRight | Qt::AlignVCenter);
            }
            sync->setText(6, tr("Download"));
            sync->setTextAlignment(6, Qt::AlignLeft | Qt::AlignVCenter);
            sync->setText(7, "");

            sync->setText(8, workouts[i]->id); // download_id
        }
    }

    //
    // Now setup the upload list
    //
    bool uploadEnabled = (store->capabilities() & CloudService::Upload);
    for(int i=0; uploadEnabled && i<context->athlete->rideCache->rides().count(); i++) {

        RideItem *ride = context->athlete->rideCache->rides().at(i);
        if (!specification.pass(ride) || ride->planned) continue;

        QTreeWidgetItem *add;

        add = new QTreeWidgetItem(rideListUp->invisibleRootItem());
        add->setFlags(add->flags() & ~Qt::ItemIsEditable);

        QCheckBox *check = new QCheckBox("", this);
        connect (check, SIGNAL(stateChanged(int)), this, SLOT(refreshUpCount()));
        rideListUp->setItemWidget(add, 0, check);

        add->setText(1, ride->fileName);
        add->setTextAlignment(1, Qt::AlignLeft | Qt::AlignVCenter);
        add->setText(2, ride->dateTime.toString(tr("MMM d, yyyy")));
        add->setTextAlignment(2, Qt::AlignLeft | Qt::AlignVCenter);
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
        add->setText(5, QString("%1 %2").arg(distance*distanceFactor, 0, 'f', 1).arg(distanceUnits));
        add->setTextAlignment(5, Qt::AlignRight | Qt::AlignVCenter);

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

        // check if on <CloudService> already
        if (uploadFiles.contains(targetnosuffix.mid(0,14))) exists->setChecked(true);
        else {
            exists->setChecked(Qt::Unchecked);

            // doesn't exist -- add it to the sync list too then
            QTreeWidgetItem *sync = new QTreeWidgetItem(rideListSync->invisibleRootItem());

            QCheckBox *check = new QCheckBox("", this);
            connect (check, SIGNAL(stateChanged(int)), this, SLOT(refreshSyncCount()));
            rideListSync->setItemWidget(sync, 0, check);

            sync->setText(1, ride->fileName);
            sync->setTextAlignment(1, Qt::AlignCenter);
            sync->setText(2, ride->dateTime.toString(tr("MMM d, yyyy")));
            sync->setTextAlignment(2, Qt::AlignLeft | Qt::AlignVCenter );
            sync->setText(3, ride->dateTime.toString("hh:mm:ss"));
            sync->setTextAlignment(3, Qt::AlignCenter);
            sync->setText(4, duration);
            sync->setTextAlignment(4, Qt::AlignCenter);
            sync->setText(5, QString("%1 %2").arg(distance*distanceFactor, 0, 'f', 1).arg(distanceUnits));
            sync->setTextAlignment(5, Qt::AlignRight | Qt::AlignVCenter);
            sync->setText(6, tr("Upload"));
            sync->setTextAlignment(6, Qt::AlignLeft | Qt::AlignVCenter);
            sync->setText(7, "");
        }
        add->setText(7, "");
    }

    // adjust column widths to content text length
    for (int j=0; j<rideListUp->columnCount(); j++) {
        rideListUp->resizeColumnToContents(j);
    }
    for (int j=0; j<rideListSync->columnCount(); j++) {
        rideListSync->resizeColumnToContents(j);
    }
    for (int j=0; j<rideListDown->columnCount(); j++) {
        rideListDown->resizeColumnToContents(j);
    }

    // refresh the progress label
    tabChanged(tabs->currentIndex());
}

void
CloudServiceSyncDialog::tabChanged(int idx)
{
    if (downloadButton->text() == tr("Abort")) return;

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
CloudServiceSyncDialog::selectAllChanged(int state)
{
    for (int i=0; i<rideListDown->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListDown->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListDown->itemWidget(curr, 0);
        check->setChecked(state);
    }
}

void
CloudServiceSyncDialog::selectAllUpChanged(int state)
{
    for (int i=0; i<rideListUp->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListUp->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListUp->itemWidget(curr, 0);
        check->setChecked(state);
    }
}

void
CloudServiceSyncDialog::selectAllSyncChanged(int state)
{
    for (int i=0; i<rideListSync->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListSync->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListSync->itemWidget(curr, 0);
        check->setChecked(state);
    }
}

void
CloudServiceSyncDialog::refreshUpCount()
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
CloudServiceSyncDialog::refreshSyncCount()
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
CloudServiceSyncDialog::refreshCount()
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
CloudServiceSyncDialog::downloadClicked()
{
    if (downloading == true) {
        rideListDown->setSortingEnabled(true);
        rideListUp->setSortingEnabled(true);
        progressLabel->setText("");
        downloadButton->setText(tr("Download"));
        downloading=false;
        aborted=true;
        cancelButton->show();
        return;
    } else {
        rideListDown->setSortingEnabled(false);
        rideListUp->setSortingEnabled(true);
        downloading=true;
        aborted=false;
        downloadButton->setText(tr("Abort"));
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
CloudServiceSyncDialog::syncNext()
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
            if (curr->text(6) == tr("Download")) {
                curr->setText(7, tr("Downloading"));
                rideListSync->setCurrentItem(curr);

                QByteArray *data = new QByteArray;
                store->readFile(data, curr->text(1), curr->text(8)); // filename
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

                    store->writeFile(data, QFileInfo(curr->text(1)).baseName() + store->uploadExtension(), ride);
                    QApplication::processEvents();
                    delete ride; // clean up!
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
    downloadButton->setText(tr("Synchronize"));
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
CloudServiceSyncDialog::downloadNext()
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
            store->readFile(data, curr->text(1), curr->text(6));
            QApplication::processEvents();
            //delete data;
            return true;
        }
    }

    //
    // Our work is done!
    //
    rideListDown->setSortingEnabled(true);
    rideListUp->setSortingEnabled(true);
    progressLabel->setText(tr("Downloads complete"));
    downloadButton->setText(tr("Download"));
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
CloudServiceSyncDialog::completedRead(QByteArray *data, QString name, QString /*message*/)
{
    QTreeWidget *which = sync ? rideListSync : rideListDown;
    int col = sync ? 7 : 5;

    // was abort pressed?
    if (aborted == true) {
        QTreeWidgetItem *curr = which->invisibleRootItem()->child(listindex-1);
        curr->setText(col, tr("Aborted"));
        return;
    }

    // uncompress and parse, note the filename is passed and may be
    // different to what we asked for (sometimes the data is converted
    // from one file format to another).
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
CloudServiceSyncDialog::uploadNext()
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

            // read in the file - TEMPORARILY *** WE DON'T USE IN MEMORY VERSION ***
            QStringList errors;
            QFile file(context->athlete->home->activities().canonicalPath() + "/" + curr->text(1));
            RideFile *ride = RideFileFactory::instance().openRideFile(context, file, errors);

            if (ride) {

                    // get a compressed version
                    QByteArray data;
                    store->compressRide(ride, data, QFileInfo(curr->text(1)).baseName() + ".json");
                    store->writeFile(data, QFileInfo(curr->text(1)).baseName() + store->uploadExtension(), ride);
                    QApplication::processEvents();
                    delete ride; // clean up!
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
    downloadButton->setText(tr("Upload"));
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
CloudServiceSyncDialog::completedWrite(QString, QString result)
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
CloudServiceSyncDialog::saveRide(RideFile *ride, QStringList &errors)
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

    // process linked defaults
    GlobalContext::context()->rideMetadata->setLinkedDefaults(ride);

    // run the processor first... import
    DataProcessorFactory::instance().autoProcess(ride, "Auto", "Import");
    ride->recalculateDerivedSeries();
    // now metrics have been calculated
    DataProcessorFactory::instance().autoProcess(ride, "Save", "ADD");

    JsonFileReader reader;
    QFile file(filename);
    reader.writeRideFile(context, ride, file);

    // add to the ride list
    rideFiles<<targetnosuffix;
    context->athlete->addRide(fileinfo.fileName(), true);

    return true;
}


//
// Upgrade settings now we have migrated to a cloud service factory
// and notion of setting up "accounts" etc
//
void
CloudServiceFactory::upgrade(QString name)
{
    foreach(QString servicename, CloudServiceFactory::instance().serviceNames()) {

        QString sname; // setting name
        bool active = false;

        const CloudService *s = CloudServiceFactory::instance().service(servicename);
        if (s == NULL) continue;

        // look at config and see if it has been configured
        // if it needs a user, pass or token make sure its there
        if ((sname=s->settings.value(CloudService::OAuthToken, "")) != "") {
            if (appsettings->cvalue(name, sname, "").toString() != "") active = true;
        }
        if ((sname=s->settings.value(CloudService::Username, "")) != "") {
            if (appsettings->cvalue(name, sname, "").toString() != "") active = true;
            else active = false;
        }
        if ((sname=s->settings.value(CloudService::Password, "")) != "") {
            if (appsettings->cvalue(name, sname, "").toString() != "") active = true;
            else active = false;
        }

        // so now we can set it
        appsettings->setCValue(name, s->activeSettingName(), active ? "true" : "false");
    }
}

//
// Auto download
//
void
CloudServiceAutoDownload::autoDownload()
{
    if (initial) {
        initial = false;

        // starts a thread
        start();
    }
}

void
CloudServiceAutoDownload::checkDownload()
{
    // manually called to check
    start();
}

void
CloudServiceAutoDownload::run()
{
    // this is a separate thread and can run in parallel with the main gui
    // so we can loop through services and download the data needed.
    // we notify the main gui via the usual signals.

    // get a list of services to sync from
    QStringList worklist;
    foreach(QString name, CloudServiceFactory::instance().serviceNames()) {
        if (appsettings->cvalue(context->athlete->cyclist, CloudServiceFactory::instance().service(name)->syncOnStartupSettingName(), "false").toString() == "true") {
            worklist << name;
        }
    }

    //
    // generate a worklist to process
    //
    if (worklist.count()) {

        // Start means we are looking for downloads to do
        context->notifyAutoDownloadStart();

        // workthrough
        for(int i=0; i<worklist.count(); i++) {

            // instantiate
            CloudService *service = CloudServiceFactory::instance().newService(worklist[i], context);

            // we want to trap received files
            connect(service, SIGNAL(readComplete(QByteArray*,QString,QString)), this, SLOT(readComplete(QByteArray*,QString,QString)));

            // open connection
            QStringList errors;
            if (service->open(errors) == false) {
                delete service;
                continue;
            }

            // get list of entries
            QDateTime now = QDateTime::currentDateTime();
            QList<CloudServiceEntry*> found = service->readdir(service->home(), errors, now.addDays(-30), now);

            // some were found, so lets see if they match
            if (found.count()) {

                QStringList rideFiles; // what we have already

                Specification specification;
                specification.setDateRange(DateRange(now.addDays(-30).date(), now.date()));
                foreach(RideItem *item, context->athlete->rideCache->rides()) {
                    if (specification.pass(item))
                        rideFiles << QFileInfo(item->fileName).baseName().mid(0,16);
                }

                // eliminate matches
                bool need=false;
                foreach(CloudServiceEntry *entry, found) {

                    QDateTime ridedatetime;

                    // skip files that aren't ride files
                    if (!RideFile::parseRideFileName(entry->name, &ridedatetime)) continue;

                    // skip files that aren't in range
                    if (ridedatetime.date() < now.addDays(-30).date() || ridedatetime.date() > now.date()) continue;

                    // skip files we already have
                    bool got=false;
                    foreach(QString name, rideFiles)
                        if (entry->name.startsWith(name))
                            got=true;

                    // we want it !
                    if (!got) {
                        need = true; // need to download, so don't zap the service

                        CloudServiceDownloadEntry add;
                        add.state = CloudServiceDownloadEntry::Pending;
                        add.entry = entry;
                        add.provider = service;
                        downloadlist << add;
                    }
                }

                if (!need) {

                    // none found that we need
                    service->close();
                    delete service;
                } else {
                    providers << service; // so we can clean up later
                }

            } else {

                // none found
                service->close();
                delete service;
            }
        }
    }

    //
    // Worker loop to process the list, blocking on each download
    // and timeout if no response in 30 seconds for each
    //
    // Since this is asynchronous, the actual data is processed
    // by the receivedFile method
    //

    double progress=0;
    double inc = 100.0f / double(downloadlist.count());
    for(int i=0; i<downloadlist.count(); i++) {

        // update progress indicator
        context->notifyAutoDownloadProgress(downloadlist[i].provider->uiName(), progress, i, downloadlist.count());

        CloudServiceDownloadEntry download= downloadlist[i];

        // we block on read completing
        QEventLoop loop;
        connect(download.provider, SIGNAL(readComplete(QByteArray*,QString,QString)), &loop, SLOT(quit()));
        QTimer::singleShot(30000,&loop, SLOT(quit())); // timeout after 30 seconds

        // preallocate
        downloadlist[i].data = new QByteArray;

        download.provider->readFile(downloadlist[i].data, download.entry->name, download.entry->id);

        // block on timeout or readComplete...
        loop.exec();

        // update progress
        progress += inc;

        // if last one we need to signal done.
        if ((i+1) == downloadlist.count()) context->notifyAutoDownloadProgress(download.provider->uiName(), progress, i+1, downloadlist.count());
    }

    // time to see completion
    sleep(3);

    // all done, close the sync notification, regardless of if anything was downloaded
    context->notifyAutoDownloadEnd();

    // remove providers
    foreach(CloudService *s, providers) {
        s->close();
        delete s;
    }

    // in case we restart
    providers.clear();
    downloadlist.clear();

    // and end thread
    exit(0);
}

void
CloudServiceAutoDownload::readComplete(QByteArray*data,QString name,QString)
{
    // find the entry I belong too
    CloudServiceDownloadEntry entry;
    bool found=false;
    foreach(CloudServiceDownloadEntry p, downloadlist) {
        if (p.data == data) {
            entry=p;
            found=true;
        }
    }

    if (!found) {
        qDebug() <<"Autodownload: received file has no download entry";
        return;
    }

    // ok. so we now know what request it was for
    // so can process the result
    // uncompress and parse, note the filename is passed and may be
    // different to what we asked for (sometimes the data is converted
    // from one file format to another).
    QStringList errors;
    RideFile *ride = entry.provider->uncompressRide(data, name, errors);

    // free up regardless
    delete data;

    // can't process the content received.
    if (ride == NULL) return;

    // lets save this one away as json with the right filename
    QDateTime ridedatetime = ride->startTime();

    QChar zero = QLatin1Char ('0');
    QString targetnosuffix = QString ( "%1_%2_%3_%4_%5_%6" )
                           .arg ( ridedatetime.date().year(), 4, 10, zero )
                           .arg ( ridedatetime.date().month(), 2, 10, zero )
                           .arg ( ridedatetime.date().day(), 2, 10, zero )
                           .arg ( ridedatetime.time().hour(), 2, 10, zero )
                           .arg ( ridedatetime.time().minute(), 2, 10, zero )
                           .arg ( ridedatetime.time().second(), 2, 10, zero );

    QString filename = context->athlete->home->activities().canonicalPath() + "/" + targetnosuffix + ".json";

    // exists? -- totally should never happen unless readdir timestamp mismatches actual ride
    //            could happen if same file available at two services XXX should check above... XXX
    QFileInfo fileinfo(filename);
    if (fileinfo.exists()) {
        qDebug()<<"auto download got a duplicate:"<<filename;
        delete ride;
        return;
    }

    // process linked defaults
    GlobalContext::context()->rideMetadata->setLinkedDefaults(ride);

    // run the processor first... import
    DataProcessorFactory::instance().autoProcess(ride, "Auto", "Import");
    ride->recalculateDerivedSeries();
    // now metrics have been calculated
    DataProcessorFactory::instance().autoProcess(ride, "Save", "ADD");

    JsonFileReader reader;
    QFile file(filename);
    reader.writeRideFile(context, ride, file);

    // delete temporary in-memory copy
    delete ride;

    // add to the ride list -- but don't select it
    context->athlete->addRide(fileinfo.fileName(), true, false);

}


CloudServiceAutoDownloadWidget::CloudServiceAutoDownloadWidget(Context *context,QWidget *parent) :
    QWidget(parent), context(context), state(Dormant)
{
    connect(context, SIGNAL(autoDownloadStart()), this, SLOT(downloadStart()));
    connect(context, SIGNAL(autoDownloadEnd()), this, SLOT(downloadFinish()));
    connect(context, SIGNAL(autoDownloadProgress(QString,double,int,int)), this, SLOT(downloadProgress(QString,double,int,int)));

    // just a small little thing
    setFixedHeight(dpiYFactor * 50);
    hide();

    // animating checking
    animator= new QPropertyAnimation(this, "transition");
    animator->setStartValue(0);
    animator->setEndValue(100);
    animator->setDuration(1000);
    animator->setEasingCurve(QEasingCurve::Linear);
}

void
CloudServiceAutoDownloadWidget::downloadStart()
{
    state = Checking;
    animator->start();
    show();
}

void
CloudServiceAutoDownloadWidget::downloadFinish()
{
    state = Dormant;
    animator->stop();
    hide();
}

void
CloudServiceAutoDownloadWidget::downloadProgress(QString s, double x, int i, int n)
{
    state = Downloading;
    animator->stop();
    show();
    progress = x;
    oneof=i;
    total=n;
    servicename=s;
    repaint();
}

void
CloudServiceAutoDownloadWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    QBrush brush(GColor(CPLOTBACKGROUND));
    painter.fillRect(0,0,width(),height(), brush);

    QString statusstring;
    switch(state) {
    case Dormant: statusstring=""; break;
    case Downloading: statusstring=tr("Downloading"); break;
    case Checking: statusstring=tr("Checking"); break;
    }

    // smallest font we can
    QFont font;
    QFontMetrics fm(font);
    painter.setFont(font);
    painter.setPen(GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    QRectF textbox = QRectF(0,0, fm.horizontalAdvance(statusstring), height() / 2.0f);
    painter.drawText(textbox, Qt::AlignVCenter | Qt::AlignCenter, statusstring);

    // rectangle
    QRectF pr(textbox.width()+(5.0f*dpiXFactor), textbox.top()+(8.0f*dpiXFactor), width()-(10.0f*dpiXFactor)-textbox.width(), (height()/2.0f)-(16*dpiXFactor));

    // progress rect
    QColor col = GColor(CPLOTMARKER);
    col.setAlpha(150);
    brush= QBrush(col);

    if (state == Downloading) {
        QRectF bar(pr.left(), pr.top(), (pr.width() / 100.00f * progress), pr.height());
        painter.fillRect(bar, brush);

        // what's being downloaded?
        QRectF bottom(0, height()/2.0f, width(), height()/2.0f);
        painter.drawText(bottom, Qt::AlignLeft | Qt::AlignVCenter, QString("%1 of %2").arg(oneof).arg(total));
        painter.drawText(bottom, Qt::AlignRight | Qt::AlignVCenter, servicename);

    } else if (state == Checking) {
        // bounce
        QRectF lbar(pr.left()+ ((pr.width() *0.8f) / 100.0f * transition), pr.top(), pr.width() * 0.2f, pr.height());
        QRectF rbar(pr.left()+ (pr.width()*0.8f) - ((pr.width() *0.8f) / 100.0f * transition), pr.top(), pr.width() * 0.2f, pr.height());
        painter.fillRect(lbar, brush);
        painter.fillRect(rbar, brush);

        QRectF bottom(0, height()/2.0f, width(), height()/2.0f);
        painter.drawText(bottom, Qt::AlignLeft | Qt::AlignVCenter, tr("Last 30 days"));

        // if we ran out of juice start again
        if (transition == 100) { animator->stop(); animator->start(); }
    }

    // border of progress bar
    painter.drawRect(pr);
}
