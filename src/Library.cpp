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
#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QApplication>
#include <QDirIterator>
#include <QFileInfo>

// helpers
#ifdef Q_OS_MAC
#include "QtMacVideoWindow.h"
#else
#include "VideoWindow.h"
#endif

#include "ErgFile.h"

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
Library::importFiles(Context *context, QStringList files)
{
    QStringList videos, workouts;
    MediaHelper helper;

    // sort the wheat from the chaff
    foreach(QString file, files) {

        // they must exist!!
        if (!QFile(file).exists()) continue;

        // media just check file name
        if (helper.isMedia(file)) videos << file;

        // if it is a workout we parse it to check
        if (ErgFile::isWorkout(file)) {
            int mode;
            ErgFile *p = new ErgFile(file, mode, context);
            if (p->isValid()) workouts << file;
            delete p;
        }
    }

    // nothing to dialog about...
    if (!videos.count() && !workouts.count()) {

        QMessageBox::warning(NULL, tr("Import Videos and Workouts"), 
            "No supported videos or workouts were found to import");

        return;
    }

    // with only 1 of each max, lets import without any
    // fuss and select the items imported
    if (videos.count()<=1 && workouts.count() <= 1) {

        trainDB->startLUW();

        Library *l = Library::findLibrary("Media Library");

        // import the video...
        if (videos.count()) {

            if (l->refs.contains(videos[0])) {

                // do nothing .. this is harmless!

                //QMessageBox::warning(NULL, tr("Video already known"), 
                //    QString("%1 already exists in workout library").arg(QFileInfo(videos[0]).fileName()));

            } else {
                l->refs.append(videos[0]);
            }
            // still add it, it may not have been scanned
            trainDB->importVideo(videos[0]);
        }

        QString target;
        if (workouts.count()) {

            QFile source(workouts[0]);

            // set target directory
            QString workoutDir = appsettings->value(NULL, GC_WORKOUTDIR).toString();
            if (workoutDir == "") {
                QDir root = context->athlete->home->root();
                root.cdUp();
                workoutDir = root.absolutePath();
            }

            // set target filename
            target = workoutDir + "/" + QFileInfo(source).fileName();

            if (!source.copy(target)) {

                QMessageBox::warning(NULL, tr("Copy Workout Failed"), 
                    QString("%1 already exists in workout library").arg(QFileInfo(target).fileName()));
            }

            // still add it, it may noit have been scanned...
            int mode;
            ErgFile file(target, mode, context);
            trainDB->importWorkout(target, &file);

        }

        trainDB->endLUW();

        // now write to disk.. any refs we added
        LibraryParser::serialize(context->athlete->home->root());

        // Tell traintool to select what was imported
        if (videos.count()) context->notifySelectVideo(videos[0]);
        if (workouts.count()) context->notifySelectWorkout(target);

    } else {

        // we have a list of files to import, lets kick off the importer...
        WorkoutImportDialog *p = new WorkoutImportDialog(context, files);
        p->exec();
    }
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
    setWindowTitle(tr("Search for Workouts and Media"));
    setMinimumWidth(600);

    searcher = NULL;

    findWorkouts = new QCheckBox(tr("Workout files (.erg, .mrc etc)"), this);
    findWorkouts->setChecked(true);
    findMedia = new QCheckBox(tr("Video files (.mp4, .avi etc)"), this);
    findMedia->setChecked(true);

    addPath = new QPushButton("+", this);
    removePath = new QPushButton("-", this);
    removeRef = new QPushButton("-", this);
#ifndef Q_OS_MAC
    addPath->setFixedSize(20,20);
    removePath->setFixedSize(20,20);
    removeRef->setFixedSize(20,20);
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
    mediaCount->setFixedWidth(40);
    mediaCountTitle->setFixedWidth(40);
    workoutCount->setFixedWidth(60);
    workoutCountTitle->setFixedWidth(60);

    cancelButton = new QPushButton(tr("Cancel"), this);
    cancelButton->setDefault(false);
    searchButton = new QPushButton(tr("Search"), this);
    searchButton->setDefault(true);

    QLabel *searchLabel = new QLabel(tr("Files to search for:"), this);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    mainLayout->addWidget(searchLabel);
    mainLayout->addWidget(findWorkouts, Qt::AlignCenter);
    mainLayout->addWidget(findMedia, Qt::AlignCenter);

    QHBoxLayout *editButtons = new QHBoxLayout;
    QVBoxLayout *tableLayout = new QVBoxLayout;
    editButtons->addWidget(addPath);
    editButtons->addWidget(removePath);
    editButtons->addStretch();
    editButtons->setSpacing(2);
    tableLayout->setSpacing(2);
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
    progressLayout->addWidget(pathLabel, 1,0);
    progressLayout->addWidget(mediaCount, 1,1);
    progressLayout->addWidget(workoutCount, 1,2);
    progressLayout->setColumnStretch(0, 7);
    progressLayout->setColumnStretch(1, 1);
    progressLayout->setColumnStretch(2, 1);
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
        setFixedHeight(200);
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

    } else {
        setFixedHeight(400);
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
            searcher = new LibrarySearch(path, findMedia->isChecked(), findWorkouts->isChecked());
        }

    } else {

        setSearching(true);
        workoutCountN = videoCountN = pathIndex = 0;
        workoutCount->setText(QString("%1").arg(workoutCountN));
        mediaCount->setText(QString("%1").arg(videoCountN));
        QTreeWidgetItem *item = searchPathTable->invisibleRootItem()->child(pathIndex);
        QString path = item->text(0);
        searcher = new LibrarySearch(path, findMedia->isChecked(), findWorkouts->isChecked());
    }

    connect(searcher, SIGNAL(done()), this, SLOT(search()));
    connect(searcher, SIGNAL(searching(QString)), this, SLOT(pathsearching(QString)));
    connect(searcher, SIGNAL(foundVideo(QString)), this, SLOT(foundVideo(QString)));
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
        int mode;
        ErgFile file(ergFile, mode, context);
        if (file.isValid()) {
            trainDB->importWorkout(ergFile, &file);
        }
    }

    // videos
    foreach(QString video, videosFound) {
        trainDB->importVideo(video);
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

            // is a workout?
            if (ErgFile::isWorkout(r)) {
                int mode;
                ErgFile file(r, mode, context);
                if (file.isValid()) {
                    trainDB->importWorkout(r, &file);
                }
            }
        }
    }
    trainDB->endLUW();
}

