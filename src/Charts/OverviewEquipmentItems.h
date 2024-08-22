/*
 * Copyright (c) 2024 Paul Johnson (paulj49457@gmail.com)
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

#ifndef _GC_OverviewEquipmentItem_h
#define _GC_OverviewEquipmentItem_h 1

#include <QMap>
#include <memory> // Atomics
#include "OverviewItems.h"

// to allow integral type atomics (c++11) to be used and to get them to hold values to 3 decimal places the following
// factors are used to scale the values, this is sufficient for equipment overview usage. When c++23 is available
// this can changed to use atomic<double> and the scaling can be removed.
#define EQ_REAL_TO_SCALED 10000
#define EQ_SCALED_TO_REAL 0.0001

class ColorButton;

class EqTimeWindow
{
public:
    EqTimeWindow();
    EqTimeWindow(const QString& eqLinkName, bool startSet, const QDate& startDate, bool endSet, const QDate& endDate);

    QString eqLinkName_;
    bool startSet_, endSet_;
    QDate startDate_, endDate_;

    bool isWithin(const QDate& actDate) const;
    bool rangeIsValid() const;
};

//
// Configuration widget for ALL Overview Items
//
class OverviewEquipmentItemConfig : public OverviewItemConfig
{
    Q_OBJECT

    public:

        OverviewEquipmentItemConfig(ChartSpaceItem *);
        virtual ~OverviewEquipmentItemConfig() {};

        static bool registerItems();

    public slots:

        // retrieve values when user edits them (if they're valid)
        virtual void dataChanged() override;

        // set the config widgets to reflect current config
        virtual void setWidgets() override;

        void addEqLinkRow();
        void removeEqLinkRow();
        void tableCellClicked(int row, int column);

    protected:

        // before show, lets make sure the background color and widgets are set correctly
        void showEvent(QShowEvent *) override { setWidgets(); }

    private:

        void setEqLinkRowWidgets(int tableRow, const EqTimeWindow* eqUse);

        // Equipment items
        QLineEdit* eqLinkName;
        QLineEdit *nonGCDistance, *nonGCElevation;
        QLineEdit *replaceDistance, *replaceElevation;

        QTableWidget *eqTimeWindows;
        QPushButton *addEqLink, *removeEqLink;

        QCheckBox *startSet, *endSet;
        QDateEdit *startDate, *endDate;

        QPlainTextEdit *notes;
};

class EquipmentItem : public ChartSpaceItem
{
    Q_OBJECT

    public:

        EquipmentItem(ChartSpace* parent, const QString& name);
        EquipmentItem(ChartSpace *parent, const QString& name, QVector<EqTimeWindow>& eqLinkUse,
                        const uint64_t nonGCDistanceScaled, const uint64_t nonGCElevationScaled,
                        const uint64_t repDistanceScaled, const uint64_t repElevationScaled,
                        const QString& notes);
        virtual ~EquipmentItem() {}

        void itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;
        void configChanged(qint32) override;

        // The following don't apply to the Equipment Item.
        void setData(RideItem*) override {}
        void setDateRange(DateRange) override {}
        void itemGeometryChanged() override {}

        bool isWithin(const QDate& actDate) const;
        bool isWithin(const QString& rideEqLinkName, const QDate& actDate) const;
        bool isWithin(const QStringList& rideEqLinkNameList, const QDate& actDate) const;
        bool rangeIsValid() const;

        void resetEqItem();
        void unitsChanged();
        void addActivity(uint64_t rideDistanceScaled, uint64_t rideElevationScaled, uint64_t rideTimeInSecs);
                        uint64_t getNumActivities() const { return activities_; }

        void setNonGCDistance(double nonGCDistance);
        uint64_t getNonGCDistanceScaled() const { return nonGCDistanceScaled_; }
        uint64_t getGCDistanceScaled() const { return gcDistanceScaled_; }
        uint64_t getTotalDistanceScaled() const { return totalDistanceScaled_; }

        void setNonGCElevation(double nonGCElevation);
        uint64_t getNonGCElevationScaled() const { return nonGCElevationScaled_; }
        uint64_t getGCElevationScaled() const { return gcElevationScaled_; }
        uint64_t getTotalElevationScaled() const { return totalElevationScaled_; }

        QWidget *config() override { return configwidget_; }

        // create and config
        static ChartSpaceItem* create(ChartSpace* parent) {
            return new EquipmentItem(parent, tr("Equipment Item")); }

        QVector<EqTimeWindow> eqLinkUse_;
        uint64_t repDistanceScaled_, repElevationScaled_;
        QString notes_;

        OverviewEquipmentItemConfig* configwidget_;

    private:
        std::atomic<uint64_t> activities_;
        std::atomic<uint64_t> activityTimeInSecs_;

        uint64_t nonGCDistanceScaled_;
        std::atomic<uint64_t> gcDistanceScaled_;
        std::atomic<uint64_t> totalDistanceScaled_;

        uint64_t nonGCElevationScaled_;
        std::atomic<uint64_t> gcElevationScaled_;
        std::atomic<uint64_t> totalElevationScaled_;

        QColor inactiveColor, textColor, alertColor;
};

class EquipmentSummary : public ChartSpaceItem
{
    Q_OBJECT

    public:

        EquipmentSummary(ChartSpace* parent, const QString& name, const QString& eqLinkName, bool showActivitiesPerAthlete);
        virtual ~EquipmentSummary() {}

        void itemPaint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override;
        void configChanged(qint32) override;

        // The following don't apply to the Equipment Summary Item.
        void setData(RideItem*) override {}
        void setDateRange(DateRange) override {}
        void itemGeometryChanged() override {}

        void resetAthleteActivity();
        void addAthleteActivity(const QString& athleteName);
        void updateSummaryItem(const uint64_t eqNumActivities, const uint64_t eqTotalTimeInSecs,
                                const uint64_t eqTotalDistanceScaled, const uint64_t eqTotalElevationScaled,
                                const QDate& earliestDate, const QDate& eqLinkLatestDate_);

        QWidget* config() override { return configwidget_; }

        // create and config
        static ChartSpaceItem* create(ChartSpace* parent) {
            return new EquipmentSummary(parent, tr("Summary"), "", true); }

        QString eqLinkName_;
        bool showActivitiesPerAthlete_;
        QDate eqLinkEarliestDate_, eqLinkLatestDate_;

        OverviewEquipmentItemConfig* configwidget_;

    private:

        QColor textColor;
        uint64_t eqLinkTotalTimeInSecs_;
        uint64_t eqLinkTotalDistanceScaled_;
        uint64_t eqLinkTotalElevationScaled_;
        uint64_t eqLinkNumActivities_;
        
        QMutex athleteActivityMutex_;
        QMap<QString, uint32_t> athleteActivityMap_;
};

class EquipmentNotes : public ChartSpaceItem
{
    Q_OBJECT

    public:

        EquipmentNotes(ChartSpace* parent, const QString& name, const QString& notes);
        virtual ~EquipmentNotes() {}

        void itemPaint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override;
        void configChanged(qint32) override;

        // The following don't apply to the Equipment Notes Item.
        void setData(RideItem*) override {}
        void setDateRange(DateRange) override {}
        void itemGeometryChanged() override {}

        QWidget* config() override { return configwidget_; }

        // create and config
        static ChartSpaceItem* create(ChartSpace* parent) {
            return new EquipmentNotes(parent, tr("Notes"), "");
        }

        QString notes_;

        OverviewEquipmentItemConfig* configwidget_;

    private:

        QColor textColor;

};

#endif // _GC_OverviewEquipmentItem_h
