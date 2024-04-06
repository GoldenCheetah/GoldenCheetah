/*
 * Copyright (c) 2011 Damien Grauser (Damien.Grauser@pev-geneve.ch)
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

#ifndef _MergeActivityWizard_h
#define _MergeActivityWizard_h

#include "GoldenCheetah.h"
#include "RideItem.h"
#include "RideFile.h"
#include "RideFile.h"
#include "Colors.h"
#include "Settings.h"
#include "SmallPlot.h"
#include "JsonRideFile.h"
#include "DownloadRideDialog.h"
#include "Device.h"
#include "Athlete.h"
#include "AllPlot.h"

#include <qwt_plot_marker.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_picker.h>
#include <qwt_scale_widget.h>
#include <qwt_arrow_button.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_text.h>
#include <qwt_legend.h>
#include <qwt_series_data.h>

#include <QtGui>
#include <QButtonGroup>
#include <QLineEdit>
#include <QRadioButton>
#include <QWizard>
#include <QWizardPage>
#include <QLabel>
#include <QFileDialog>
#include <QCommandLinkButton>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTreeWidget>
#include <QScrollArea>
#include <QCheckBox>

#include <qxtspanslider.h>

class MainWindow;
class RideItem;

class MergeActivityWizard : public QWizard
{
    Q_OBJECT
    G_OBJECT


    public:
        MergeActivityWizard(Context *context);
        Context *context;

        bool keepOriginal;
        int delay;
        int stop;

        // method for combining
        int mode; // 0 = merge, 1 = join
        int strategy; // 0=clock, 1=shared, 2=start, 3=end, 4=distance

        // recording interval to use
        double recIntSecs;

        // offset from start in samples for each ride
        int offset1, offset2;

        // which series are we going to merge ?
        QMap<RideFile::SeriesType, QCheckBox *> leftSeries, rightSeries;

        // inpit and result !
        RideItem *current;
        RideItem *combinedItem; // for interface with AllPlot. *sigh*
        RideFile *combined;

        // working copies
        void setRide(RideFile **, RideFile*);
        RideFile *ride1;
        RideFile *ride2;

        // methods for combining etc
        void analyse(); // set initial parameters based upon mode/strategy
        void combine(); // combine rides using the current parameters

    private:

        void mergeRideSamplesByDistance();

        SmallPlot *smallPlot1, *smallPlot2;

        QPushButton *mergeButton;
        QPushButton *uploadButton;

        QVBoxLayout *ride2Layout;
        QLabel *ride2Label;
};

class MergeWelcome : public QWizardPage
{
    Q_OBJECT

    public:
        MergeWelcome(MergeActivityWizard *);

    private:
        MergeActivityWizard *wizard;
};

class MergeSource : public QWizardPage
{
    Q_OBJECT

    public:
        MergeSource(MergeActivityWizard *);
        void initializePage();
        bool validate() const { return false; }
        bool isComplete() const { return false; }
        int nextId() const { return next; }

        // importing is easy !
        bool importFile();
        bool importFile(QList<QString> files);

    public slots:
        void clicked(QString);

    private:
        MergeActivityWizard *wizard;
        QSignalMapper *mapper;
        QLabel *label;
        int next;
};

class MergeDownload : public QWizardPage
{
    Q_OBJECT

    public:
        MergeDownload(MergeActivityWizard *);
        void initializePage();
        bool validate() const { return false; }
        bool isComplete() const { return false; }
        int nextId() const { return next; }

        // embed a download dialog !

    public slots:
        void downloadStarts();
        void downloadEnds();
        void downloadFiles(QList<DeviceDownloadFile>);
        //XXX void done(); // we got one ?

    private:
        MergeActivityWizard *wizard;
        QLabel *label;
        int next;
};

class MergeChoose : public QWizardPage
{
    Q_OBJECT

    public:
        MergeChoose(MergeActivityWizard *);
        void initializePage();
        bool validatePage(); 
        bool isComplete() const { return chosen; }
        int nextId() const { return next; }

    public slots:
        void selected();

    private:
        MergeActivityWizard *wizard;
        QTreeWidget *files;
        QLabel *label;
        int next;
        bool chosen;
};

class MergeMode : public QWizardPage
{
    Q_OBJECT

    public:
        MergeMode(MergeActivityWizard *);
        void initializePage();
        bool validate() const { return false; }
        bool isComplete() const { return false; }
        int nextId() const { return next; }

    public slots:
        void clicked(QString);

    private:
        MergeActivityWizard *wizard;
        QSignalMapper *mapper;
        QLabel *label;
        int next;
};

class MergeStrategy : public QWizardPage
{
    Q_OBJECT

    public:
        MergeStrategy(MergeActivityWizard *);
        void initializePage();
        bool validate() const { return false; }
        bool isComplete() const { return false; }
        int nextId() const { return next; }

    public slots:
        void clicked(QString);

    private:
        MergeActivityWizard *wizard;
        QSignalMapper *mapper;
        QLabel *label;
        int next;
        QCommandLinkButton *shared;
};

class MergeAdjust : public QWizardPage
{
    Q_OBJECT

    public:
        MergeAdjust(MergeActivityWizard *);

        void initializePage();

    private:
        MergeActivityWizard *wizard;
        AllPlot *fullPlot;
        QxtSpanSlider *spanSlider;

        QSlider *adjustSlider;
        QLabel *offsetLabel;
        QPushButton *reset;

        int offset1, offset2;

    private slots:
        void zoomChanged();
        void offsetChanged();
        void resetClicked();
};

class MergeSelect : public QWizardPage
{
    Q_OBJECT

    public:
        MergeSelect(MergeActivityWizard *);
        void initializePage();

    public slots:
        void checkboxes();


    private:
        MergeActivityWizard *wizard;

        QLabel *leftNameHeader, *rightNameHeader, *leftName, *rightName;
        QVBoxLayout *leftLayout, *rightLayout;
};

class MergeConfirm : public QWizardPage
{
    Q_OBJECT

    public:
        MergeConfirm(MergeActivityWizard *);

        bool validatePage();

    public slots:


    private:
        MergeActivityWizard *wizard;

};

#endif // _MergeActivityWizard_h

