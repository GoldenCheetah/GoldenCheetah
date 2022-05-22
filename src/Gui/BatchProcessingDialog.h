
/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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
#include "Pages.h"

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

 // Dialog class to allow editing of core data processor parameters
class CoreDpOptionsDialog : public ProcessorPage
{
    Q_OBJECT
    G_OBJECT

public:
    CoreDpOptionsDialog(Context* context);
    ~CoreDpOptionsDialog();

protected:
    QDialog* dialog;

signals:

private slots:
    void saveDpEdits();
    void cancelDpEdits();
};

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
    void cancelClicked();
    void okClicked();
    void selectClicked();
    void allClicked();
    void radioClicked(int);
    void comboSelected();
    void dpButtonSelected();

#ifdef GC_WANT_PYTHON
    void dpPyButtonSelected();
#endif

private:
    typedef enum {
        exportB,
        dataB,
#ifdef GC_WANT_PYTHON
        pythonB,
#endif
        metaB,
        deleteB } bpRadioButtonsType;

    Context *context;
    bool aborted;

    int processed, fails, processedFiles;
    bpRadioButtonsType outputMode;

    QTreeWidget* files; // choose files to export

    QWidget* disableContainer;

#ifdef GC_WANT_PYTHON
    QComboBox* pythonProcessorToRun;
#endif

    QComboBox* fileFormat, *dataProcessorToRun;
    QComboBox* metaDataFieldToUpdate, *metaDataProcessing;

    QLabel *dirName, *status;
    QCheckBox *overwrite, *all;
    QPushButton *cancel, *ok;

    void updateActionColumn();
    QString getActionColumnText();
    void fileSelected(QTreeWidgetItem*);
    void updateNumSelected();

    void exportFiles();
    void deleteFiles();
    void applyDPtoActivities(const QString& processorName);
    void applyMPtoActivities(const QString& processorName);
    
};
#endif // _BatchProcessingDialog_h
