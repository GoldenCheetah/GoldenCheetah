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
    seriesLabel->setAutoFillBackground(true);
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
    connect(seriesSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(seriesChanged()));
    connect(mainWindow, SIGNAL(configChanged()), this, SLOT(seriesChanged()));

    // setup colors
    seriesChanged();

    // setup fontsize etc
    resizeEvent(NULL);

    // set to zero
    telemetryUpdate(RealtimeData());
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
DialWindow::telemetryUpdate(const RealtimeData &rtData)
{
    // we got some!
    RealtimeData::DataSeries series = static_cast<RealtimeData::DataSeries>
                  (seriesSelector->itemData(seriesSelector->currentIndex()).toInt());

    double value = rtData.value(series);

    switch (series) {

    case RealtimeData::Time:
    case RealtimeData::LapTime:
        {
        long msecs = value;
        valueLabel->setText(QString("%1:%2:%3.%4").arg(msecs/3600000)
                                               .arg((msecs%3600000)/60000,2,10,QLatin1Char('0'))
                                               .arg((msecs%60000)/1000,2,10,QLatin1Char('0'))
                                               .arg((msecs%1000)/100));
        }
        break;

    default:
        valueLabel->setText(QString("%1").arg(round(value)));
        break;

    }
}

void DialWindow::resizeEvent(QResizeEvent * )
{

   if (geometry().height()-37 < 0) return;

   QFont font;
   font.setPointSize((geometry().height()-37));
   font.setWeight(QFont::Bold);
   valueLabel->setFont(font);
}

void DialWindow::seriesChanged()
{
    // we got some!
    RealtimeData::DataSeries series = static_cast<RealtimeData::DataSeries>
                  (seriesSelector->itemData(seriesSelector->currentIndex()).toInt());

    // the series selector changed so update the colors
    switch(series) {

    case RealtimeData::Time:
    case RealtimeData::LapTime:
    case RealtimeData::Distance:
    case RealtimeData::Lap:
    case RealtimeData::Load:
    case RealtimeData::None:
            foreground = GColor(CPLOTMARKER);
            break;

    case RealtimeData::XPower:
    case RealtimeData::Joules:
    case RealtimeData::BikeScore:
    case RealtimeData::Watts:
            foreground = GColor(CPOWER);
            break;

    case RealtimeData::Speed:
            foreground = GColor(CSPEED);
            break;

    case RealtimeData::Cadence:
            foreground = GColor(CCADENCE);
            break;

    case RealtimeData::HeartRate:
            foreground = GColor(CHEARTRATE);
            break;
    }

    // ugh. we use style sheets becuase palettes don't work on labels
    background = GColor(CRIDEPLOTBACKGROUND);
    setProperty("color", background);
    QString sh = QString("QLabel { background: %1; color: %2; }")
                 .arg(background.name())
                 .arg(foreground.name());
    valueLabel->setStyleSheet(sh);
}
