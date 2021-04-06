/*
 * Copyright (c) 2007 Sean C. Rhea (srhea@srhea.net)
 * Additions Copyright (c) 2021 Paul Johnson (paulj49457@gmail.com)
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

#ifndef _RideImportWizard_h
#define _RideImportWizard_h
#include "GoldenCheetah.h"

#include <QtGui>
#include <QDialog>
#include <QLabel>
#include <QCheckBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QProgressBar>
#include <QList>
#include <QListIterator>
#include <QItemDelegate>
#include "Context.h"
#include "RideAutoImportConfig.h"

typedef enum {
    COPY_ON_IMPORT = 0,
    EXCLUDED_FROM_IMPORT,
} importCopyType;

// Utility class to associate the imported filename with the data and paths
class ImportFile {

public:
    // These attributes are populated for all file considered for importation.
    QString activityFullSource;
    importCopyType copyOnImport;
    bool overwriteableFile;

    // These attributes are populated for files that can be imported
    // i.e. no errors, no exclusion, no duplicates in activities
    int tableRow;
    QDateTime ridedatetime;
    QString targetNoSuffix;
    QString targetWithSuffix;
    QString tmpActivitiesFulltarget;
    QString finalActivitiesFulltarget;

    ImportFile(); 
    ImportFile(const QString& name);

    static importCopyType stringToCopyType(const QString& importString);
    static QString copyTypeToString(const importCopyType importCopy);
};

// Dialog class to show filenames, import progress and to capture user input
// of ride date and time

class RideImportWizard : public QDialog
{
    Q_OBJECT
    G_OBJECT


public:
    RideImportWizard(QList<QUrl> *urls, Context *context, QWidget *parent = 0);
    RideImportWizard(QList<QString> files, Context *context, QWidget *parent = 0);
    RideImportWizard(RideAutoImportConfig *dirs, Context *context, QWidget *parent = 0);

    ~RideImportWizard();
    void closeEvent(QCloseEvent*);
    void done(int);

    // explicitly only expand archives like .zip or .gz that contain >1 file
    QList<ImportFile> expandFiles(QList<ImportFile>);

    int getNumberOfFiles();  // get the number of files selected for processing
    int process();
    bool importInProcess() { return _importInProcess; }
    bool isAutoImport() { return autoImportMode;}

private slots:
    void abortClicked();
    void cancelClicked();
    void todayClicked(int index);
    void overClicked();
    void activateSave();

private:
    void init(QList<ImportFile> files, Context *context);
    bool moveFile(const QString &source, const QString &target);
    bool isFileExcludedFromImportation(const int i);
    void addFileToImportExclusionList(const QString& importExcludedFile);
    void copySourceFileToImportDir(const ImportFile& source, const QString& importsTarget, bool srcInImportsDir);

    QList <ImportFile> filenames; // list of filenames passed
    int numberOfFiles; // number of files to be processed
    QList <bool> blanks; // record of which have a RideFileReader returned date & time
    QDir homeImports; // target directory for source files
    QDir homeActivities; // target directory for .JSON
    QDir tmpActivities; // activitiy .JSON is stored here until rideCache() update was successfull
    bool aborted;
    bool autoImportMode;
    bool autoImportStealth;
    bool _importInProcess;
    QLabel *phaseLabel;
    QTableWidget *tableWidget;
    QTableWidget *directoryWidget;
    QProgressBar *progressBar;
    QPushButton *abortButton;  // also used for save and finish
    QPushButton *cancelButton; // cancel when asking for dates
    QComboBox *todayButton;    // set date to today when asking for dates
    QCheckBox *overFiles;      // chance to set overwrite when asking for dates
    bool overwriteFiles;       // flag to overwrite files from checkbox
    Context *context; // caller
    RideAutoImportConfig *importConfig;

    QStringList deleteMe; // list of temp files created during import


};

// Item Delegate for Editing Date and Time of Ride inside the
// QTableWidget

class RideDelegate : public QItemDelegate
{
    Q_OBJECT
    G_OBJECT


public:
    RideDelegate(int dateColumn, QObject *parent = 0);

    // override inherited methods
    // repaint the cell
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    // setup editor in the cell
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    // Fetch data from model ready for editing
    void setEditorData(QWidget *editor, const QModelIndex &index) const;

    // Save data back to model after editing
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;

private slots:
    void commitAndCloseTimeEditor();
    void commitAndCloseDateEditor();
    void commitAndCloseAutoImportEditor(int index);

private:
    int dateColumn; // date is always followed by time
};

#endif // _RideImportWizard_h

