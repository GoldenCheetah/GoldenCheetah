
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
#include "GoldenCheetah.h"
#include "Context.h"

#include <QtGui>
#include <QSlider>
#include <QLineEdit>

#include <qwt_plot_picker.h>
#include "Settings.h"


class AllPlot;
class QwtPlotPanner;
class QwtPlotZoomer;
class QSlider;
class RideItem;
class PerfPlot;
class StressCalculator;


class PerformanceManagerWindow : public GcWindow
{
    Q_OBJECT
    G_OBJECT

    Q_PROPERTY(int scheme READ scheme WRITE setScheme USER true)

    public:

	PerformanceManagerWindow (Context *context);
	~PerformanceManagerWindow (void);

    int scheme() const { return metricCombo->currentIndex(); }
    void setScheme(int x) const { metricCombo->setCurrentIndex(x); }

#ifdef GC_HAVE_LUCENE
    bool isFiltered() const { return isfiltered; }
#endif

    public slots:

	void PMpickerMoved(const QPoint &pos);
	void setPMSizeFromSlider();
	void replot();
        void configChanged();
        void metricChanged();
        void rideSelected();

#ifdef GC_HAVE_LUCENE
    void filterChanged();
#endif

    protected:

	int days, count;
        QString metric;
	StressCalculator *sc;

        Context *context;
    bool active;

	PerfPlot *perfplot;
	QLineEdit *PMSTSValue;
	QLineEdit *PMLTSValue;
	QLineEdit *PMSBValue;
	QLineEdit *PMDayValue;
	QwtPlotPicker *PMpicker;
	QLineEdit *PMdateRangefrom, *PMdateRangeto;
        QSlider *PMleftSlider, *PMrightSlider;
        QComboBox *metricCombo;
	QSharedPointer<QSettings> settings;

	void setPMSliderDates();
    QStringList filter;
    bool isfiltered;

};

#endif
