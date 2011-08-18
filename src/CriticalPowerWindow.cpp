/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net)
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

#include "CriticalPowerWindow.h"
#include "CpintPlot.h"
#include "MainWindow.h"
#include "RideItem.h"
#include "TimeUtils.h"
#include <qwt_picker.h>
#include <qwt_plot_picker.h>
#include <QFile>
#include "Season.h"
#include "SeasonParser.h"
#include "Colors.h"
#include <QXmlInputSource>
#include <QXmlSimpleReader>

CriticalPowerWindow::CriticalPowerWindow(const QDir &home, MainWindow *parent) :
    GcWindow(parent), home(home), mainWindow(parent), currentRide(NULL)
{
    setInstanceName("Critical Power Window");

    // main plot area
    QVBoxLayout *vlayout = new QVBoxLayout;
    cpintPlot = new CpintPlot(mainWindow, home.path(), mainWindow->zones());
    vlayout->addWidget(cpintPlot);
    setLayout(vlayout);

    // controls
    QWidget *c = new QWidget;
    QVBoxLayout *cl = new QVBoxLayout(c);
    setControls(c);

    // picker details
    QFormLayout *cpintPickerLayout = new QFormLayout;
    QLabel *cpintTimeLabel = new QLabel(tr("Interval Duration:"), this);
    cpintTimeValue = new QLineEdit("0 s");
    QLabel *cpintTodayLabel = new QLabel(tr("Today:"), this);
    cpintTodayValue = new QLineEdit(tr("no data"));
    QLabel *cpintAllLabel = new QLabel(tr("Best:"), this);
    cpintAllValue = new QLineEdit(tr("no data"));
    QLabel *cpintCPLabel = new QLabel(tr("CP Curve:"), this);
    cpintCPValue = new QLineEdit(tr("no data"));

    QFontMetrics metrics(QApplication::font());
    //int width = metrics.width("8888 watts (88/88/8888)") + 10;
    //cpintAllValue->setFixedWidth(width);
    //cpintCPValue->setFixedWidth(width); // so lines up nicely

    cpintTimeValue->setReadOnly(false);
    cpintTodayValue->setReadOnly(true);
    cpintAllValue->setReadOnly(true);
    cpintCPValue->setReadOnly(true);
    cpintPickerLayout->addRow(cpintTimeLabel, cpintTimeValue);
    cpintPickerLayout->addRow(cpintTodayLabel, cpintTodayValue);
    cpintPickerLayout->addRow(cpintAllLabel, cpintAllValue);
    cpintPickerLayout->addRow(cpintCPLabel, cpintCPValue);
    cl->addLayout(cpintPickerLayout);

    // tools /properties
    seriesCombo = new QComboBox(this);
    addSeries();
    cComboSeason = new QComboBox(this);
    addSeasons();
    cpintSetCPButton = new QPushButton(tr("&Save CP value"), this);
    cpintSetCPButton->setEnabled(false);
    cl->addWidget(cpintSetCPButton);
    cl->addWidget(cComboSeason);
    cl->addWidget(seriesCombo);
    cl->addStretch();

    picker = new QwtPlotPicker(QwtPlot::xBottom, QwtPlot::yLeft,
                               QwtPicker::PointSelection,
                               QwtPicker::VLineRubberBand,
                               QwtPicker::AlwaysOff, cpintPlot->canvas());
    picker->setRubberBandPen(GColor(CPLOTTRACKER));

    connect(picker, SIGNAL(moved(const QPoint &)), SLOT(pickerMoved(const QPoint &)));
    connect(cpintTimeValue, SIGNAL(editingFinished()), this, SLOT(cpintTimeValueEntered()));
    connect(cpintSetCPButton, SIGNAL(clicked()), this, SLOT(cpintSetCPButtonClicked()));
    connect(cComboSeason, SIGNAL(currentIndexChanged(int)), this, SLOT(seasonSelected(int)));
    connect(seriesCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(setSeries(int)));
    //connect(mainWindow, SIGNAL(rideSelected()), this, SLOT(rideSelected()));
    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(mainWindow, SIGNAL(configChanged()), cpintPlot, SLOT(configChanged()));

    // redraw on config change -- this seems the simplest approach
    connect(mainWindow, SIGNAL(configChanged()), this, SLOT(rideSelected()));
    connect(mainWindow, SIGNAL(rideAdded(RideItem*)), this, SLOT(newRideAdded(RideItem*)));
    connect(mainWindow, SIGNAL(rideDeleted(RideItem*)), this, SLOT(newRideAdded(RideItem*)));
}

void
CriticalPowerWindow::newRideAdded(RideItem *here)
{
    // mine just got Zapped, a new rideitem would not be my current item
    if (here == currentRide) currentRide = NULL;

    Season season = seasons.at(cComboSeason->currentIndex());

    // Refresh global curve if a ride is added during those dates
    if ((here->dateTime.date() >= season.getStart() || season.getStart() == QDate())
        && (here->dateTime.date() <= season.getEnd() || season.getEnd() == QDate()))
        cpintPlot->changeSeason(season.getStart(), season.getEnd());

    // if visible make the changes visible
    // rideSelected is easiest way
    if (amVisible()) rideSelected();
}

void
CriticalPowerWindow::rideSelected()
{
    if (!amVisible())
        return;
    currentRide = myRideItem;
    if (currentRide) {
        cpintPlot->calculate(currentRide);

        // apply latest colors
        picker->setRubberBandPen(GColor(CPLOTTRACKER));
        cpintSetCPButton->setEnabled(cpintPlot->cp > 0);
    }
}

