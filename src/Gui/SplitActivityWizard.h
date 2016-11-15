/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _SplitActivityWizard_h
#define _SplitActivityWizard_h

#include "GoldenCheetah.h"
#include "Context.h"
#include "SmallPlot.h"
#include "RideItem.h"
#include "RideFile.h"
#include "JsonRideFile.h"
#include "Units.h"
#include "Colors.h"

#include <QWizard>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QLabel>
#include <QTreeWidget>

#include <qwt_plot_marker.h>

class SplitSelect;
class SplitBackground;

class SplitActivityWizard : public QWizard
{
    Q_OBJECT

public:
    SplitActivityWizard(Context *context);

    Context *context;
    bool keepOriginal;
    RideItem *rideItem;

    int minimumGap,
        minimumSegmentSize;

    QTreeWidget *intervals;
    QTreeWidget *files; // what we will do!
    SmallPlot *smallPlot;
    int usedMinimumGap,
        usedMinimumSegmentSize;

    QList<RideFile*> activities;
    QList<QwtPlotMarker *> markers;
    QList<long> marks; // indexes into dataPoints
    QList<RideFileInterval *> gaps; // gaps in recording

    SplitBackground *bg;

    bool done; // have we finished?

public slots:
    // ride file utilities
    QString hasBackup(QString filename);
    QStringList conflicts(QDateTime datetime);
    void setIntervalsList(SplitSelect*);
    void setFilesList();

signals:

private slots:

};

class SplitWelcome : public QWizardPage
{
    Q_OBJECT

    public:
        SplitWelcome(SplitActivityWizard *);

    private:
        SplitActivityWizard *wizard;
};

class SplitKeep : public QWizardPage
{
    Q_OBJECT

    public:
        SplitKeep(SplitActivityWizard *);
        QLabel *warning;

    public slots:
        void keepOriginalChanged();
        void setWarning();

    private:
        SplitActivityWizard *wizard;

        QCheckBox *keepOriginal;
};

class SplitParameters : public QWizardPage
{
    Q_OBJECT

    public:
        SplitParameters(SplitActivityWizard *);

    public slots:
        void valueChanged();

    private:
        SplitActivityWizard *wizard;

        QDoubleSpinBox *minimumGap,
                    *minimumSegmentSize;
};

class SplitSelect : public QWizardPage
{
    Q_OBJECT

    public:
        SplitSelect(SplitActivityWizard *);
        void initializePage(); // reset the interval list if needed

    public slots:
        void refreshMarkers(); // set the markers on the plot

    private:
        SplitActivityWizard *wizard;
};

class SplitConfirm : public QWizardPage
{
    Q_OBJECT

    public:
        SplitConfirm(SplitActivityWizard *);
        bool isCommitPage() const { return true; }
        bool validatePage();
        void initializePage(); // create ridefile * for each selected
        bool isComplete() const;

        // constructs the new ridefile from current ride
        RideFile* createRideFile(long start, long stop);
    private:
        SplitActivityWizard *wizard;
};

// paint the alternating rectangles and split numbers
class SplitBackground: public QwtPlotItem
{
    private:
        SplitActivityWizard *parent;

    public:

        SplitBackground(SplitActivityWizard *_parent)
        {
            setZ(0.0);
            parent = _parent;
        }

        virtual int rtti() const
        {
            return QwtPlotItem::Rtti_PlotUserItem;
        }

        virtual void draw(QPainter *painter,
                        const QwtScaleMap &xMap, const QwtScaleMap &,
                        const QRectF &rect) const
        {
            RideItem *rideItem = parent->rideItem;

            double lastmark = -1;
            int segment = 0;

            // create a sorted list of markers, since we
            // may have duplicates and the sequence is
            // not guaranteed to be ordered
            QList<double> points;
            foreach(QwtPlotMarker *m, parent->markers) points.append(m->xValue());
            qSort(points.begin(), points.end());

            foreach(double mark, points) {

                if (mark == lastmark) continue;

                // construct a rectangle
                QRectF r = rect;

                if (lastmark == -1) {
                    // before first mark
                    if (mark > rideItem->ride()->dataPoints().at(0)->secs) {
                        r.setTop(1000);
                        r.setBottom(0);
                        r.setLeft(xMap.transform(0));
                        r.setRight(xMap.transform(mark));
                        painter->fillRect(r, Qt::lightGray);
                    }

                } else {

                    r.setTop(1000);
                    r.setBottom(0);
                    r.setLeft(xMap.transform(lastmark));
                    r.setRight(xMap.transform(mark));

                    if (isGap(lastmark,mark)) { // don't mark as a segment if it is a gap!

                        painter->fillRect(r, Qt::lightGray);

                    } else {

                        segment++; // starts at 0 so increment before use to start from 1

                        painter->fillRect(r, segment%2 ? QColor(216,233,255,200) : QColor(233,255,222,200));
                    }

                }
                lastmark = mark;
            }

            // is there some space to the right -- if none selected this
            // will shade the entire ride gray since lastmark will be -1
            if (lastmark < (rideItem->ride()->dataPoints().last()->secs-rideItem->ride()->recIntSecs())) {

                QRect r;
                r.setTop(1000);
                r.setBottom(0);
                r.setLeft(xMap.transform(lastmark));
                r.setRight(xMap.transform(rideItem->ride()->dataPoints().last()->secs-rideItem->ride()->recIntSecs()));
                painter->fillRect(r, Qt::lightGray);
            }
        }
        bool isGap(double start, double stop) const
        {
            foreach(RideFileInterval *g, parent->gaps) {

                double gstart = g->start / 60.0; // convert to minutes
                double gstop = g->stop / 60.0; // convert to minutes
                if (start >= gstart && start <= gstop && stop >= gstart && stop <= gstop) return true;
            }
            return false;
        }
};
#endif // _SplitActivityWizard_h
