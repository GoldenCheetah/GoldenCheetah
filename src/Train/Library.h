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

#ifndef _Library_h
#define _Library_h
#include "GoldenCheetah.h"

#include <QDir>
#include <QLabel>
#include <QDialog>
#include <QFileDialog>
#include <QCheckBox>
#include <QPushButton>
#include <QTextEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QThread>


enum class LibraryBatchImportConfirmation {
    forcedDialog,
    optionalDialog,
    noDialog
};

class Library : QObject
{
    Q_OBJECT

	public:
        QString name;           // e.g. Media Library
        QList<QString> paths;   // array of search paths for files in this library
        QList<QString> refs;    // array of drag-n-dropped files referenced not copied

        static void initialise(QDir); // init
        static Library *findLibrary(QString);
        static void importFiles(Context *context, QStringList files, LibraryBatchImportConfirmation dialog=LibraryBatchImportConfirmation::optionalDialog);
        void removeRef(Context *context, QString ref);

        static bool refreshWorkouts(Context *context);
};

extern QList<Library *> libraries;        // keep track of all Library search paths for all users

class LibrarySearch;
class LibrarySearchDialog : public QDialog
{
    Q_OBJECT

    public:
        LibrarySearchDialog(Context *context);

    private slots:
        void search();
        void cancel();

        void pathsearching(QString);
        void foundWorkout(QString);
        void foundVideo(QString);
        void foundVideoSync(QString);

        void addDirectory();
        void removeDirectory();
        void removeReference();
        void updateDB();

    private:
        Context *context;
        Library *library;
        LibrarySearch *searcher;
        bool searching;
        int pathIndex, workoutCountN, videoCountN, videosyncCountN;

        QStringList workoutsFound, videosFound, videosyncsFound;

        // let us know we are searching
        void setSearching(bool amsearching) {
            searching = amsearching;
            setWidgets();
        }
        
        // update widgets to switch between searching and not searching
        void setWidgets();

        // gui widgets
        QCheckBox *findWorkouts,
                  *findVideoSyncs,
                  *findMedia;
        QPushButton *addPath,
                    *removePath;
        QPushButton *removeRef;
        QTreeWidget *searchPathTable;
        QTreeWidget *refTable;
        QTreeWidgetItem *allRefs;
        QTreeWidgetItem *allPaths;
        QLabel *pathLabelTitle, *mediaCountTitle, *videosyncCountTitle, *workoutCountTitle;
        QLabel *pathLabel, *mediaCount, *videosyncCount, *workoutCount;
        QPushButton *cancelButton,
                    *searchButton;
};

class LibrarySearch : public QThread
{
    Q_OBJECT

    public:
        LibrarySearch(QString path, bool findMedia, bool findVideoSync, bool findWorkout);
        void run();

    public slots:
        void abort();


    signals:
        void searching(QString);
        void done();
        void foundVideo(QString);
        void foundVideoSync(QString);
        void foundWorkout(QString);

    private:
        volatile bool aborted;
        QString path;
        bool findMedia, findWorkout, findVideoSync;
};

class WorkoutImportDialog : public QDialog
{
    Q_OBJECT

    public:
        WorkoutImportDialog(Context *context, QStringList files);

    public slots:
        void import();

    private:
        Context *context;
        QStringList files;
 
        QStringList videos, videosyncs, workouts;

        QTreeWidget *fileTable;
        QPushButton *okButton, *cancelButton;

        QCheckBox *overwrite;
};

#endif // _Library_h
