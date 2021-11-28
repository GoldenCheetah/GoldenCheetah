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
#include "Athlete.h"
#include "Context.h"
#include "HelpWhatsThis.h"

DialWindow::DialWindow(Context *context) :
    GcChartWindow(context), context(context), average(1), isNewLap(false)
{
    HelpWhatsThis *helpContents = new HelpWhatsThis(this);
    this->setWhatsThis(helpContents->getWhatsThisText(HelpWhatsThis::ChartTrain_Telemetry));

    rolling.resize(150); // enough for 30 seconds at 5hz

    setContentsMargins(0,0,0,0);

    QWidget *c = new QWidget;
    HelpWhatsThis *helpConfig = new HelpWhatsThis(c);
    c->setWhatsThis(helpConfig->getWhatsThisText(HelpWhatsThis::ChartTrain_Telemetry));
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

    // average selection
    averageLabel= new QLabel(tr("Smooth (secs)"), this);
    averageLabel->hide();
    averageLabel->setAutoFillBackground(true);

    QHBoxLayout *averageLayout = new QHBoxLayout;
    averageSlider = new QSlider(Qt::Horizontal);
    averageSlider->hide();
    averageSlider->setTickPosition(QSlider::TicksBelow);
    averageSlider->setTickInterval(5);
    averageSlider->setMinimum(1);
    averageSlider->setMaximum(30);
    averageSlider->setValue(1);
    averageLayout->addWidget(averageSlider);
    averageEdit = new QLineEdit();
    averageEdit->hide();
    averageEdit->setFixedWidth(20);
    averageEdit->setText(QString("%1").arg(1) );
    averageLayout->addWidget(averageEdit);

    controlsLayout->addRow(averageLabel, averageLayout);

    // display label...
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setContentsMargins(3,3,3,3);
    valueLabel = new QLabel(this);
    valueLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    layout->addWidget(valueLabel);
    setChartLayout(layout);

    // get updates..
    connect(context, SIGNAL(telemetryUpdate(RealtimeData)), this, SLOT(telemetryUpdate(RealtimeData)));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(seriesChanged()));
    connect(context, SIGNAL(stop()), this, SLOT(stop()));
    connect(context, SIGNAL(start()), this, SLOT(start()));
    connect(context, SIGNAL(newLap()), this, SLOT(onNewLap()));

    connect(seriesSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(seriesChanged()));
    connect(averageSlider, SIGNAL(valueChanged(int)),this, SLOT(setAverageFromSlider()));
    connect(averageEdit, SIGNAL(textChanged(const QString)), this, SLOT(setAverageFromText(const QString)));

    // setup colors
    seriesChanged();

    // setup fontsize etc
    resizeEvent(NULL);

    // set to zero
    resetValues();
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

    // Average value for display for HeartRate, Watts and Cadence
    double displayValue = value;

    if (series == RealtimeData::HeartRate ||
        series == RealtimeData::Watts  ||
        series == RealtimeData::AltWatts  ||
        series == RealtimeData::Cadence) {

        sum += value;
        int j = index-(count<average*5?count:average*5);
        sum -= rolling[(j>=0?j:150+j)];

        //store value
        rolling[index] = value;

        // move index on/round for next value
        index = (index >= 149) ? 0 : index+1;
        count++;

        if (average > 1) {
            // rolling average
            if (count < average*5)
                displayValue = sum/(count);
            else
                displayValue = sum/(average*5);
        }

        // if we have a target load and erg mode then red background if not on target...
        if (series == RealtimeData::Watts && (rtData.mode == ERG || rtData.mode == MRC) && rtData.getLoad() > 0) {

            // background for power, if we have a target load
            double load=rtData.getLoad();
            QColor color = GColor(CTRAINPLOTBACKGROUND);
            QColor foreground = GColor(CPOWER);

            if (displayValue < (load*0.95f) || displayValue > (load * 1.05)) {
                color = displayValue < load ? QColor(Qt::blue) : QColor(Qt::red);
                foreground = QColor(Qt::white);
            }

            // set color
            setProperty("color", background);
            QString sh = QString("QLabel { background: %1; color: %2; }")
                                .arg(color.name())
                                .arg(foreground.name());
            valueLabel->setStyleSheet(sh);

        } else if (series == RealtimeData::Watts && rtData.getLoad() <= 0) {

            // reset if load is zero, we've ended the workout
            seriesChanged();
        }
    }

    if( isNewLap &&
        ( series == RealtimeData::AvgCadenceLap ||
        series == RealtimeData::AvgHeartRateLap ||
        series == RealtimeData::AvgSpeedLap ||
        series == RealtimeData::AvgWattsLap ) )
    {
        count = 0;
        sum = 0.0;
        isNewLap = false;
    }

    switch (series) {

    case RealtimeData::Time:
    case RealtimeData::LapTime:
        {
        long msecs = value;
        valueLabel->setText(QString("%1:%2:%3.%4").arg(msecs/MS_IN_ONE_HOUR)
                                               .arg((msecs%MS_IN_ONE_HOUR)/60000,2,10,QLatin1Char('0'))
                                               .arg((msecs%60000)/1000,2,10,QLatin1Char('0'))
                                               .arg((msecs%1000)/100));
        }
        break;

    case RealtimeData::LapTimeRemaining:
    case RealtimeData::ErgTimeRemaining:
        {
        long msecs = value;
        valueLabel->setText(QString("%1:%2:%3").arg(msecs/MS_IN_ONE_HOUR)
                                               .arg((msecs%MS_IN_ONE_HOUR)/60000,2,10,QLatin1Char('0'))
                                               .arg((msecs%60000)/1000,2,10,QLatin1Char('0')));
        }
    break;

    case RealtimeData::LRBalance:
        {
          double left = 0; double right = 0;
          if (value == 0) { // no LR Balance provided - so use previous logic
              double tot = rtData.getWatts() + rtData.getAltWatts();
              left = rtData.getWatts() / tot * 100.00f;
              right = 100.00 - left;
              if (tot < 0.1) left = right = 0;
          } else {
              left = 100.00 - value; // value in ANT message is the "right" pedal portion
              right = value;
          }
          valueLabel->setText(QString("%1 / %2").arg(left, 0, 'f', 0).arg(right, 0, 'f', 0));
        }
        break;

    case RealtimeData::Speed:
    case RealtimeData::VirtualSpeed:
        if (!GlobalContext::context()->useMetricUnits) value *= MILES_PER_KM;
        valueLabel->setText(QString("%1").arg(value, 0, 'f', 1));
        break;

    // If the distance remaining is negative, it is either irrelevant, or we've passed the last lap marker
    case RealtimeData::LapDistanceRemaining:
        if (value < 0) {
            valueLabel->setText("N/A");
            break;
        }
        // intentional fall-through
        // to standard distance rendering

    case RealtimeData::Distance:
    case RealtimeData::LapDistance:
    case RealtimeData::RouteDistance:
    case RealtimeData::DistanceRemaining:
        if (!GlobalContext::context()->useMetricUnits) value *= MILES_PER_KM;
        // Default floating point rounds to nearest. For distance we'd like to not see the
        // next biggest number until we're actually there... Truncate to meter.
        value = ((double)((unsigned)(value * 1000.))) / 1000.;
        valueLabel->setText(QString("%1").arg(value, 0, 'f', 3));
        break;

    case RealtimeData::AvgWatts:
    case RealtimeData::AvgWattsLap:
        sum += rtData.value(RealtimeData::Watts);
        count++;
        value = sum / count;
        valueLabel->setText(QString("%1").arg(round(value)));
        break;

    case RealtimeData::AvgSpeed:
    case RealtimeData::AvgSpeedLap:
        sum += rtData.value(RealtimeData::Speed);
        count++;
        value = sum / count;
        if (!GlobalContext::context()->useMetricUnits) value *= MILES_PER_KM;
        valueLabel->setText(QString("%1").arg(value, 0, 'f', 1));
        break;

    case RealtimeData::AvgCadence:
    case RealtimeData::AvgCadenceLap:
        sum += rtData.value(RealtimeData::Cadence);
        count++;
        value = sum / count;
        valueLabel->setText(QString("%1").arg(round(value)));
        break;

    case RealtimeData::AvgHeartRate:
    case RealtimeData::AvgHeartRateLap:
        sum += rtData.value(RealtimeData::HeartRate);
        count++;
        value = sum / count;
        valueLabel->setText(QString("%1").arg(round(value)));
        break;

    // ENERGY
    case RealtimeData::Joules:
        sum += rtData.value(RealtimeData::Watts) / 5; // joules
        valueLabel->setText(QString("%1").arg(round(sum/1000))); // kJoules
        break;

    case RealtimeData::Wbal:
        valueLabel->setText(QString("%1").arg(rtData.getWbal()/1000.00f, 0, 'f', 1)); // kJoules
        break;

    // COGGAN Metrics
    case RealtimeData::IsoPower:
    case RealtimeData::IF:
    case RealtimeData::BikeStress:
    case RealtimeData::VI:
        {

        // Update sum of watts for last 30 seconds
        sum += rtData.value(RealtimeData::Watts);
        sum -= rolling[index];
        rolling[index] = rtData.value(RealtimeData::Watts);

        // raise average to the 4th power
        rollingSum += pow(sum/150,4); // raise rolling average to 4th power
        count ++;

        // move index on/round
        index = (index >= 149) ? 0 : index+1;

        // calculate IsoPower
        double np = pow(rollingSum / (count), 0.25);

        if (series == RealtimeData::IsoPower) {
            // We only wanted IsoPower so thats it
            valueLabel->setText(QString("%1").arg(round(np)));

        } else {

            double rif, cp;
            // carry on and calculate IF
            if (context->athlete->zones("Bike")) {

                // get cp for today
                int zonerange = context->athlete->zones("Bike")->whichRange(QDateTime::currentDateTime().date());
                if (zonerange >= 0) cp = context->athlete->zones("Bike")->getCP(zonerange);
                else cp = 0;

            } else {
                cp = 0;
            }

            if (cp) rif = np / cp;
            else rif = 0;

            if (series == RealtimeData::IF) {

                // we wanted IF so thats it
                valueLabel->setText(QString("%1").arg(rif, 0, 'f', 3));

            } else {

                double normWork = np * (rtData.value(RealtimeData::Time) / 1000); // msecs
                double rawTSS = normWork * rif;
                double workInAnHourAtCP = cp * 3600;
                double tss = rawTSS / workInAnHourAtCP * 100.0;

                if (series == RealtimeData::BikeStress) {

                    valueLabel->setText(QString("%1").arg(tss, 0, 'f', 1));

                } else {

                    // track average power for VI
                    apsum += rtData.value(RealtimeData::Watts);
                    apcount++;

                    double ap = apsum ? apsum / apcount : 0;

                    // VI is all that is left!
                    valueLabel->setText(QString("%1").arg(ap ? np / ap : 0, 0, 'f', 3));

                }

            }

        }

        }
        break;

    // SKIBA Metrics
    case RealtimeData::XPower:
    case RealtimeData::RI:
    case RealtimeData::BikeScore:
    case RealtimeData::SkibaVI:
        {

        static const double exp = 2.0f / ((25.0f / 0.2f) + 1.0f);
        static const double rem = 1.0f - exp;

        count++;

        if (count < 125) {

            // get up to speed
            rsum += rtData.value(RealtimeData::Watts);
            ewma = rsum / count;

        } else {

            // we're up to speed
            ewma = (rtData.value(RealtimeData::Watts) * exp) + (ewma * rem);
        }

        sum += pow(ewma, 4.0f);
        double xpower = pow(sum / count, 0.25f);

        if (series == RealtimeData::XPower) {

            // We wanted XPower!
            valueLabel->setText(QString("%1").arg(round(xpower)));

        } else {

            double rif, cp;
            // carry on and calculate IF
            if (context->athlete->zones("Bike")) {

                // get cp for today
                int zonerange = context->athlete->zones("Bike")->whichRange(QDateTime::currentDateTime().date());
                if (zonerange >= 0) cp = context->athlete->zones("Bike")->getCP(zonerange);
                else cp = 0;

            } else {
                cp = 0;
            }

            if (cp) rif = xpower / cp;
            else rif = 0;

            if (series == RealtimeData::RI) {

                // we wanted IF so thats it
                valueLabel->setText(QString("%1").arg(rif, 0, 'f', 3));

            } else {

                double normWork = xpower * (rtData.value(RealtimeData::Time) / 1000); // msecs
                double rawTSS = normWork * rif;
                double workInAnHourAtCP = cp * 3600;
                double tss = rawTSS / workInAnHourAtCP * 100.0;

                if (series == RealtimeData::BikeScore) {

                    valueLabel->setText(QString("%1").arg(tss, 0, 'f', 1));

                } else {

                    // track average power for Relative Intensity
                    apsum += rtData.value(RealtimeData::Watts);
                    apcount++;

                    double ap = apsum ? apsum / apcount : 0;

                    // RI is all that is left!
                    valueLabel->setText(QString("%1").arg(ap ? xpower / ap : 0, 0, 'f', 3));

                }

            }

        }

        }
        break;

    case RealtimeData::Load:
        if (rtData.mode == ERG || rtData.mode == MRC) {
            value = rtData.getLoad();
            valueLabel->setText(QString("%1").arg(round(value)));
        } else {
            value = rtData.getSlope();
            valueLabel->setText(QString("%1%").arg(value, 0, 'f', 1));
        }
        break;

    case RealtimeData::SmO2:
        valueLabel->setText(QString("%1%").arg(value, 0, 'f', 0));
        break;
    case RealtimeData::tHb:
    case RealtimeData::O2Hb:
    case RealtimeData::HHb:
        valueLabel->setText(QString("%1").arg(value, 0, 'f', 1));
        break;


    case RealtimeData::LeftPedalSmoothness:
    case RealtimeData::RightPedalSmoothness:
    case RealtimeData::LeftTorqueEffectiveness:
    case RealtimeData::RightTorqueEffectiveness:
        {
          valueLabel->setText(QString("%1").arg(value, 0, 'f', 0));
        }
        break;

    case RealtimeData::Slope:
        valueLabel->setText(QString("%1").arg(value, 0, 'f', 1));
        break;

    case RealtimeData::Latitude:
    case RealtimeData::Longitude:
        valueLabel->setText(QString("%1").arg(value, 0, 'f', 8));
        break;

    case RealtimeData::Altitude:
        valueLabel->setText(QString("%1").arg(value, 0, 'f', 1));
        break;

    case RealtimeData::VO2:
    case RealtimeData::VCO2:
    case RealtimeData::Rf:
    case RealtimeData::RMV:
        valueLabel->setText(QString("%1").arg(value, 0, 'f', 0));
        break;

    case RealtimeData::TidalVolume:
        valueLabel->setText(QString("%1").arg(value, 0, 'f', 1));
        break;

    case RealtimeData::FeO2:
    case RealtimeData::RER:
        valueLabel->setText(QString("%1").arg(value, 0, 'f', 2));
        break;

    default:
        valueLabel->setText(QString("%1").arg(round(displayValue)));
        break;

    }
}

