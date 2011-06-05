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

#ifndef _GC_LTMTool_h
#define _GC_LTMTool_h 1

#include "MainWindow.h"
#include "Season.h"
#include "RideMetric.h"
#include "LTMSettings.h"

#include <QDir>
#include <QtGui>

// tree widget types
#define ROOT_TYPE   1
#define DATE_TYPE   2
#define METRIC_TYPE 3

#define SYS_DATE    1
#define USER_DATE   2


class LTMTool : public QWidget
{
    Q_OBJECT

    public:

        LTMTool(MainWindow *parent, const QDir &home);

        const Season *currentDateRange() { return dateRange; }
        void selectDateRange(int);
        QList<QTreeWidgetItem *> selectedMetrics() { return metricTree->selectedItems(); }

        QString metricName(QTreeWidgetItem *);
        QString metricSymbol(QTreeWidgetItem *);
        MetricDetail metricDetails(QTreeWidgetItem *);

        // allow others to create and update season structures
        int newSeason(QString, QDate, QDate, int);
        void updateSeason(int, QString, QDate, QDate, int);

        // apply settings to the metric selector
        void applySettings(LTMSettings *);

        // Helper function for default charts translation
        void translateDefaultCharts(QList<LTMSettings>&charts);

    signals:

        void dateRangeSelected(const Season *);
        void metricSelected();

    private slots:
        void dateRangeTreeWidgetSelectionChanged();
        void dateRangePopup(QPoint);
        void dateRangeChanged(QTreeWidgetItem *, int);
        void renameRange();
        void editRange();
        void deleteRange();
        void metricTreeWidgetSelectionChanged();
        void metricTreePopup(QPoint);
        void colorPicker();
        void editMetric();
        void configChanged();
        void readSeasons();
        void writeSeasons();

    private:

        QwtPlotCurve::CurveStyle curveStyle(RideMetric::MetricType);
        QwtSymbol::Style symbolStyle(RideMetric::MetricType);

        const QDir home;
        MainWindow *main;
        bool useMetricUnits;

        QList<Season> seasons;
        QTreeWidget *dateRangeTree;
        QTreeWidgetItem *allDateRanges;
        const Season *dateRange;

        QList<MetricDetail> metrics;
        QTreeWidget *metricTree;
        QTreeWidgetItem *allMetrics;

        QTreeWidgetItem *activeDateRange; // when using context menus
        QTreeWidgetItem *activeMetric; // when using context menus

        QSplitter   *ltmSplitter;
};

class EditMetricDetailDialog : public QDialog
{
    Q_OBJECT

    public:
        EditMetricDetailDialog(MainWindow *, MetricDetail *);

    public slots:
        void colorClicked();
        void applyClicked();
        void cancelClicked();

    private:
        MainWindow *mainWindow;
        MetricDetail *metricDetail;

        QLineEdit *userName,
                  *userUnits;

        QComboBox *curveStyle,
                  *curveSymbol;
        QPushButton *curveColor;
        QDoubleSpinBox *showBest,
                       *baseLine;
        QCheckBox *curveSmooth,
                  *curveTrend;

        QPushButton *applyButton, *cancelButton;

        QColor penColor; // chosen from color Picker
        void setButtonIcon(QColor);
};

#endif // _GC_LTMTool_h

