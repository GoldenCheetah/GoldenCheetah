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

#include <QWidget>

class MainWindow;
class PowerHist;
class QCheckBox;
class QComboBox;
class QLineEdit;
class QSlider;
class RideItem;

class HistogramWindow : public QWidget
{
    Q_OBJECT

    public:

        HistogramWindow(MainWindow *mainWindow);

    public slots:

        void rideSelected();
        void intervalSelected();
        void zonesChanged();

    protected slots:

	void setHistWidgets(RideItem *rideItem);
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

	int histWattsID;
	int histWattsZoneID;
	int histNmID;
	int histHrID;
	int histHrZoneID;
	int histKphID;
	int histCadID;
	int histAltID;

    int powerRange, hrRange;
};

#endif // _GC_HistogramWindow_h