void DialWindow::resizeEvent(QResizeEvent * )
{
    QFont font;

    // set point size within reasonable limits for low dpi screens
    int size = (geometry().height() - 24) * 72 / logicalDpiY();
    if (size <= 0) size = 4;
    if (size >= 64) size = 64;

    font.setPointSize(size);

    font.setWeight(QFont::Bold);
    valueLabel->setFont(font);
}

void DialWindow::seriesChanged()
{
    // we got some!
    RealtimeData::DataSeries series = static_cast<RealtimeData::DataSeries>
                  (seriesSelector->itemData(seriesSelector->currentIndex()).toInt());

    if (series == RealtimeData::HeartRate ||
        series == RealtimeData::Watts  ||
        series == RealtimeData::AltWatts  ||
        series == RealtimeData::Cadence) {
        averageLabel->show();
        averageEdit->show();
        averageSlider->show();
    } else {
        averageLabel->hide();
        averageEdit->hide();
        averageSlider->hide();
    }

    // the series selector changed so update the colors
    switch(series) {

    case RealtimeData::Time:
    case RealtimeData::LapTime:
    case RealtimeData::LapTimeRemaining:
    case RealtimeData::ErgTimeRemaining:
    case RealtimeData::LRBalance:
    case RealtimeData::Lap:
    case RealtimeData::RI:
    case RealtimeData::IF:
    case RealtimeData::VI:
    case RealtimeData::SkibaVI:
    case RealtimeData::FeO2:
        foreground = GColor(CFEO2);
        break;
            
    case RealtimeData::Distance:
    case RealtimeData::RouteDistance:
    case RealtimeData::DistanceRemaining:
    case RealtimeData::Latitude:
    case RealtimeData::Longitude:
    case RealtimeData::LapDistance:
    case RealtimeData::LapDistanceRemaining:
    case RealtimeData::RER:
    case RealtimeData::None:
            foreground = GColor(CDIAL);
            break;

    case RealtimeData::Rf:
        foreground = GColor(CRESPFREQUENCY);
        break;

    case RealtimeData::RMV:
        foreground = GColor(CVENTILATION);
        break;

    case RealtimeData::VO2:
        foreground = GColor(CVO2);
        break;

    case RealtimeData::VCO2:
        foreground = GColor(CVCO2);
        break;

    case RealtimeData::TidalVolume:
        foreground = GColor(CTIDALVOLUME);
        break;

    case RealtimeData::Slope:  
        foreground = GColor(CSLOPE);
        break;
            
    case RealtimeData::Load:
        foreground = GColor(CLOAD);
        break;

    case RealtimeData::BikeScore:
            foreground = GColor(CBIKESCORE);
            break;

    case RealtimeData::BikeStress:
            foreground = GColor(CTSS);
            break;

    case RealtimeData::XPower:
    case RealtimeData::IsoPower:
    case RealtimeData::Joules:
    case RealtimeData::Watts:
    case RealtimeData::AvgWatts:
    case RealtimeData::AvgWattsLap:
            foreground = GColor(CPOWER);
            break;

    case RealtimeData::Speed:
    case RealtimeData::VirtualSpeed:
    case RealtimeData::AvgSpeed:
    case RealtimeData::AvgSpeedLap:
            foreground = GColor(CSPEED);
            break;

    case RealtimeData::Wbal:
            foreground = GColor(CWBAL);
            break;

    case RealtimeData::Cadence:
    case RealtimeData::AvgCadence:
    case RealtimeData::AvgCadenceLap:
            foreground = GColor(CCADENCE);
            break;

    case RealtimeData::HeartRate:
    case RealtimeData::AvgHeartRate:
    case RealtimeData::AvgHeartRateLap:
            foreground = GColor(CHEARTRATE);
            break;

    case RealtimeData::AltWatts:
            foreground = GColor(CALTPOWER);
            break;

    case RealtimeData::SmO2:
            foreground = GColor(CSMO2);
            break;

    case RealtimeData::tHb:
            foreground = GColor(CTHB);
            break;

    case RealtimeData::O2Hb:
            foreground = GColor(CO2HB);
            break;

    case RealtimeData::HHb:
            foreground = GColor(CHHB);
            break;

    case RealtimeData::LeftPedalSmoothness:
            foreground = GColor(CLPS);
            break;

    case RealtimeData::RightPedalSmoothness:
            foreground = GColor(CRPS);
            break;

    case RealtimeData::LeftTorqueEffectiveness:
            foreground = GColor(CLTE);
            break;

    case RealtimeData::RightTorqueEffectiveness:
           foreground = GColor(CRTE);
           break;

    case RealtimeData::Altitude:
           foreground = GColor(CALTITUDE);
           break;
    }

    // ugh. we use style sheets because palettes don't work on labels
    background = GColor(CTRAINPLOTBACKGROUND);
    setProperty("color", background);
    QString sh = QString("QLabel { background: %1; color: %2; }")
                 .arg(background.name())
                 .arg(foreground.name());
    valueLabel->setStyleSheet(sh);
    repaint();
}

void
DialWindow::setAverageFromText(const QString text) {
    int value = text.toDouble();
    averageEdit->setText(text);
    if (average != value) {
        average = value;
        averageSlider->setValue(average);

        // correction of sum
        sum = 0;
        int j = index-(count<average*5?count:average*5);
        for (int i=0; i<average*5; i++)  {
            sum += rolling[(j>=0?j:150+j)];
            j++;
        }
    }
}

void
DialWindow::setAverageFromSlider() {
    setAverageFromText(QString("%1").arg(averageSlider->value()));
}

void
DialWindow::onNewLap()
{
    isNewLap = true;
}
