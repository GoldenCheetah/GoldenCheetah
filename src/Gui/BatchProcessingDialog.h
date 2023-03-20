
/*
 * Batch Export Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
 * Batch Processing Copyright (c) 2022 Paul Johnson (paulj49457@gmail.com)
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

#ifndef _BatchProcessingDialog_h
#define _BatchProcessingDialog_h

#include "GoldenCheetah.h"
#include "Context.h"
#include "Settings.h"
#include "Units.h"

#include "RideItem.h"
#include "RideFile.h"

#include <QtGui>
#include <QTreeWidget>
#include <QProgressBar>
#include <QList>
#include <QFileDialog>
#include <QCheckBox>
#include <QLabel>
#include <QListIterator>
#include <QDebug>

// Dialog class to allow batch processing of activities
class BatchProcessingDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT

public:
    BatchProcessingDialog(Context *context);

protected:

signals:

private slots:
    void dpButtonClicked();
    void cancelClicked();
    void okClicked();
    void selectClicked();
    void allClicked();
    void radioClicked(int);
    void comboSelected();

private:

    typedef enum {
        unknownF,
        finishedF,
        userF,
        noDataProcessorF
    } bpFailureType;

    typedef enum {
        exportB,
        dataProcessorB,
        deleteB } batchRadioBType;

    Context *context;
    bool aborted;

    int processed, fails, numFilesToProcess;
    batchRadioBType outputMode;

    QTreeWidget *files; // choose files to export

    QWidget *disableContainer, *dpContainer, *exportContainer;

    QComboBox *fileFormat, *dataProcessorToRun;

    QLabel *dirName, *status;
    QCheckBox *overwrite, *all;
    QPushButton *dpButton, *cancel, *ok;

    void updateActionColumn();
    QString getActionColumnText();
    void fileSelected(QTreeWidgetItem* current);
    void updateNumberSelected();

    bpFailureType exportFiles();
    bpFailureType deleteFiles();
    bpFailureType runDataProcessorOnActivities(const QString& processorName);

    void failedToProcessEntry(QTreeWidgetItem* current);
    
};
#endif // _BatchProcessingDialog_h
