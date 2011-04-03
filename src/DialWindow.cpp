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


#include "DialWindow.h"

DialWindow::DialWindow(MainWindow *mainWindow) :
    GcWindow(mainWindow), mainWindow(mainWindow)
{
    setContentsMargins(0,0,0,0);
    setInstanceName("Dial");
    resetValues();

    QWidget *c = new QWidget;
    QVBoxLayout *cl = new QVBoxLayout(c);
    setControls(c);

    // setup the controls
    QFormLayout *controlsLayout = new QFormLayout();
    controlsLayout->setSpacing(0);
    controlsLayout->setContentsMargins(5,5,5,5);
    cl->addLayout(controlsLayout);

    // data series selection
    QLabel *seriesLabel = new QLabel(tr("Data Series"), this);
    seriesSelector = new QComboBox(this);
    foreach (RealtimeData::DataSeries x, RealtimeData::listDataSeries()) {
        seriesSelector->addItem(RealtimeData::seriesName(x), static_cast<int>(x));
    }
    controlsLayout->addRow(seriesLabel, seriesSelector);

    // display label...
    QVBoxLayout *layout = new QVBoxLayout(this);
    valueLabel = new QLabel(this);
    valueLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    layout->addWidget(valueLabel);

    // get updates..
    connect(mainWindow, SIGNAL(telemetryUpdate(RealtimeData)), this, SLOT(telemetryUpdate(RealtimeData)));

    // setup fontsize etc
    resizeEvent(NULL);
}

void
DialWindow::lap(int lapnumber)
{
    lapNumber = lapnumber;
    avgLap = 0;
}

void
DialWindow::start()
{
    resetValues();
}

void
DialWindow::stop()
{
    resetValues();
}

void
DialWindow::pause()
{
}

void
DialWindow::telemetryUpdate(RealtimeData rtData)
{
    // we got some!
    RealtimeData::DataSeries series = static_cast<RealtimeData::DataSeries>(seriesSelector->itemData(seriesSelector->currentIndex()).toInt());
    double value = rtData.value(series);
    valueLabel->setText(QString("%1").arg(round(value)));
}

void DialWindow::resizeEvent(QResizeEvent * )
{
   QFont font;
   font.setPointSize(geometry().height()/2);
   font.setWeight(QFont::Bold);
   valueLabel->setFont(font);
}
