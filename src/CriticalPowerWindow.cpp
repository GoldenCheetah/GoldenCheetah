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

CriticalPowerWindow::CriticalPowerWindow(const QDir &home, MainWindow *parent) :
    QWidget(parent), home(home), mainWindow(parent)
{
    QVBoxLayout *vlayout = new QVBoxLayout;
    QHBoxLayout *cpintPickerLayout = new QHBoxLayout;
    QLabel *cpintTimeLabel = new QLabel(tr("Interval Duration:"), this);
    cpintTimeValue = new QLineEdit("0 s");
    QLabel *cpintTodayLabel = new QLabel(tr("Today:"), this);
    cpintTodayValue = new QLineEdit(tr("no data"));
    QLabel *cpintAllLabel = new QLabel(tr("All Rides:"), this);
    cpintAllValue = new QLineEdit(tr("no data"));
    cpintTimeValue->setReadOnly(true);
    cpintTodayValue->setReadOnly(true);
    cpintAllValue->setReadOnly(true);

    cpintSetCPButton = new QPushButton(tr("&Save CP value"), this);
    cpintSetCPButton->setEnabled(false);

    cpintPickerLayout->addWidget(cpintTimeLabel);
    cpintPickerLayout->addWidget(cpintTimeValue);
    cpintPickerLayout->addWidget(cpintTodayLabel);
    cpintPickerLayout->addWidget(cpintTodayValue);
    cpintPickerLayout->addWidget(cpintAllLabel);
    cpintPickerLayout->addWidget(cpintAllValue);
    cpintPickerLayout->addWidget(cpintSetCPButton);
    cpintPlot = new CpintPlot(home.path());
    vlayout->addWidget(cpintPlot);
    vlayout->addLayout(cpintPickerLayout);
    setLayout(vlayout);

    picker = new QwtPlotPicker(QwtPlot::xBottom, QwtPlot::yLeft,
                               QwtPicker::PointSelection,
                               QwtPicker::VLineRubberBand,
                               QwtPicker::AlwaysOff, cpintPlot->canvas());
    picker->setRubberBandPen(QColor(Qt::blue));

    connect(picker, SIGNAL(moved(const QPoint &)),
            SLOT(pickerMoved(const QPoint &)));
    connect(cpintSetCPButton, SIGNAL(clicked()),
	    this, SLOT(cpintSetCPButtonClicked()));
}

void
CriticalPowerWindow::newRideAdded()
{
    cpintPlot->needToScanRides = true;
}

void
CriticalPowerWindow::deleteCpiFile(QString rideFilename)
{
    cpintPlot->deleteCpiFile(home.absolutePath() + "/" +
                             ride_filename_to_cpi_filename(rideFilename));
}

void
CriticalPowerWindow::setData(RideItem *ride)
{
    cpintPlot->calculate(ride);
    cpintSetCPButton->setEnabled(cpintPlot->cp > 0);
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
CriticalPowerWindow::pickerMoved(const QPoint &pos)
{
    double minutes = cpintPlot->invTransform(QwtPlot::xBottom, pos.x());
    cpintTimeValue->setText(interval_to_str(60.0*minutes));

    // current ride
    {
      unsigned watts = curve_to_point(minutes, cpintPlot->getThisCurve());
      QString label;
      if (watts > 0)
	label = QString("%1 watts").arg(watts);
      else
	label = tr("no data");
      cpintTodayValue->setText(label);
    }

    // global ride
    {
      QString label;
      int index = (int) ceil(minutes * 60);
      if (cpintPlot->getBests().count() > index) {
	  QDate date = cpintPlot->getBestDates()[index];
	  label =
	      QString("%1 watts (%2)").
	      arg(cpintPlot->getBests()[index]).
	      arg(date.isValid() ? date.toString("MM/dd/yyyy") : "no date");
      }
      else
	  label = tr("no data");
      cpintAllValue->setText(label);
    }
}