void
CriticalPowerWindow::setSeries(int index)
{
    if (index >= 0) {
        cpintPlot->setSeries(static_cast<RideFile::SeriesType>(seriesCombo->itemData(index).toInt()));
        cpintPlot->calculate(currentRide);
    }
}

void
CriticalPowerWindow::cpintSetCPButtonClicked()
{
    int cp = (int) cpintPlot->cp;
    if (cp <= 0) {
        QMessageBox::critical(
            this,
            tr("Set CP value to extracted value"),
            tr("No non-zero extracted value was identified:\n") +
            tr("Zones were unchanged."));
        return;
    }
    mainWindow->setCriticalPower(cp);
}

static unsigned
curve_to_point(double x, const QwtPlotCurve *curve)
{
    unsigned result = 0;
    if (curve) {
        const QwtData &data = curve->data();
        if (data.size() > 0) {
            if (x < data.x(0) || x > data.x(data.size() - 1))
                return 0;
            unsigned min = 0, mid = 0, max = data.size();
            while (min < max - 1) {
                mid = (max - min) / 2 + min;
                if (x < data.x(mid)) {
                    result = (unsigned) round(data.y(mid));
                    max = mid;
                }
                else {
                    min = mid;
                }
            }
        }
    }
    return result;
}

void
CriticalPowerWindow::updateCpint(double minutes)
{
    QString units;

    switch (series()) {

        case RideFile::none:
            units = "kJ";
            break;

        case RideFile::cad:
            units = "rpm";
            break;

        case RideFile::kph:
            units = "kph";
            break;

        case RideFile::hr:
            units = "bpm";
            break;

        case RideFile::nm:
            units = "nm";
            break;

        case RideFile::vam:
            units = "metres/hour";
            break;

        default:
        case RideFile::watts:
            units = "Watts";
            break;

    }

    // current ride
    {
      unsigned value = curve_to_point(minutes, cpintPlot->getThisCurve());
      QString label;
      if (value > 0)
        label = QString("%1 %2").arg(value).arg(units);
      else
	      label = tr("no data");
      cpintTodayValue->setText(label);
    }

    // cp line
    if (cpintPlot->getCPCurve()) {
      unsigned value = curve_to_point(minutes, cpintPlot->getCPCurve());
      QString label;
      if (value > 0)
        label = QString("%1 %2").arg(value).arg(units);
      else
        label = tr("no data");
      cpintCPValue->setText(label);
    }

    // global ride
    {
      QString label;
      int index = (int) ceil(minutes * 60);
      if (index >= 0 && cpintPlot->getBests().count() > index) {
          QDate date = cpintPlot->getBestDates()[index];
          unsigned value = cpintPlot->getBests()[index];
#if 0
              label = QString("%1 kJ (%2)").arg(watts * minutes * 60.0 / 1000.0, 0, 'f', 0);
#endif
              label = QString("%1 %2 (%3)").arg(value).arg(units)
                      .arg(date.isValid() ? date.toString(tr("MM/dd/yyyy")) : tr("no date"));
      }
      else {
        label = tr("no data");
      }
      cpintAllValue->setText(label);
    }
}

void
CriticalPowerWindow::cpintTimeValueEntered()
{
  double minutes = str_to_interval(cpintTimeValue->text()) / 60.0;
  updateCpint(minutes);
}

void
CriticalPowerWindow::pickerMoved(const QPoint &pos)
{
    double minutes = cpintPlot->invTransform(QwtPlot::xBottom, pos.x());
    cpintTimeValue->setText(interval_to_str(60.0*minutes));
    updateCpint(minutes);
}
void CriticalPowerWindow::addSeries()
{
    // setup series list
    seriesList << RideFile::watts
               << RideFile::xPower
               << RideFile::NP
               << RideFile::hr
               << RideFile::kph
               << RideFile::cad
               << RideFile::nm
               << RideFile::vam
               << RideFile::none; // XXX actually this shows energy (hack)

    foreach (RideFile::SeriesType x, seriesList) {
        if (x==RideFile::none)
            seriesCombo->addItem(tr("Energy"), static_cast<int>(x));
        else
            seriesCombo->addItem(RideFile::seriesName(x), static_cast<int>(x));
    }
}

void CriticalPowerWindow::addSeasons()
{
    QFile seasonFile(home.absolutePath() + "/seasons.xml");
    QXmlInputSource source( &seasonFile );
    QXmlSimpleReader xmlReader;
    SeasonParser( handler );
    xmlReader.setContentHandler(&handler);
    xmlReader.setErrorHandler(&handler);
    xmlReader.parse( source );
    seasons = handler.getSeasons();
    Season season;
    season.setName(tr("All Seasons"));
    seasons.insert(0,season);

    foreach (Season season, seasons)
        cComboSeason->addItem(season.getName());
    if (!seasons.empty()) {
        cComboSeason->setCurrentIndex(cComboSeason->count() - 1);
        Season season = seasons.last();
        cpintPlot->changeSeason(season.getStart(), season.getEnd());
    }
}

void CriticalPowerWindow::seasonSelected(int iSeason)
{
    Season season = seasons.at(iSeason);
    cpintPlot->changeSeason(season.getStart(), season.getEnd());
    cpintPlot->calculate(currentRide);
}


