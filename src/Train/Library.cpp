/*
 * Copyright (c) 2012 Mark Liversedge (liversedge@gmail.com)
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

#include "Athlete.h"
#include "Context.h"
#include "Library.h"
#include "Settings.h"
#include "LibraryParser.h"
#include "TrainDB.h"
#include "HelpWhatsThis.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QApplication>
#include <QDirIterator>
#include <QFileInfo>

// helpers
#if defined(GC_VIDEO_AV) || defined(GC_VIDEO_QUICKTIME)
#include "QtMacVideoWindow.h"
#else
#include "VideoWindow.h"
#endif

#include "ErgFile.h"
#include "VideoSyncFile.h"

QList<Library*> libraries;       // keep track of all the library search paths (global)

//
// MAINTAIN 'libraries' GLOBAL
//
Library *
Library::findLibrary(QString name)
{
    foreach(Library *l, ::libraries)
        if (l->name == name)
            return l;

    return NULL;
}

void
Library::initialise(QDir home)
{
    // Search paths from library.xml
    if (libraries.count() == 0) {

        // it sits above all cyclist directories
        home.cdUp();

        // lets read library.xml, if not there then add workout config
        // if thats not set then add home
        QFile libraryXML(home.canonicalPath() + "/library.xml");
        if (libraryXML.exists() == true) {

            // parse it!
            QXmlInputSource source(&libraryXML);
            QXmlSimpleReader xmlReader;
            LibraryParser handler;
            xmlReader.setContentHandler(&handler);
            xmlReader.setErrorHandler(&handler);
            xmlReader.parse(source);
            libraries = handler.getLibraries();

        } else {

            Library *one = new Library;
            one->name = "Media Library";
            QString spath = appsettings->value(NULL, GC_WORKOUTDIR).toString();
            if (spath == "") spath = home.absolutePath();
            one->paths.append(spath);
            libraries.append(one);
        }
    }
}

void
Library::importFiles(Context *context, QStringList files, LibraryBatchImportConfirmation showDialog)
{
    QStringList videos, workouts, videosyncs;
    MediaHelper helper;

    // sort the wheat from the chaff
    foreach(QString file, files) {

        // they must exist!!
        if (!QFile(file).exists()) continue;

        // media just check file name
        if (helper.isMedia(file)) videos << file;

        // if it is a workout we parse it to check
        if (ErgFile::isWorkout(file)) {
            ErgFile *p = new ErgFile(file, ErgFileFormat::unknown, context);
            if (p->isValid()) workouts << file;
            delete p;
        }

        // if it is a VideoSync we parse it to check
        if (VideoSyncFile::isVideoSync(file)) {
            int mode = 0;
            VideoSyncFile *p = new VideoSyncFile(file, mode, context);
            if (p->isValid()) videosyncs << file;
            delete p;
        }
    }

    // nothing to dialog about...
    if (!videos.count() && !workouts.count() && !videosyncs.count()) {
        if (showDialog != LibraryBatchImportConfirmation::noDialog) {
            QMessageBox::warning(NULL, tr("Import Videos, VideoSyncs and Workouts"),
                tr("No supported videos, videoSyncs or workouts were found to import"));
        } else {
            qDebug() << "Library::importFiles:"
                     << tr("Import Videos, VideoSyncs and Workouts")
                     << tr("No supported videos, videoSyncs or workouts were found to import");
        }

        return;
    }

    // with only 1 of each max, lets import without any
    // fuss and select the items imported
    // additionally allow to disable all GUI dialogs / popups
    // when using LibraryBatchImportConfirmation::noDialog
    if (   showDialog == LibraryBatchImportConfirmation::noDialog
        || (   showDialog == LibraryBatchImportConfirmation::optionalDialog
            && videos.count() <=1
            && workouts.count() <= 1
            && videosyncs.count() <= 1)) {

        trainDB->startLUW();

        Library *l = Library::findLibrary("Media Library");

        // import the video...
        for (const auto &video : videos) {
            if (l != nullptr) {
                if (l->refs.contains(video)) {

                    // do nothing .. this is harmless!

                    //QMessageBox::warning(NULL, tr("Video already known"),
                    //    QString("%1 already exists in workout library").arg(QFileInfo(videos[0]).fileName()));

                } else {
                    l->refs.append(video);
                }
            }
            // still add it, it may not have been scanned
            trainDB->importVideo(video);
        }

        QString targetSync; // dedicated variable for notification

        // import the videosync...
        if (videosyncs.count()) {
            for (const auto &videosync : videosyncs) {
                QFile source(videosync);

                // set target directory
                QString videosyncDir = appsettings->value(NULL, GC_WORKOUTDIR).toString();
                if (videosyncDir == "") {
                    QDir root = context->athlete->home->root();
                    root.cdUp();
                    videosyncDir = root.absolutePath();
                }

                // set target filename
                targetSync = videosyncDir + "/" + QFileInfo(source).fileName();

                if (targetSync != QFileInfo(source).absoluteFilePath() && QFile(targetSync).exists()) {
                    if (showDialog != LibraryBatchImportConfirmation::noDialog) {
                        QMessageBox::warning(NULL, tr("Copy VideoSync Failed"),
                            QString(tr("%1 already exists in videoSync library: %2")).arg(QFileInfo(targetSync).fileName()).arg(videosyncDir));
                    } else {
                        qDebug() << "Library::importFiles:"
                                 << tr("Copy VideoSync Failed")
                                 << QString(tr("%1 already exists in videoSync library: %2")).arg(QFileInfo(targetSync).fileName()).arg(videosyncDir);
                    }
                } else if (targetSync != QFileInfo(source).absoluteFilePath() && !source.copy(targetSync)) {
                    if (showDialog != LibraryBatchImportConfirmation::noDialog) {
                        QMessageBox::warning(NULL, tr("Copy VideoSync Failed"),
                            QString(tr("%1 cannot be written to videoSync library %2, check permissions and free space")).arg(QFileInfo(targetSync).fileName()).arg(videosyncDir));
                    } else {
                        qDebug() << "Library::importFiles:"
                                 << tr("Copy VideoSync Failed")
                                 << QString(tr("%1 cannot be written to videoSync library %2, check permissions and free space")).arg(QFileInfo(targetSync).fileName()).arg(videosyncDir);
                    }
                }

                // still add it, it may not have been scanned
                int mode;
                VideoSyncFile file(targetSync, mode, context);
                trainDB->importVideoSync(targetSync, file);
            }
        }

        QString targetWorkout; // dedicated variable for notification

        if (workouts.count()) {

            for (const auto &workout : workouts) {
                QFile source(workout);

                // set target directory
                QString workoutDir = appsettings->value(NULL, GC_WORKOUTDIR).toString();
                if (workoutDir == "") {
                    QDir root = context->athlete->home->root();
                    root.cdUp();
                    workoutDir = root.absolutePath();
                }

                // set target filename
                targetWorkout = workoutDir + "/" + QFileInfo(source).fileName();

                // Some (highly advanced) files can be both workouts and videosync.
                // If we find a videosync here in the workout list then it was
                // emplaced above so don't the file copy again.

                if (!VideoSyncFile::isVideoSync(QFileInfo(source).fileName())) {
                    if (targetWorkout != QFileInfo(source).absoluteFilePath() && QFile(targetWorkout).exists()) {
                        if (showDialog != LibraryBatchImportConfirmation::noDialog) {
                            QMessageBox::warning(NULL, tr("Copy Workout Failed"),
                                QString(tr("%1 already exists in workout library: %2")).arg(QFileInfo(source).fileName()).arg(workoutDir));
                        } else {
                            qDebug() << "Library::importFiles:"
                                     << tr("Copy Workout Failed")
                                     << QString(tr("%1 already exists in workout library: %2")).arg(QFileInfo(source).fileName()).arg(workoutDir);
                        }
                    } else if (targetWorkout != QFileInfo(source).absoluteFilePath() && !source.copy(targetWorkout)) {
                        if (showDialog != LibraryBatchImportConfirmation::noDialog) {
                            QMessageBox::warning(NULL, tr("Copy Workout Failed"),
                                QString(tr("%1 cannot be written to workout library %2, check permissions and free space")).arg(QFileInfo(targetWorkout).fileName()).arg(workoutDir));
                        } else {
                            qDebug() << "Library::importFiles:"
                                     << tr("Copy Workout Failed")
                                     << QString(tr("%1 cannot be written to workout library %2, check permissions and free space")).arg(QFileInfo(targetWorkout).fileName()).arg(workoutDir);
                        }
                    }
                }

                // still add it, it may not have been scanned...
                ErgFile file(targetWorkout, ErgFileFormat::unknown, context);
                trainDB->importWorkout(targetWorkout, file);
            }
        }

        trainDB->endLUW();

        // now write to disk.. any refs we added
        LibraryParser::serialize(context->athlete->home->root());

        // Tell traintool to select what was imported
        if (videos.count()) context->notifySelectVideo(videos[0]);
        if (workouts.count()) context->notifySelectWorkout(targetWorkout);
        if (videosyncs.count()) context->notifySelectVideoSync(targetSync);

    } else {

        // we have a list of files to import, lets kick off the importer...
        WorkoutImportDialog *p = new WorkoutImportDialog(context, files);
        p->exec();
    }
}


bool
Library::refreshWorkouts
(Context *context)
{
    QAbstractTableModel *model = trainDB->getWorkoutModel();
    trainDB->startLUW();
    bool ok = true;
    for (int i = 0; i < model->rowCount(); ++i) {
        QString type = model->data(model->index(i, TdbWorkoutModelIdx::type)).toString();
        if (type != "code") {
            QString filepath = model->data(model->index(i, TdbWorkoutModelIdx::filepath)).toString();
            ErgFile ergfile(filepath, ErgFileFormat::unknown, context);
            if (ergfile.isValid()) {
                ok &= trainDB->importWorkout(filepath, ergfile, ImportMode::update);
            } else {
                trainDB->deleteWorkout(filepath);
                qDebug() << "Library::refreshWorkouts:" << i << "/" << model->rowCount() << ": Removing" << filepath << "- file does not parse correctly: Does it exist?";
            }
        }
    }
    trainDB->endLUW();

    return ok;
}


void
Library::removeRef(Context *context, QString ref)
{
    // remove a previous reference
    int index = refs.indexOf(ref);
    if (index >= 0) {
        refs.removeAt(index);
        LibraryParser::serialize(context->athlete->home->root());
    }
}

//
// SEARCHDIALOG -- user select paths and files and run a search
//
LibrarySearchDialog::LibrarySearchDialog(Context *context) : context(context)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("Search for Workouts, Syncs and Media"));
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Tools_ScanDisk_WorkoutVideo));
    setMinimumWidth(600 *dpiXFactor);

    searcher = NULL;

    findWorkouts = new QCheckBox(tr("Workout files (.erg, .mrc, .zwo etc)"), this);
    findWorkouts->setChecked(true);
    findMedia = new QCheckBox(tr("Video files (.mp4, .avi etc)"), this);
    findMedia->setChecked(true);
    findVideoSyncs = new QCheckBox(tr("VideoSync files (.rlv, .tts etc)"), this);
    findVideoSyncs->setChecked(true);
    addPath = new QPushButton("+", this);
    removePath = new QPushButton("-", this);
    removeRef = new QPushButton("-", this);
#ifndef Q_OS_MAC
    addPath->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    removePath->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    removeRef->setFixedSize(20*dpiXFactor,20*dpiYFactor);
#endif

    searchPathTable = new QTreeWidget(this);
#ifdef Q_OS_MAC
    // get rid of annoying focus rectangle
    searchPathTable->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    searchPathTable->setColumnCount(1);
    searchPathTable->setIndentation(0);
    searchPathTable->headerItem()->setText(0, tr("Search Path"));
    searchPathTable->setSelectionMode(QAbstractItemView::SingleSelection);
    searchPathTable->setAlternatingRowColors (false);

    library = Library::findLibrary("Media Library");
    if (library) {
        int i=1;
        foreach (QString path, library->paths) {
            QTreeWidgetItem *item = new QTreeWidgetItem(searchPathTable->invisibleRootItem(), i++);
            item->setText(0, path);
        }
    }

    refTable = new QTreeWidget(this);
#ifdef Q_OS_MAC
    // get rid of annoying focus rectangle
    refTable->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    refTable->setColumnCount(1);
    refTable->setIndentation(0);
    refTable->headerItem()->setText(0, tr("Video references"));
    refTable->setSelectionMode(QAbstractItemView::SingleSelection);
    refTable->setAlternatingRowColors (false);

    if (library) {
        int i=1;
        foreach (QString ref, library->refs) {
            QTreeWidgetItem *item = new QTreeWidgetItem(refTable->invisibleRootItem(), i++);
            item->setText(0, ref);
        }
    }

    pathLabelTitle = new QLabel(tr("Searching..."), this);
    pathLabel = new QLabel(this);

    mediaCountTitle = new QLabel(tr("Videos"), this);
    mediaCount = new QLabel(this);
    workoutCountTitle = new QLabel(tr("Workouts"), this);
    workoutCount = new QLabel(this);
    videosyncCountTitle = new QLabel(tr("VideoSyncs"), this);
    videosyncCount = new QLabel(this);
    mediaCount->setFixedWidth(80 *dpiXFactor);
    mediaCountTitle->setFixedWidth(80 *dpiXFactor);
    workoutCount->setFixedWidth(80 *dpiXFactor);
    workoutCountTitle->setFixedWidth(80 *dpiXFactor);
    videosyncCount->setFixedWidth(80 *dpiXFactor);
    videosyncCountTitle->setFixedWidth(80 *dpiXFactor);

    cancelButton = new QPushButton(tr("Cancel"), this);
    cancelButton->setDefault(false);
    searchButton = new QPushButton(tr("Search"), this);
    searchButton->setDefault(true);

    QLabel *searchLabel = new QLabel(tr("Files to search for:"), this);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    mainLayout->addWidget(searchLabel);
    mainLayout->addWidget(findWorkouts, Qt::AlignCenter);
    mainLayout->addWidget(findMedia, Qt::AlignCenter);
    mainLayout->addWidget(findVideoSyncs, Qt::AlignCenter);

    QHBoxLayout *editButtons = new QHBoxLayout;
    QVBoxLayout *tableLayout = new QVBoxLayout;
    editButtons->addWidget(addPath);
    editButtons->addWidget(removePath);
    editButtons->addStretch();
    editButtons->setSpacing(2 *dpiXFactor);
    tableLayout->setSpacing(2 *dpiXFactor);
    tableLayout->addWidget(searchPathTable);
    tableLayout->addLayout(editButtons);
    QHBoxLayout *editButtons2 = new QHBoxLayout;
    editButtons2->addWidget(removeRef);
    editButtons2->addStretch();
    tableLayout->addWidget(refTable);
    tableLayout->addLayout(editButtons2);
    mainLayout->addLayout(tableLayout);

    QGridLayout *progressLayout = new QGridLayout;
    progressLayout->addWidget(pathLabelTitle, 0,0);
    progressLayout->addWidget(mediaCountTitle, 0,1);
    progressLayout->addWidget(workoutCountTitle, 0,2);
    progressLayout->addWidget(videosyncCountTitle, 0,3);
    progressLayout->addWidget(pathLabel, 1,0);
    progressLayout->addWidget(mediaCount, 1,1);
    progressLayout->addWidget(workoutCount, 1,2);
    progressLayout->addWidget(videosyncCount, 1,3);
    progressLayout->setColumnStretch(0, 6);
    progressLayout->setColumnStretch(1, 1);
    progressLayout->setColumnStretch(2, 1);
    progressLayout->setColumnStretch(3, 1);
    mainLayout->addLayout(progressLayout);

    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(cancelButton);
    buttons->addWidget(searchButton);

    mainLayout->addStretch();
    mainLayout->addLayout(buttons);

    setSearching(false);

    connect(addPath, SIGNAL(clicked()), this, SLOT(addDirectory()));
    connect(removePath, SIGNAL(clicked()), this, SLOT(removeDirectory()));
    connect(removeRef, SIGNAL(clicked()), this, SLOT(removeReference()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancel()));
    connect(searchButton, SIGNAL(clicked()), this, SLOT(search()));
}

void
LibrarySearchDialog::setWidgets()
{
    if (searching) {
        setFixedHeight(250 *dpiYFactor);
        searchButton->hide();
        cancelButton->setText(tr("Abort Search"));
        searchPathTable->hide();
        addPath->hide();
        removePath->hide();
        removeRef->hide();
        refTable->hide();

        pathLabelTitle->show();
        pathLabel->show();
        mediaCountTitle->show();
        mediaCount->show();
        workoutCountTitle->show();
        workoutCount->show();
        videosyncCountTitle->show();
        videosyncCount->show();

    } else {
        setFixedHeight(400 *dpiYFactor);
        searchButton->show();
        cancelButton->setText(tr("Cancel"));
        searchPathTable->show();
        addPath->show();
        removePath->show();
        removeRef->show();
        refTable->show();

        pathLabelTitle->hide();
        pathLabel->hide();
        mediaCountTitle->hide();
        mediaCount->hide();
        workoutCountTitle->hide();
        workoutCount->hide();
        videosyncCountTitle->hide();
        videosyncCount->hide();
    }
}

void
LibrarySearchDialog::search()
{
    if (searchButton->text() == tr("Save")) {

        // first lets update the library paths to
        // reflect the user selections
        if (library) {
            library->paths.clear();
            for(int i=0; i<searchPathTable->invisibleRootItem()->childCount(); i++) {
                QTreeWidgetItem *item = searchPathTable->invisibleRootItem()->child(i);
                QString path = item->text(0);

                library->paths.append(path);
            }

            library->refs.clear();
            for(int i=0; i<refTable->invisibleRootItem()->childCount(); i++) {
                QTreeWidgetItem *item = refTable->invisibleRootItem()->child(i);
                QString ref = item->text(0);

                library->refs.append(ref);
            }


            // now write to disk..
            LibraryParser::serialize(context->athlete->home->root());
        }

        // ok, we've completed a search without aborting
        // so lets rebuild the database of workouts and videos
        // using what we found...
        updateDB();
        close();

        return;
    }

    if (searching) {

        // do next search path...
        if (++pathIndex >= searchPathTable->invisibleRootItem()->childCount()) {

            searcher = NULL;

            pathLabel->setText(tr("Search completed."));
            pathLabelTitle->setText("");
            searchButton->setText(tr("Save"));
            searchButton->show();
            cancelButton->hide();

            return;

        } else {

            QTreeWidgetItem *item = searchPathTable->invisibleRootItem()->child(pathIndex);
            QString path = item->text(0);
            searcher = new LibrarySearch(path, findMedia->isChecked(), findWorkouts->isChecked(), findVideoSyncs->isChecked());
        }

    } else {

        setSearching(true);
        workoutCountN = videoCountN = videosyncCountN = pathIndex = 0;
        workoutCount->setText(QString("%1").arg(workoutCountN));
        mediaCount->setText(QString("%1").arg(videoCountN));
        videosyncCount->setText(QString("%1").arg(videosyncCountN));
        QTreeWidgetItem *item = searchPathTable->invisibleRootItem()->child(pathIndex);
        QString path = item->text(0);
        searcher = new LibrarySearch(path, findMedia->isChecked(), findWorkouts->isChecked(), findVideoSyncs->isChecked());
    }

    connect(searcher, SIGNAL(done()), this, SLOT(search()));
    connect(searcher, SIGNAL(searching(QString)), this, SLOT(pathsearching(QString)));
    connect(searcher, SIGNAL(foundVideo(QString)), this, SLOT(foundVideo(QString)));
    connect(searcher, SIGNAL(foundVideoSync(QString)), this, SLOT(foundVideoSync(QString)));
    connect(searcher, SIGNAL(foundWorkout(QString)), this, SLOT(foundWorkout(QString)));

    searcher->start();
}

void
LibrarySearchDialog::pathsearching(QString text)
{
    pathLabel->setText(text);
}

void
LibrarySearchDialog::foundWorkout(QString name)
{
    workoutCount->setText(QString("%1").arg(++workoutCountN));
    workoutsFound << name;
}

void
LibrarySearchDialog::foundVideo(QString name)
{
    mediaCount->setText(QString("%1").arg(++videoCountN));
    videosFound << name;
}

void
LibrarySearchDialog::foundVideoSync(QString name)
{
    videosyncCount->setText(QString("%1").arg(++videosyncCountN));
    videosyncsFound << name;
}

void
LibrarySearchDialog::cancel()
{
    if (searching) {

        if (searcher) {
            searcher->abort();
            searcher = NULL;
            // we will NOT get a done signal...
        }

        // ...so lets clean up
        setSearching(false);
        return;
    }

    // lets close
    accept();
}

void
LibrarySearchDialog::addDirectory()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Directory"),
                            "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    // add to tree
    if (dir != "") {
        QTreeWidgetItem *item = new QTreeWidgetItem(searchPathTable->invisibleRootItem(),
                                                    searchPathTable->invisibleRootItem()->childCount()+1);
        item->setText(0, dir);

    }
}

void
LibrarySearchDialog::removeDirectory()
{
    // remove the currently selected item
    if (searchPathTable->selectedItems().isEmpty()) return;

    QTreeWidgetItem *which = searchPathTable->selectedItems().first();
    if (which) {
        searchPathTable->invisibleRootItem()->removeChild(which);
        delete which;
    }
}

void
LibrarySearchDialog::removeReference()
{
    // remove the currently selected item
    if (refTable->selectedItems().isEmpty()) return;

    QTreeWidgetItem *which = refTable->selectedItems().first();
    if (which) {
        refTable->invisibleRootItem()->removeChild(which);
        delete which;
    }
}

void
LibrarySearchDialog::updateDB()
{
    // wipe away all user data before updating
    trainDB->rebuildDB();

    trainDB->startLUW();

    // workouts
    foreach(QString ergFile, workoutsFound) {
        ErgFile file(ergFile, ErgFileFormat::unknown, context);
        if (file.isValid()) {
            trainDB->importWorkout(ergFile, file);
        }
    }

    // videos
    foreach(QString video, videosFound) {
        trainDB->importVideo(video);
    }

    // videosyncs
    foreach(QString videosync, videosyncsFound) {
        int mode=0;
        VideoSyncFile file(videosync, mode, context);
        trainDB->importVideoSync(videosync, file);
    }

    // Now check and re-add references, if there are any
    // these are files which were drag-n-dropped into the
    // GC train window, but which were referenced not
    // copied into the workout directory.
    if (library) {
        MediaHelper helper;

        foreach(QString r, library->refs) {

            if (!QFile(r).exists()) continue;

            // is a video?
            if (helper.isMedia(r)) trainDB->importVideo(r);

            // is a videosync?
            if (VideoSyncFile::isVideoSync(r)) {
                int mode=0;
                VideoSyncFile file(r, mode, context);
                if (file.isValid()) {
                    trainDB->importVideoSync(r, file);
                }
            }

            // is a workout?
            if (ErgFile::isWorkout(r)) {
                ErgFile file(r, ErgFileFormat::unknown, context);
                if (file.isValid()) {
                    trainDB->importWorkout(r, file);
                }
            }
        }
    }
    trainDB->endLUW();
}

//
// SEARCH -- traverse a directory looking for files and signal to notify of progress etc
//

LibrarySearch::LibrarySearch(QString path, bool findMedia, bool findWorkout, bool findVideoSync)
              : path(path), findMedia(findMedia), findWorkout(findWorkout), findVideoSync(findVideoSync)
{
    aborted = false;
}

void
LibrarySearch::run()
{
    MediaHelper helper;

    // file tree walking
    QDirIterator directory_walker(path, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
    while(directory_walker.hasNext()){

        directory_walker.next();

        // whizz through every file in the directory
        // if it has the right extension then we are happy
        QString name = directory_walker.filePath();

        // skip . files
        if (QFileInfo(name).fileName().startsWith(".")) continue;

        if (QFileInfo(name).isDir()) emit searching(name);

        // we've been told to stop!
        if (aborted) {
            // we don't emit done -- since it kicks off another search
            return;
        }

        // is a video?
        if (findMedia && helper.isMedia(name)) emit foundVideo(name);
        // is a videosync?
        if (findVideoSync && VideoSyncFile::isVideoSync(name)) emit foundVideoSync(name);
        // is a workout?
        if (findWorkout && ErgFile::isWorkout(name)) emit foundWorkout(name);
    }


    emit done();
}

void
LibrarySearch::abort()
{
    aborted = true;
}

//
// LIBRARY IMPORT DIALOG...
//
WorkoutImportDialog::WorkoutImportDialog(Context *context, QStringList files) :
    context(context), files(files)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    setWindowTitle(tr("Import to Library"));
    setFixedSize(450 *dpiXFactor, 450 *dpiYFactor);

    MediaHelper helper;

    // sort the wheat from the chaff
    foreach(QString file, files) {

        // they must exist!!
        if (!QFile(file).exists()) continue;

        // media just check file name
        if (helper.isMedia(file)) videos << file;

        // if it is a workout we parse it to check
        if (ErgFile::isWorkout(file)) {
            ErgFile *p = new ErgFile(file, ErgFileFormat::unknown, context);
            if (p->isValid()) workouts << file;
            delete p;
        }
        // if it is a videosync we parse it to check
        if (VideoSyncFile::isVideoSync(file)) {
            int mode = 0;
            VideoSyncFile *p = new VideoSyncFile(file, mode, context);
            if (p->isValid()) videosyncs << file;
            delete p;
        }

    }

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *notice = new QLabel(this);
    notice->setWordWrap(true);
    notice->setText(tr("Please note, that when importing or drag and dropping "
                    "videos into the library we DO NOT copy the file into "
                    "the GoldenCheetah library, instead we add a REFERENCE "
                    "to it. We DO copy workout and videoSync files, since they are smaller.\n\n"
                    "You can remove references when managing the library "
                    "via the context menu options")); // TODO : how to manage videosync files

    fileTable = new QTreeWidget(this);
#ifdef Q_OS_MAC
    // get rid of annoying focus rectangle
    fileTable->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    fileTable->setColumnCount(1);
    fileTable->setIndentation(0);
    fileTable->headerItem()->setText(0, tr("Files"));
    fileTable->setSelectionMode(QAbstractItemView::SingleSelection);
    fileTable->setAlternatingRowColors (false);
    fileTable->setTextElideMode(Qt::ElideMiddle);

    foreach (QString file, files) {
        QTreeWidgetItem *item = new QTreeWidgetItem(fileTable->invisibleRootItem(), 0);
        item->setText(0, file);
    }

    cancelButton = new QPushButton(tr("Cancel"), this);
    cancelButton->setDefault(false);
    okButton = new QPushButton(tr("OK"), this);
    okButton->setDefault(true);

    // if importing workouts and videosync files
    overwrite = new QCheckBox(tr("Overwite existing files"),this);
    if (!workouts.count() && !videosyncs.count()) overwrite->hide();

    if (videos.count() == 0) notice->hide();

    mainLayout->addWidget(notice);
    mainLayout->addWidget(fileTable);
    //mainLayout->addStretch();

    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addWidget(overwrite);
    buttons->addStretch();
    buttons->addWidget(cancelButton);
    buttons->addWidget(okButton);

    // Cancel, OK
    mainLayout->addLayout(buttons);

    connect (cancelButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect (okButton, SIGNAL(clicked()), this, SLOT(import()));
}

void
WorkoutImportDialog::import()
{
    Library *l = Library::findLibrary("Media Library");

    if (!l) accept(); // not possible

    trainDB->startLUW();

    // videos are easy, just add a reference, if its already
    // there then do nothing
    foreach(QString video, videos) {

        // if we don't already have it, add it
        if (l && !l->refs.contains(video)) {
            l->refs.append(video);
            trainDB->importVideo(video);
        }
    }

    // now write to disk..
    LibraryParser::serialize(context->athlete->home->root());

    // set target directory
    QString workoutDir = appsettings->value(NULL, GC_WORKOUTDIR).toString();
    if (workoutDir == "") {
        QDir root = context->athlete->home->root();
        root.cdUp();
        workoutDir = root.absolutePath();
    }


    // now import those workouts
    foreach(QString workout, workouts) {

        // if doesn't exist then skip
        if (!QFile(workout).exists()) continue;

        // cannot read or not valid
        ErgFile file(workout, ErgFileFormat::unknown, context);
        if (!file.isValid()) continue;

        // get target name
        QString target = workoutDir + "/" + QFileInfo(workout).fileName();

        // only copy if source != target otherwise just keep what we have
        if (target != workout) {
            // don't overwrite existing
            if (QFile(target).exists() && !overwrite->isChecked()) continue;

            // wipe and copy
            if (QFile(target).exists()) QFile::remove(target); // zap it
            QFile(workout).copy(target);
        }

        // add to library now
        trainDB->importWorkout(target, file);
    }

    // set target directory
    QString videosyncDir = appsettings->value(NULL, GC_WORKOUTDIR).toString();
    if (videosyncDir == "") {
        QDir root = context->athlete->home->root();
        root.cdUp();
        videosyncDir = root.absolutePath();
    }

    // now import those videosync
    foreach(QString videosync, videosyncs) {

        // if doesn't exist then skip
        if (!QFile(videosync).exists()) continue;

        // cannot read or not valid
        int mode=0;
        VideoSyncFile file(videosync, mode, context);
        if (!file.isValid()) continue;

        // get target name
        QString target = videosyncDir + "/" + QFileInfo(videosync).fileName();

        // only copy if source != target otherwise just keep what we have
        if (target != videosync) {
            // don't overwrite existing
            if (QFile(target).exists() && !overwrite->isChecked()) continue;

            // wipe and copy
            if (QFile(target).exists()) QFile::remove(target); // zap it
            QFile(videosync).copy(target);
        }

        // add to library now
        trainDB->importVideoSync(target, file);
    }

    trainDB->endLUW();

    accept();
}