//
// SEARCH -- traverse a directory looking for files and signal to notify of progress etc
//

LibrarySearch::LibrarySearch(QString path, bool findMedia, bool findWorkout)
              : path(path), findMedia(findMedia), findWorkout(findWorkout)
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
        // is a workout?
        if (findWorkout && ErgFile::isWorkout(name)) emit foundWorkout(name);
    }


    emit done();
};

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
    setFixedSize(450, 450);

    MediaHelper helper;

    // sort the wheat from the chaff
    foreach(QString file, files) {

        // they must exist!!
        if (!QFile(file).exists()) continue;

        // media just check file name
        if (helper.isMedia(file)) videos << file;

        // if it is a workout we parse it to check
        if (ErgFile::isWorkout(file)) {
            int mode;
            ErgFile *p = new ErgFile(file, mode, context);
            if (p->isValid()) workouts << file;
            delete p;
        }
    }

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *notice = new QLabel(this);
    notice->setWordWrap(true);
    notice->setText("Please note, that when importing or drag and dropping "
                    "videos into the library we DO NOT copy the file into "
                    "the GoldenCheetah library, instead we add a REFERENCE "
                    "to it. We DO copy workout files, since they are smaller.\n\n"
                    "You can remove references when managing the library "
                    "via the tools menu option");

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

    // if importing workouts..
    overwrite = new QCheckBox(tr("Overwite existing files"),this);
    if (!workouts.count()) overwrite->hide();

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
        if (!l->refs.contains(video)) {
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
        int mode;
        ErgFile file(workout, mode, context);
        if (!file.isValid()) continue;

        // get target name
        QString target = workoutDir + "/" + QFileInfo(workout).fileName();

        // don't overwrite existing
        if (QFile(target).exists() && !overwrite->isChecked()) continue;

        // wipe and copy
        if (QFile(target).exists()) QFile::remove(target); // zap it
        QFile(workout).copy(target);

        // add to library now
        trainDB->importWorkout(target, &file);
    }

    trainDB->endLUW();

    accept();
}
