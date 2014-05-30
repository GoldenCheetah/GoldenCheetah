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

#include <qwt_plot_marker.h>

#include <QtGui>
#include <QButtonGroup>
#include <QLineEdit>
#include <QRadioButton>
#include <QWizard>
#include <QWizardPage>
#include <QLabel>
#include <QFileDialog>

class MainWindow;
class SmallPlot;
class RideItem;
class MergeWelcome;
class MergeUpload;
class MergeSync;
class MergeParameters;
class MergeSelect;
class MergeConfirm;
class MergeSyncBackground;

struct DataPoint {
    double time, watts, cad, kph, alt, hr;
    //watts, cad, kph, alt, hr;
    DataPoint(double t, double w, double c, double k, double a, double h ) :
        time(t), watts(w), cad(c), kph(k), alt(a), hr(h) {}
};

class MergeActivityWizard : public QWizard
{
    Q_OBJECT
    G_OBJECT


    public:
        MergeActivityWizard(Context *context);

        Context *context;

        RideItem *ride1;
        RideFile *ride2;

        bool keepOriginal;
        int delay;
        int stop;

        MergeWelcome    *mergeWelcome;
        MergeUpload     *mergeUpload;
        MergeSync       *mergeSync;
        MergeParameters *mergeParameters;
        MergeSelect     *mergeSelect;
        MergeConfirm    *mergeConfirm;

    private:

        SmallPlot *smallPlot1, *smallPlot2;

        QPushButton *mergeButton;
        QPushButton *uploadButton;

        //QDoubleSpinBox *hrsSpinBox, *minsSpinBox, *secsSpinBox, *countSpinBox;

        QVBoxLayout *ride2Layout;
        QLabel *ride2Label;

        QButtonGroup *powerGroup;
        QButtonGroup *hrGroup;
        QButtonGroup *speedGroup;
        QButtonGroup *cadGroup;
        QButtonGroup *altGroup;
        QButtonGroup *gpsGroup;

        QRadioButton *noPower;
        QRadioButton *noHr;
        QRadioButton *noSpeed;
        QRadioButton *noCad;
        QRadioButton *noAlt;
        QRadioButton *noGPS;

        QRadioButton *keepPower1;
        QRadioButton *keepHr1;
        QRadioButton *keepSpeed1;
        QRadioButton *keepCad1;
        QRadioButton *keepAlt1;
        QRadioButton *keepGPS1;

        QRadioButton *keepPower2;
        QRadioButton *keepHr2;
        QRadioButton *keepSpeed2;
        QRadioButton *keepCad2;
        QRadioButton *keepAlt2;
        QRadioButton *keepGPS2;
};

class MergeWelcome : public QWizardPage
{
    Q_OBJECT

    public:
        MergeWelcome(MergeActivityWizard *);

    private:
        MergeActivityWizard *wizard;
};

class MergeUpload : public QWizardPage
{
    Q_OBJECT

    public:
        MergeUpload(MergeActivityWizard *);

    private:
        MergeActivityWizard *wizard;

        QPushButton *uploadButton;
        QLabel      *labelSuccess, *ride2Label;
        bool isComplete() const;

    private slots:
        void importFile();
        void importFile(QList<QString> files);
};

class MergeSync : public QWizardPage
{
    Q_OBJECT

    public:
        MergeSync(MergeActivityWizard *);

        void initializePage();

        QLabel *warning;

        //QList<QwtPlotMarker *> markers1, markers2;

    //public slots:

    private:
        MergeActivityWizard *wizard;

        SmallPlot *smallPlot1;
        SmallPlot *smallPlot2;
        MergeSyncBackground *bg1, *bg2;
        QLineEdit *delayEdit;
        QSlider *delaySlider;

        int seriesCount, samplesCount, samplesLength;

        bool watts, cad, kph, alt, hr;

        QList<QList<int> > delay;
        QList<QList<double> > minR;


        QVector<DataPoint*> getSamplesForRide(RideFile *ride1);
        void analyse(QVector<DataPoint*> points1, QVector<DataPoint*> points2, int analysesCount);
        void findDelays(RideFile *ride1, RideFile *ride2);
        int bestDelay();
        void printDelays();
        DataPoint diffForSeries(QVector<DataPoint*> a1, QVector<DataPoint*> a2, int start, int length);
        void removeDelayFromRide( RideFile *ride, int delay );

        void setDelay(int delay);

    private slots:
        void findBestDelay();
        void setDelayFromLineEdit();
        void setDelayFromSlider();
};

