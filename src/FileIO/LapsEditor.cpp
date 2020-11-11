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

#include <QHeaderView>

// QTableWidgetItem to accept only non-negative integers
class QTableWidgetItemUInt : public QTableWidgetItem {
    public:
    void setData (int role, const QVariant & value) {
        QTableWidgetItem::setData(role, value.toUInt());
    }
};

// QTableWidgetItem to accept only non-negative doubles
class QTableWidgetItemUDouble : public QTableWidgetItem {
    public:
    void setData (int role, const QVariant & value) {
        QTableWidgetItem::setData(role, value.toDouble() > 0.0 ? value.toDouble() : 0.0);
    }
};

LapsEditor::LapsEditor(bool isSwim) : isSwim(isSwim)
{
    setWindowTitle(tr("Laps Editor"));
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Activity_Manual_LapsEditor));
#ifdef Q_OS_MAC
    setFixedSize(625,415);
#else
    setFixedSize(630 *dpiXFactor,420 *dpiYFactor);
#endif

    //
    // Create the GUI widgets
    //

    // Laps table
    const int nCols = 7;
    const int nRows = 100;
    tableWidget = new QTableWidget(nRows, nCols, this);
    QStringList hLabels;
    tableWidget->setColumnWidth(0,  48);
    for (int j = 1; j < nCols; j++) tableWidget->setColumnWidth(j, 88*dpiXFactor);
    hLabels<<tr("reps")<<tr("work dist")<< tr("work min")<<tr("work sec")<<tr("rest dist")<<tr("rest min")<<tr("rest sec");
    tableWidget->setHorizontalHeaderLabels(hLabels);
    tableWidget->verticalHeader()->setVisible(false);
    for (int i = 0; i < nRows; i++) {
        QTableWidgetItemUInt *item = new QTableWidgetItemUInt();
        item->setTextAlignment(Qt::AlignRight);
        item->setData(Qt::EditRole, 1);
        tableWidget->setItem(i, 0, item);
        for (int j = 1; j < nCols; j++) {
            QTableWidgetItem *item;
            if (!isSwim && (j == 1 || j == 4))
                item = new QTableWidgetItemUDouble();
            else
                item = new QTableWidgetItemUInt();
            item->setTextAlignment(Qt::AlignRight);
            item->setData(Qt::EditRole, 0);
            tableWidget->setItem(i, j, item);
        }
    }

    // buttons
    okButton = new QPushButton(tr("&OK"), this);
    cancelButton = new QPushButton(tr("&Cancel"), this);

    // save by default -- we don't overwrite and
    // the user will expect enter to save laps
    okButton->setDefault(true);
    cancelButton->setDefault(false);

    //
    // LAY OUT THE GUI
    //
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *table = new QHBoxLayout;
    mainLayout->addLayout(table);
    table->addWidget(tableWidget);

    QHBoxLayout *buttons = new QHBoxLayout;
    mainLayout->addLayout(buttons);
    buttons->addStretch();
    buttons->addWidget(okButton);
    buttons->addWidget(cancelButton);

    // dialog buttons
    connect(okButton, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
}

LapsEditor::~LapsEditor() {
    foreach(RideFilePoint *point, dataPoints_)
        delete point;
}

void
LapsEditor::cancelClicked()
{
    reject();
}

void
LapsEditor::okClicked()
{
    // Remove point from previous activation
    foreach(RideFilePoint *point, dataPoints_)
        delete point;
    dataPoints_.clear();

    // Select metric units setting according to sport
    bool metricPace = appsettings->value(this, isSwim ? GC_SWIMPACE : GC_PACE, GlobalContext::context()->useMetricUnits).toBool();
    double distance_factor = isSwim ? (metricPace ? 1.0 : METERS_PER_YARD) :
                                      (metricPace ? 1.0 : KM_PER_MILE) * 1000.0;

    // generate new sample points from laps
    int secs = 0;
    double km = 0.0;
    int interval = 1;
    for (int i = 0; i < tableWidget->rowCount(); i++) {
        int laps = tableWidget->item(i, 0)->data(Qt::EditRole).toInt();
        for (int lap = 1; lap <= laps; lap++) {
            for (int j = 0; j < 2; j++) {
                double lap_dist = tableWidget->item(i, 3*j+1)->data(Qt::EditRole).toDouble() * distance_factor;
                int lap_secs = 60*tableWidget->item(i, 3*j+2)->data(Qt::EditRole).toInt() + tableWidget->item(i, 3*j+3)->data(Qt::EditRole).toInt();
                double kph = (lap_secs > 0) ? 3.6*lap_dist/lap_secs : 0.0;
                for (int s = 0; s < lap_secs; s++) {
                    secs++;
                    km += kph/3600.0;
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
    accept();
}
