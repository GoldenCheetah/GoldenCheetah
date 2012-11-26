/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_TreeMapWindow_h
#define _GC_TreeMapWindow_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <QTimer>
#include "MainWindow.h"
#include "MetricAggregator.h"
#include "RideMetadata.h"
#include "Season.h"
#include "TreeMapPlot.h"
#include "LTMPopup.h"
#include "LTMTool.h"
#include "LTMSettings.h"
#include "LTMWindow.h"
#include "LTMCanvasPicker.h"
#include "GcPane.h"

#include <math.h>

#include <qwt_plot_picker.h>
#include <qwt_text_engine.h>

class TreeMapPlot;

class TreeMapWindow : public GcWindow
{
    Q_OBJECT
    G_OBJECT

    Q_PROPERTY(QString f1 READ f1 WRITE setf1 USER true)
    Q_PROPERTY(QString f2 READ f2 WRITE setf2 USER true)
#ifdef GC_HAVE_LUCENE
    Q_PROPERTY(QString filter READ filter WRITE setFilter USER true)
#endif
    Q_PROPERTY(LTMSettings settings READ getSettings WRITE applySettings USER true)

    public:

        MainWindow *main; // used by zones shader
        TreeMapWindow(MainWindow *, bool, const QDir &);
        ~TreeMapWindow();

        QString f1() const { return field1->currentText(); }
        void setf1(QString x) const { field1->setCurrentIndex(field1->findText(x)); }
        QString f2() const { return field2->currentText(); }
        void setf2(QString x) const { field2->setCurrentIndex(field1->findText(x)); }
        LTMSettings getSettings() const { return settings; }
        void applySettings(LTMSettings x) { ltmTool->applySettings(&x); }
#ifdef GC_HAVE_LUCENE
        QString filter() const { return ltmTool->searchBox->filter(); }
        void setFilter(QString x) { ltmTool->searchBox->setFilter(x); }
#endif

    public slots:
        void rideSelected();
        void refreshPlot();
        void dateRangeChanged(DateRange);
        void metricSelected();
        void filterChanged();
        void refresh();
        void fieldSelected(int);
        void pointClicked(QwtPlotCurve*, int);
        int groupForDate(QDate, int);

    private:
        // passed from MainWindow
        QDir home;
        bool useMetricUnits;

        // popup - the GcPane to display within
        //         and the LTMPopup contents widdget
        GcPane *popup;
        LTMPopup *ltmPopup;

        // local state
        bool active;
        bool dirty;
        QList<KeywordDefinition> keywordDefinitions;
        QList<FieldDefinition>   fieldDefinitions;
        LTMSettings settings; // all the plot settings
        QList<SummaryMetrics> results;
        QList<SummaryMetrics> measures;

        // Widgets
        QVBoxLayout *mainLayout;
        TreeMapPlot *ltmPlot;
        LTMTool *ltmTool;
        QLabel *title;

        QComboBox *field1,
                  *field2;
        void addTextFields(QComboBox *);
};

#endif // _GC_TreeMapWindow_h
