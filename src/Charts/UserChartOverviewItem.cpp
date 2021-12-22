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

#include "UserChartOverviewItem.h"
#include "UserChart.h"

UserChartOverviewItem::UserChartOverviewItem(ChartSpace *parent, QString name, QString settings) : ChartSpaceItem(parent, name), space_(parent)
{

    this->type = OverviewItemType::USERCHART;

    // default is a bit bigger
    span = 2;
    deep = 30;

    // create the chart and place on scene
    chart = new UserChart(NULL, parent->context, parent->scope == OverviewScope::TRENDS ? true : false, StandardColor(CCARDBACKGROUND).name());
    chart->hide();
    proxy = parent->getScene()->addWidget(chart);
    proxy->setParent(this);
    proxy->setZValue(20); // tile is 10, dragging is 100
    chart->setGraphicsItem(proxy);// only watch the smallest space

    // name editor
    QFormLayout *form = new QFormLayout();
    nameEdit = new QLineEdit(parent);
    nameEdit->setText(name);
    form->addRow(new QLabel(tr("Name")), nameEdit);
    chart->settingsTool()->insertLayout(form);

    // settings... a boring one on first showing.
    setConfig(settings);

    connect(nameEdit, SIGNAL(textEdited(QString)), this, SLOT(nameChanged()));
}

UserChartOverviewItem::~UserChartOverviewItem() { }

QColor
UserChartOverviewItem::color()
{
    return QColor(chart->chartinfo.bgcolor);
}

void
UserChartOverviewItem::configChanged(qint32)
{
    //chart->setBackgroundColor(GColor(CCARDBACKGROUND));
}

void
UserChartOverviewItem::nameChanged()
{
    name = nameEdit->text();
}

void
UserChartOverviewItem::setData(RideItem *item)
{
    if (item == NULL || item->ride() == NULL) return;
    chart->setRide(item);
}
void
UserChartOverviewItem::setDateRange(DateRange dr)
{
    chart->setDateRange(dr);
}

void
UserChartOverviewItem::dragging(bool /*isdragging*/)
{
    // we previously hid user charts due to rendering speed
    // and how it affects the user experience when dragging charts
    // around, but this was an artefact of a legend bug
    // see github issue:
    //
    // https://github.com/GoldenCheetah/GoldenCheetah/issues/3989
    //
    // so we don't worry about hiding during drag operations
    // any more, but left here in case it becomes an issue
    // in the future
    //if (isdragging) chart->hide();
    //else chart->show();
}

void
UserChartOverviewItem::itemGeometryChanged() {

    QRectF geom = geometry();
    proxy->setGeometry(QRectF(geom.x()+20,geom.y()+(ROWHEIGHT*2),geom.width()-40, geom.height()-((ROWHEIGHT*2)+20)));

    if (drag) chart->hide();
    else /*if (parent->state != ChartSpace::DRAG)*/ chart->show();
}

// proxy and chart does the work for us
void
UserChartOverviewItem::itemPaint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) { }
