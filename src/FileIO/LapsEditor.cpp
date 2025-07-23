/*
 * Copyright (c) 2015 Alejandro Martinez (amtriathlon@gmail.com)
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

#include "LapsEditor.h"
#include "RideItem.h"
#include "Settings.h"
#include "Colors.h"
#include "Units.h"
#include "HelpWhatsThis.h"
#include "Context.h"



LapsEditorWidget::LapsEditorWidget
(QWidget *parent)
: QWidget(parent)
{
    qRegisterMetaType<QList<RideFilePoint*>>("QList<RideFilePoint*>");

    repDelegate.setRange(1, 100);

    woDistDelegate.setShowSuffixOnEdit(true);
    woDistDelegate.setShowSuffixOnDisplay(true);

    woDurDelegate.setFormat("mm:ss");

    reDistDelegate.setShowSuffixOnEdit(true);
    reDistDelegate.setShowSuffixOnDisplay(true);

    reDurDelegate.setFormat("mm:ss");

    setSwim(false);

    tree = new QTreeWidget();
    tree->headerItem()->setText(0, tr("Repetitions"));
    tree->headerItem()->setText(1, tr("Work Distance"));
    tree->headerItem()->setText(2, tr("Work Duration"));
    tree->headerItem()->setText(3, tr("Rest Distance"));
    tree->headerItem()->setText(4, tr("Rest Duration"));
    tree->setColumnCount(5);
    tree->setItemDelegateForColumn(0, &repDelegate);
    tree->setItemDelegateForColumn(1, &woDistDelegate);
    tree->setItemDelegateForColumn(2, &woDurDelegate);
    tree->setItemDelegateForColumn(3, &reDistDelegate);
    tree->setItemDelegateForColumn(4, &reDurDelegate);
    basicTreeWidgetStyle(tree);

    ActionButtonBox *actionButtons = new ActionButtonBox(ActionButtonBox::AddDeleteGroup);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(tree);
    layout->addWidget(actionButtons);

    actionButtons->defaultConnect(tree);
    connect(actionButtons, &ActionButtonBox::addRequested, this, &LapsEditorWidget::addClicked);
    connect(actionButtons, &ActionButtonBox::deleteRequested, this, &LapsEditorWidget::deleteClicked);
    connect(&repDelegate, &QStyledItemDelegate::closeEditor, this, &LapsEditorWidget::editingFinished);
    connect(&woDistDelegate, &QStyledItemDelegate::closeEditor, this, &LapsEditorWidget::editingFinished);
    connect(&woDurDelegate, &QStyledItemDelegate::closeEditor, this, &LapsEditorWidget::editingFinished);
    connect(&reDistDelegate, &QStyledItemDelegate::closeEditor, this, &LapsEditorWidget::editingFinished);
    connect(&reDurDelegate, &QStyledItemDelegate::closeEditor, this, &LapsEditorWidget::editingFinished);

    setLayout(layout);
}


LapsEditorWidget::~LapsEditorWidget
()
{
    for (RideFilePoint *point : dataPoints_) {
        delete point;
    }
}


void
LapsEditorWidget::setSwim
(bool swim)
{
    isSwim = swim;

    if (isSwim) {
        bool metricPace = appsettings->value(this, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool();
        woDistDelegate.setRange(0, 10000);
        woDistDelegate.setDecimals(0);
        woDistDelegate.setSuffix(metricPace ? tr("m") : tr("yd"));
        reDistDelegate.setRange(0, 10000);
        reDistDelegate.setDecimals(0);
        reDistDelegate.setSuffix(metricPace ? tr("m") : tr("yd"));
    } else {
        bool metricPace = appsettings->value(this, GC_PACE, GlobalContext::context()->useMetricUnits).toBool();
        woDistDelegate.setRange(0, 100);
        woDistDelegate.setDecimals(3);
        woDistDelegate.setSuffix(metricPace ? tr("km") : tr("mi"));
        reDistDelegate.setRange(0, 100);
        reDistDelegate.setDecimals(3);
        reDistDelegate.setSuffix(metricPace ? tr("km") : tr("mi"));
    }
    emit editingFinished();
}


const QList<RideFilePoint*>&
LapsEditorWidget::dataPoints
()
{
    // Remove point from previous activation
    for (RideFilePoint *point : dataPoints_) {
        delete point;
    }
    dataPoints_.clear();

    // Select metric units setting according to sport
    bool metricPace = appsettings->value(this, isSwim ? GC_SWIMPACE : GC_PACE, GlobalContext::context()->useMetricUnits).toBool();
    double distance_factor = isSwim ? (metricPace ? 1.0 : METERS_PER_YARD) :
                                      (metricPace ? 1.0 : KM_PER_MILE) * 1000.0;

    // generate new sample points from laps
    int secs = 0;
    double km = 0.0;
    int interval = 1;
    for (int i = 0; i < tree->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *rowItem = tree->invisibleRootItem()->child(i);
        int laps = rowItem->data(0, Qt::EditRole).toInt();
        for (int lap = 1; lap <= laps; lap++) {
            for (int j = 0; j < 2; j++) {
                double lap_dist = rowItem->data(2 * j + 1, Qt::EditRole).toDouble() * distance_factor;
                QTime dur = rowItem->data(2 * j + 2, Qt::EditRole).toTime();
                if (! dur.isValid()) {
                    continue;
                }
                int lap_secs = dur.minute() * 60 + dur.second();
                double kph = (lap_secs > 0) ? 3.6 * lap_dist/lap_secs : 0.0;
                for (int s = 0; s < lap_secs; s++) {
                    secs++;
                    km += kph / 3600.0;
                    dataPoints_.append(new RideFilePoint(secs, 0.0, 0.0, km, kph,                                     0.0, 0.0, 0.0, 0.0, 0.0,
                                                         0.0, 0.0,
                                                         RideFile::NA, RideFile::NA,
                                                         0.0, 0.0, 0.0, 0.0,
                                                         0.0, 0.0,
                                                         0.0, 0.0, 0.0, 0.0,
                                                         0.0, 0.0, 0.0, 0.0,
                                                         0.0, 0.0,
                                                         0.0, 0.0, 0.0,
                                                         0.0,
                                                         interval));
                }
                interval++;
            }
        }
    }
    return dataPoints_;
}


void
LapsEditorWidget::addClicked
()
{
    int index = tree->invisibleRootItem()->childCount();
    QTreeWidgetItem *add = new QTreeWidgetItem();
    add->setFlags(add->flags() | Qt::ItemIsEditable);
    tree->invisibleRootItem()->insertChild(index, add);
    add->setData(0, Qt::DisplayRole, 1);

    emit editingFinished();
}


void
LapsEditorWidget::deleteClicked
()
{
    if (tree->currentItem()) {
        int index = tree->invisibleRootItem()->indexOfChild(tree->currentItem());
        delete tree->invisibleRootItem()->takeChild(index);

        emit editingFinished();
    }
}
