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

#ifndef _GC_CriticalPowerWindow_h
#define _GC_CriticalPowerWindow_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include "Season.h"
#ifdef GC_HAVE_LUCENE
#include "SearchFilterBox.h"
#endif

class CpintPlot;
class MainWindow;
class RideItem;
class QwtPlotPicker;

class CriticalPowerWindow : public GcWindow
{
    Q_OBJECT
    G_OBJECT

    // properties can be saved/restored/set by the layout manager

#ifdef GC_HAVE_LUCENE
    Q_PROPERTY(QString filter READ filter WRITE setFilter USER true)
#endif
    Q_PROPERTY(int mode READ mode WRITE setMode USER true)

    // for retro compatibility
    Q_PROPERTY(QString season READ season WRITE setSeason USER true)

    public:

        CriticalPowerWindow(const QDir &home, MainWindow *parent, bool range = false);

        void deleteCpiFile(QString filename);

        // set/get properties
        // ---------------------------------------------------
        int mode() const { return seriesCombo->currentIndex(); }
        void setMode(int x) { seriesCombo->setCurrentIndex(x); }

#ifdef GC_HAVE_LUCENE
        // filter
        QString filter() const { return searchBox->filter(); }
        void setFilter(QString x) { searchBox->setFilter(x); }
#endif

        // for retro compatibility
        QString season() const { return cComboSeason->itemText(cComboSeason->currentIndex()); }
        void setSeason(QString x) { 
            int index = cComboSeason->findText(x);
            if (index != -1) {
                cComboSeason->setCurrentIndex(index);
                seasonSelected(index);
            }
        }

        RideFile::SeriesType series() { 
            return static_cast<RideFile::SeriesType>
                (seriesCombo->itemData(seriesCombo->currentIndex()).toInt());
        }

    protected slots:
        void newRideAdded(RideItem*);
        void cpintTimeValueEntered();
        void cpintSetCPButtonClicked();
        void pickerMoved(const QPoint &pos);
        void rideSelected();
        void seasonSelected(int season);
        void setSeries(int index);
        void resetSeasons();
        void filterChanged();
        void dateRangeChanged(DateRange);

    private:
        void updateCpint(double minutes);

        QString _dateRange;

    protected:

        QDir home;
        CpintPlot *cpintPlot;
        MainWindow *mainWindow;
        QLineEdit *cpintTimeValue;
        QLabel *cpintTodayValue;
        QLabel *cpintAllValue;
        QLabel *cpintCPValue;
        QComboBox *seriesCombo;
        QComboBox *cComboSeason;
        QPushButton *cpintSetCPButton;
        QwtPlotPicker *picker;
        void addSeries();
        Seasons *seasons;
        QList<Season> seasonsList;
        RideItem *currentRide;
        QList<RideFile::SeriesType> seriesList;
#ifdef GC_HAVE_LUCENE
        SearchFilterBox *searchBox;
#endif

        bool rangemode;
};

#endif // _GC_CriticalPowerWindow_h

