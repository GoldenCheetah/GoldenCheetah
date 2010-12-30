/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net)
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

#ifndef _GC_HistogramWindow_h
#define _GC_HistogramWindow_h 1
#include "GoldenCheetah.h"

#include <QtGui>

class MainWindow;
class PowerHist;
class RideItem;

class HistogramWindow : public GcWindow
{
    Q_OBJECT
    G_OBJECT

    Q_PROPERTY(int series READ series WRITE setSeries USER true)
    Q_PROPERTY(int percent READ percent WRITE setPercent USER true)
    Q_PROPERTY(double bin READ bin WRITE setBin USER true)
    Q_PROPERTY(bool logY READ logY WRITE setLogY USER true)
    Q_PROPERTY(bool zeroes READ zeroes WRITE setZeroes USER true)
    Q_PROPERTY(bool shade READ shade WRITE setShade USER true)

    public:

        HistogramWindow(MainWindow *mainWindow);

        // get/set properties
        int series() const { return histParameterCombo->currentIndex(); }
        void setSeries(int x) { histParameterCombo->setCurrentIndex(x); }
        int percent() const { return histSumY->currentIndex(); }
        void setPercent(int x) { histSumY->setCurrentIndex(x); }
        double bin() const { return binWidthSlider->value(); }
        void setBin(double x) { binWidthSlider->setValue(x); }
        bool logY() const { return lnYHistCheckBox->isChecked(); }
        void setLogY(bool x) { lnYHistCheckBox->setChecked(x); }
        bool zeroes() const { return withZerosCheckBox->isChecked(); }
        void setZeroes(bool x) { withZerosCheckBox->setChecked(x); }
        bool shade() const { return histShadeZones->isChecked(); }
        void setShade(bool x) { histShadeZones->setChecked(x); }

    public slots:

        void rideSelected();
        void intervalSelected();
        void zonesChanged();

    protected slots:

        void setBinWidthFromSlider();
        void setBinWidthFromLineEdit();
        void setlnYHistFromCheckBox();
        void setWithZerosFromCheckBox();
        void setHistSelection(int id);
        void setSumY(int);

    protected:

        void setHistTextValidator();
        void setHistBinWidthText();

        MainWindow *mainWindow;
        PowerHist *powerHist;
        QSlider *binWidthSlider;
        QLineEdit *binWidthLineEdit;
        QCheckBox *lnYHistCheckBox;
        QCheckBox *withZerosCheckBox;
        QCheckBox *histShadeZones;
        QComboBox *histParameterCombo;
        QComboBox *histSumY;

        int powerRange, hrRange;
};

#endif // _GC_HistogramWindow_h
