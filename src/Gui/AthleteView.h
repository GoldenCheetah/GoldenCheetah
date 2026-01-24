/*
 * Copyright (c) 2020 Mark Liversedge <liversedge@gmail.com>
 * Additional functionality Copyright (c) 2026 Paul Johnson (paulj49457@gmail.com)
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

#include "ChartSpace.h"
#include "OverviewItems.h"
#include <QPoint>

class AthleteView : public ChartSpace
{
    Q_OBJECT

public:
    AthleteView(Context *context);

    void currentAthlete(const QString& name);

protected slots:
    void configChanged(qint32);
    void configItem(ChartSpaceItem*, QPoint);
    void newAthlete(QString);
    void deleteAthlete(QString);
};

// the athlete display
class AthleteCard : public ChartSpaceItem
{
    Q_OBJECT

    public:

        AthleteCard(ChartSpace *parent, QString name);

        void itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
        void dragChanged(bool) override;
        void itemGeometryChanged() override;
        void setData(RideItem *) override {}
        void setDateRange(DateRange) override {}
        void configChanged(qint32) override;
        QWidget *config() { return new OverviewItemConfig(this); }

        // create and config
        static ChartSpaceItem *create(ChartSpace *parent) { return new AthleteCard(parent, ""); }

        void currentAthlete(bool status);
        void configAthlete();

    protected slots:

        // opening/closing etc
        void opening(QString, Context*);
        void closing(QString, Context*);
        void loadProgress(QString, double);
        void loadDone(QString, Context*);

        // metric refreshing
        void refreshStart();
        void refreshEnd();
        void refreshUpdate(QDate);

        // refresh stats on last workouts etc
        void refreshStats(RideItem * item = nullptr);

        void openCloseAthlete();

    private:
        double loadprogress;
        Context *context_;
        QImage avatar;
        inline static QImage locked;

        Button *openCloseButton;
        Button *deleteButton;
        Button *backupButton;
        Button *saveAllUnsavedRidesButton;

        bool refresh;
        int actualActivities; // total actual activities
        int plannedActivities; // total planned activities
        int unsavedActivities; // number of unsaved activities
        QDateTime lastActivity; // date of last activity recorded

        bool selectedAthlete;
        const bool anchor; // holds app context so not allowed to close it

};

