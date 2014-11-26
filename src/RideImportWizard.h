/*
 * Copyright (c) 2007 Sean C. Rhea (srhea@srhea.net)
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
    int getNumberOfFiles();  // get the number of files selected for processing
    int process();

signals:

private slots:
    void abortClicked();
    void cancelClicked();
    void todayClicked(int index);
    // void overClicked(); // deprecate for this release... XXX
    void activateSave();

private:
    void init(QList<QString> files, Context *context);
    QList <QString> filenames; // list of filenames passed
    int numberOfFiles; // number of files to be processed
    QList <bool> blanks; // record of which have a RideFileReader returned date & time
    QDir homeImports; // target directory for source files
    QDir homeActivities; // target directory for .JSON
    bool aborted;
    bool autoImportMode;
    QLabel *phaseLabel;
    QTableWidget *tableWidget;
    QTableWidget *directoryWidget;
    QProgressBar *progressBar;
    QPushButton *abortButton; // also used for save and finish
    QPushButton *cancelButton; // cancel when asking for dates
    QComboBox *todayButton;    // set date to today when asking for dates
    // QCheckBox *overFiles;      // chance to set overwrite when asking for dates // deprecate for this release... XXX
    // bool overwriteFiles; // flag to overwrite files from checkbox               // deprecate for this release... XXX
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

private:
    int dateColumn; // date is always followed by time
};

#endif // _RideImportWizard_h