class MergeParameters : public QWizardPage
{
    Q_OBJECT

    public:
        MergeParameters(MergeActivityWizard *);

    public slots:
        void valueChanged();

    private:
        MergeActivityWizard *wizard;

};

class MergeSelect : public QWizardPage
{
    Q_OBJECT

    public:
        MergeSelect(MergeActivityWizard *);

        void initializePage();

        QButtonGroup *powerGroup;
        QButtonGroup *altPowerGroup;
        QButtonGroup *hrGroup;
        QButtonGroup *speedGroup;
        QButtonGroup *cadGroup;
        QButtonGroup *altGroup;
        QButtonGroup *gpsGroup;

        QRadioButton *noPower;
        QRadioButton *noAltPower;
        QRadioButton *noHr;
        QRadioButton *noSpeed;
        QRadioButton *noCad;
        QRadioButton *noAlt;
        QRadioButton *noGPS;

        QRadioButton *keepPower1;
        QRadioButton *keepAltPower1;
        QRadioButton *keepHr1;
        QRadioButton *keepSpeed1;
        QRadioButton *keepCad1;
        QRadioButton *keepAlt1;
        QRadioButton *keepGPS1;

        QRadioButton *keepPower2;
        QRadioButton *keepAltPower2;
        QRadioButton *keepHr2;
        QRadioButton *keepSpeed2;
        QRadioButton *keepCad2;
        QRadioButton *keepAlt2;
        QRadioButton *keepGPS2;

    public slots:


    private:
        MergeActivityWizard *wizard;



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

class MergeSyncBackground: public QwtPlotItem
{
    private:
        MergeSync *parent;
        RideFile *ride;
        SmallPlot *plot;
        double startTime;
        double endTime;

        QwtPlotMarker *start, *end;

    public:

        MergeSyncBackground(MergeSync *_parent, RideFile *_ride=NULL)
        {
            setZ(0.0);
            parent = _parent;
            ride = _ride;

            start = new QwtPlotMarker;
            start->setLineStyle(QwtPlotMarker::VLine);
            start->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
            start->setLinePen(QPen(GColor(CPLOTMARKER), 0, Qt::DashDotLine));

            end = new QwtPlotMarker;
            end->setLineStyle(QwtPlotMarker::VLine);
            end->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
            end->setLinePen(QPen(GColor(CPLOTMARKER), 0, Qt::DashDotLine));
        }

        virtual int rtti() const
        {
            return QwtPlotItem::Rtti_PlotUserItem;
        }

        virtual void draw(QPainter *painter,
                        const QwtScaleMap &xMap, const QwtScaleMap &,
                        const QRectF &rect) const
        {
            // construct a rectangle
            QRectF r = rect;

            // before first mark
            if (startTime != -1 && startTime > ride->dataPoints().at(0)->secs) {
                r.setTop(1000);
                r.setBottom(0);
                r.setLeft(xMap.transform(0));
                r.setRight(xMap.transform(startTime/60.0));
                painter->fillRect(r, Qt::lightGray);
            }

            // after end mark
            if (endTime != -1 && endTime < (ride->dataPoints().last()->secs-ride->recIntSecs())) {
                QRect r;
                r.setTop(1000);
                r.setBottom(0);
                r.setLeft(xMap.transform(endTime/60.0));
                r.setRight(xMap.transform(ride->dataPoints().last()->secs-ride->recIntSecs()));
                painter->fillRect(r, Qt::lightGray);
            }

            r.setTop(1000);
            r.setBottom(0);
            r.setLeft(xMap.transform(startTime/60.0));
            r.setRight(xMap.transform(endTime/60.0));

            painter->fillRect(r, QColor(216,233,255,200));
        }

        virtual void attach( QwtPlot *plot )  {
            QwtPlotItem::attach(plot);

            start->attach(plot);
            end->attach(plot);
        }

        void setRide( RideFile *_ride )  {
            ride = _ride;
        }

        void setStartTime( double _startTime )  {
            startTime = _startTime;

            if (startTime == 0)
                startTime = ride->dataPoints().at(0)->secs;

            start->setValue(startTime / 60.0, 0.0);
        }

        void setEndTime( double _endTime )  {
            endTime = _endTime;

            if (endTime == 0)
                endTime = ride->dataPoints().last()->secs-ride->recIntSecs();

            end->setValue(endTime / 60.0, 0.0);
        }
};

#endif // _MergeActivityWizard_h

