/*
 * Copyright (c) 2020 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_MetricSelect_h
#define _GC_MetricSelect_h 1

#include <QWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QCompleter>
#include <QMap>

#include "RideMetric.h"
#include "RideMetadata.h"
#include "Context.h"
#include "Athlete.h"

// for settings dialogs etc, when we want the name of a metric
// or metadata field, we can use this lineedit to help users
// and tell us if the field is valid or not
class MetricSelect : public QLineEdit
{
    Q_OBJECT

    public:

        enum { Metric=0x01, Meta=0x02 };

        MetricSelect(QWidget *parent, Context *context, int scope = Metric);

        void setSymbol(QString symbol);
        const RideMetric *rideMetric();
        QString metaname();
        void setMeta(QString);

        // check what we got
        bool isValid();
        bool isMeta();
        bool isMetric();

    private:

        Context *context;
        int scope;

        const RideMetric *_metric;
        QCompleter *completer;

        QStringList listText;
        QMap<QString, const RideMetric *> metricMap;
        QMap<QString, QString> metaMap;
};

class SeriesSelect : public QComboBox
{
    Q_OBJECT

    public:

        // all possible ride series, or just those with ridecache MMP or
        // just those that have zone settings available
        enum { All, MMP, Zones };

        SeriesSelect(QWidget *parent, int scope = All);

        void setSeries(RideFile::SeriesType);
        RideFile::SeriesType series();

    private:
        int scope;
};

class MultiMetricSelector : public QWidget
{
    Q_OBJECT

    public:
        MultiMetricSelector(const QString &leftLabel, const QString &rightLabel, const QStringList &selectedMetrics, QWidget *parent = nullptr);

        void setSymbols(const QStringList &selectedMetrics);
        QStringList getSymbols() const;

        void updateMetrics();

    signals:
        void selectedChanged();

    private:
        QLineEdit *filterEdit;
        QListWidget *availList;
        QListWidget *selectedList;
#ifndef Q_OS_MAC
        QToolButton *selectButton;
        QToolButton *unselectButton;
#else
        QPushButton *selectButton;
        QPushButton *unselectButton;
#endif

    private slots:
        void filterAvail(const QString &filter);
        void upClicked();
        void downClicked();
        void unselectClicked();
        void selectClicked();
        void updateSelectionButtons();
};

#endif
