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

#include "Library.h"
#include "Settings.h"
#include "LibraryParser.h"
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
#ifdef GC_HAVE_VLC
#include "VideoWindow.h"
#else
// if no VLC on Windows / Linux then no media!
class MediaHelper {
    public: bool isMedia(QString) { return false; }
};
#endif
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
        QFile libraryXML(home.absolutePath() + "/library.xml");
        if (libraryXML.exists() == true) {

            // parse it!
            QXmlInputSource source(&libraryXML);
            QXmlSimpleReader xmlReader;
            LibraryParser(handler);
            xmlReader.setContentHandler(&handler);
            xmlReader.setErrorHandler(&handler);
            xmlReader.parse( source );
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

//
// SEARCHDIALOG -- user select paths and files and run a search
//
LibrarySearchDialog::LibrarySearchDialog(MainWindow *mainWindow) : mainWindow(mainWindow)
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
#ifndef Q_OS_MAC
    addPath->setFixedSize(20,20);
    removePath->setFixedSize(20,20);
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

    pathLabelTitle = new QLabel(tr("Searching..."), this);
    pathLabel = new QLabel(this);

    mediaCountTitle = new QLabel(tr("Videos"), this);
    mediaCount = new QLabel(this);
    workoutCountTitle = new QLabel(tr("Workouts"), this);
    workoutCount = new QLabel(this);

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

        pathLabelTitle->show();
        pathLabel->show();
        mediaCountTitle->show();
        mediaCount->show();
        workoutCountTitle->show();
        workoutCount->show();

    } else {
        setFixedHeight(300);
        searchButton->show();
        cancelButton->setText(tr("Cancel"));
        searchPathTable->show();
        addPath->show();
        removePath->show();

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

            // now write to disk..
            LibraryParser::serialize(mainWindow->home);
        }

        // ok, we;ve completed a search without aborting
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
            searcher = new LibrarySearch(path);
        }

    } else {

        setSearching(true);
        workoutCountN = videoCountN = pathIndex = 0;
        workoutCount->setText(QString("%1").arg(workoutCountN));
        mediaCount->setText(QString("%1").arg(videoCountN));
        QTreeWidgetItem *item = searchPathTable->invisibleRootItem()->child(pathIndex);
        QString path = item->text(0);
        searcher = new LibrarySearch(path);
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
LibrarySearchDialog::updateDB()
{
    // update the database of workouts and videos
    //XXX update db here...
}

//
// SEARCH -- traverse a directory looking for files and signal to notify of progress etc
//

LibrarySearch::LibrarySearch(QString path) : path(path)
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
        if (QFileInfo(name).fileName().startsWith(",")) continue;

        emit searching(QFileInfo(name).filePath());

        // we've been told to stop!
        if (aborted) {
            // we don't emit done -- since it kicks off another search
            return;
        }

        // is a video?
        if (helper.isMedia(name)) emit foundVideo(name);
        // is a workout?
        if (ErgFile::isWorkout(name)) emit foundWorkout(name);
    }
    emit done();
};

void
LibrarySearch::abort()
{
    aborted = true;
}
