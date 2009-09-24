
/*
 * Copyright (c) 2009 Eric Murray (ericm@lne.com)
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

#ifndef _GC_PerformanceManagerWindow_h
#define _GC_PerformanceManagerWindow_h 1

#include <QtGui>
#include <QSlider>

#include <qwt_plot_picker.h>
#include "Settings.h"


class AllPlot;
class QwtPlotPanner;
class QwtPlotZoomer;
class QSlider;
class RideItem;
class PerfPlot;
class StressCalculator;


class PerformanceManagerWindow : public QWidget
{
    Q_OBJECT

    public:

	PerformanceManagerWindow (void);
	~PerformanceManagerWindow (void);
	void replot(QDir home, QTreeWidgetItem *allRides);


    public slots:

	void PMpickerMoved(const QPoint &pos);
	void setPMSizeFromSlider();

    protected:
	int days, count;
	StressCalculator *sc;

	PerfPlot *perfplot;
	QLineEdit *PMSTSValue;
	QLineEdit *PMLTSValue;
	QLineEdit *PMSBValue;
	QLineEdit *PMDayValue;
	QwtPlotPicker *PMpicker;
	QLineEdit *PMdateRangefrom, *PMdateRangeto;
        QSlider *PMleftSlider, *PMrightSlider;
	boost::shared_ptr<QSettings> settings;

	void setPMSliderDates();

};

#endif
