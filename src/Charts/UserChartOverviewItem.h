/*
 * Copyright (c) 2021 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_UserChartOverviewItem
#define _GC_UserChartOverviewItem_h 1

// basics
#include "ChartSpace.h"
#include "OverviewItems.h"
#include "GenericChart.h"
#include "UserChart.h"

#include <QGraphicsProxyWidget>

class QLineEdit;
class UserChartOverviewItem : public ChartSpaceItem
{
    Q_OBJECT

    public:

        UserChartOverviewItem(ChartSpace *parent, QString name, QString config);
        ~UserChartOverviewItem();

        void dragging(bool) override;
        void itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;
        void itemGeometryChanged() override;
        void setData(RideItem *item) override;
        void setDateRange(DateRange) override;
        QColor color() override;
        QWidget *config()  override { return chart->settingsTool(); }

        // create a blank one
        static ChartSpaceItem *create(ChartSpace *parent) { return new UserChartOverviewItem(parent, "User Chart",
                                                                        "{ \"title\": \" \",\n\"description\": \"A description of the chart, mostly useful when the chart is uploaded to the cloud to let others know what this chart is useful for etc. \",\n\"type\": 2,\n\"animate\": false,\n\"legendpos\": 2,\n\"stack\": false,\n\"orientation\": 1,\"scale\": 2.5, \n\"SERIES\": [\n{ \"name\": \"Power \", \"group\": \" \", \"xname\": \"Cadence \", \"yname\": \"watts \", \"program\": \"{\\n    finalise {\\n        # we just fetch samples at end\\n        xx <- samples(CADENCE);\\n        yy <- samples(POWER);\\n    }\\n\\n    x { xx; }\\n    y { yy; }\\n}\\n \", \"line\": 0, \"symbol\": 1, \"size\": 14, \"color\": \"#010112\", \"opacity\": 100, \"legend\": true, \"opengl\": false, \"datalabels\": false, \"fill\": false},\n{ \"name\": \"LR \", \"group\": \" \", \"xname\": \"Cadence \", \"yname\": \"watts \", \"program\": \"{\\n    finalise {\\n        # we just fetch samples at end\\n        xx <- samples(CADENCE);\\n        yy <- samples(POWER);\\n        fit <- lr(xx,yy);\\n        ex <- c(10,120);\\n        wy <- c((ex[0]*fit[0])+fit[1],\\n                (ex[1]*fit[0])+fit[1]);\\n                \\n    }\\n\\n    x { ex; }\\n    y { wy; }\\n}\\n \", \"line\": 2, \"symbol\": 1, \"size\": 3, \"color\": \"#01010b\", \"opacity\": 100, \"legend\": false, \"opengl\": false, \"datalabels\": false, \"fill\": false} ]\n,\n\"AXES\": [\n{ \"name\": \"Cadence \", \"type\": 0, \"orientation\": 1, \"align\": 1, \"minx\": 0, \"maxx\": 160, \"miny\": 0, \"maxy\": 0, \"visible\": true, \"fixed\": true, \"log\": false, \"minorgrid\": false, \"majorgrid\": true, \"labelcolor\": \"#6569a5\", \"axiscolor\": \"#6569a5\"},\n{ \"name\": \"watts \", \"type\": 0, \"orientation\": 2, \"align\": 1, \"minx\": 0, \"maxx\": 0, \"miny\": 0, \"maxy\": 1000, \"visible\": true, \"fixed\": false, \"log\": false, \"minorgrid\": false, \"majorgrid\": true, \"labelcolor\": \"#010112\", \"axiscolor\": \"#010112\"} ]\n} "); }

        // chartspace
        ChartSpace *chartspace() const { return space_; }

        // settings get and set
        QString getConfig() const { return chart->settings(); }
        void setConfig(QString config) { chart->applySettings(config); }

    protected:
        bool sceneEvent(QEvent *) override { return false; } // override the default one

    public slots:

        void nameChanged();
        void configChanged(qint32) override;

    private:
        // embedding
        QGraphicsProxyWidget *proxy;
        UserChart *chart;
        ChartSpace *space_;

        // we need to edit the tile name since we use a
        // custom config tool, it gets inserted into the
        // standard user chart config dialog
        QLineEdit *nameEdit;
};

#endif // _GC_UserChartOverviewItem_h
