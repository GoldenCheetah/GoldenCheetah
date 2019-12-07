/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_IntervalItem_h
#define _GC_IntervalItem_h 1
#include "GoldenCheetah.h"

#include "RideItem.h"
#include "RideItem.h"
#include <QtGui>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>

class IntervalItem
{

    public:

        // constructors and accessors
        IntervalItem(const RideItem *, QString, double, double, double, double, int, QColor, bool test, RideFileInterval::IntervalType);
        IntervalItem();

        // ride item we are in
        RideItem* rideItem() { return rideItem_; }
        RideItem* rideItem_;

        // set from other
        void setFrom(IntervalItem &other);

        // change basic values, will also apply to ridefile
        void setValues(QString name, double duration1, double duration2, 
                                     double distance1, double distance2,
                                     QColor color, bool test);

        // is this interval currently selected ?
        bool selected;

        // access the metric value
        double getForSymbol(QString name, bool useMetricUnits=true);

        // as a well formatted string
        QString getStringForSymbol(QString name, bool useMetricUnits=true);

        // is this a performance test ?
        bool istest() const { return test; }

        // interval details
        QString name;                   // name
        RideFileInterval::IntervalType type; // type User, Hill etc
        double start, stop;                  // by Time
        double startKM, stopKM;              // by Distance
        int displaySequence;                 // order to display on ride plots
        QColor color;                        // color to use on plots that differentiate by color
        QUuid route;                         // the route this interval is for
        bool test;                            // is a performance test

        // order to show on plot
        void setDisplaySequence(int seq) { displaySequence = seq; }

        // precomputed metrics
        void refresh();
        QVector<double> metrics_;
        QVector<double> count_;
        QMap <int, double>stdmean_;
        QMap <int, double>stdvariance_;

        QVector<double> &metrics() { return metrics_; }
        QVector<double> &counts() { return count_; }
        QMap <int, double>&stdmeans() { return stdmean_; }
        QMap <int, double>&stdvariances() { return stdvariance_; }

        // extracted sample data
        RideFileInterval *rideInterval;

        // used by qSort()
        bool operator< (const IntervalItem &right) const {
            return (start < right.start);
        }
};

class RenameIntervalDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT

    public:
        RenameIntervalDialog(QString &, QWidget *);

    public slots:
        void applyClicked();
        void cancelClicked();

    private:
        QString &string;
        QPushButton *applyButton, *cancelButton;
        QLineEdit *nameEdit;
};

class ColorButton;
class EditIntervalDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:
        EditIntervalDialog(QWidget *, IntervalItem &);

    public slots:
        void applyClicked();
        void cancelClicked();

    private:
        IntervalItem &interval;

        QPushButton *applyButton, *cancelButton;
        QLineEdit *nameEdit;
        QTimeEdit *fromEdit, *toEdit;
        ColorButton *colorEdit;
        QCheckBox *istest;
};

#endif // _GC_IntervalItem_h

