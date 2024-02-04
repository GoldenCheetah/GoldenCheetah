/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#include "ScatterWindow.h"
#include "ScatterPlot.h"
#include "GcOverlayWidget.h"
#include "Athlete.h"
#include "Context.h"
#include "Colors.h"
#include "HelpWhatsThis.h"

#include <QtGui>
#include <QString>

void
ScatterWindow::addStandardChannels(QComboBox *box)
{
    box->addItem(tr("Power"), MODEL_POWER);
    box->addItem(tr("Cadence"), MODEL_CADENCE);
    box->addItem(tr("Heartrate"), MODEL_HEARTRATE);
    box->addItem(tr("Speed"), MODEL_SPEED);
    box->addItem(tr("Altitude"), MODEL_ALT);
    box->addItem(tr("Torque"), MODEL_TORQUE);
    box->addItem(tr("AEPF"), MODEL_AEPF);
    box->addItem(tr("CPV"), MODEL_CPV);
    box->addItem(tr("Time"), MODEL_TIME);
    box->addItem(tr("Distance"), MODEL_DISTANCE);
    box->addItem(tr("Headwind"), MODEL_HEADWIND);
    box->addItem(tr("Slope"), MODEL_SLOPE);
    box->addItem(tr("Temperature"), MODEL_TEMP);
    box->addItem(tr("L/R Balance"), MODEL_LRBALANCE);
    box->addItem(tr("L/R Torque Effectiveness"), MODEL_TE);
    box->addItem(tr("L/R Pedal Smoothness"), MODEL_PS);
    box->addItem(tr("Running Vertical Oscillation"), MODEL_RV);
    box->addItem(tr("Running Cadence"), MODEL_RCAD);
    box->addItem(tr("Running GCT"), MODEL_RGCT);
    box->addItem(tr("Gear Ratio"), MODEL_GEAR);
    box->addItem(tr("Muscle Oxygen"), MODEL_SMO2);
    box->addItem(tr("Haemoglobin Mass"), MODEL_THB);
    box->addItem(tr("Deoxygenated Hb"), MODEL_HHB);
    box->addItem(tr("Oxygenated Hb"), MODEL_O2HB);
    //box->addItem(tr("Interval"), MODEL_INTERVAL); //supported differently for now
    //box->addItem(tr("Latitude"), MODEL_LAT); //weird values make the plot ugly
    //box->addItem(tr("Longitude"), MODEL_LONG); //weird values make the plot ugly
}

void
ScatterWindow::addrStandardChannels(QxtStringSpinBox *box)
{
    QStringList list;
    list.append(tr("Power"));
    list.append(tr("Cadence"));
    list.append(tr("Heartrate"));
    list.append(tr("Speed"));
    list.append(tr("Altitude"));
    list.append(tr("Torque"));
    list.append(tr("AEPF"));
    list.append(tr("CPV"));
    list.append(tr("Time"));
    list.append(tr("Distance"));
    list.append(tr("Headwind"));
    list.append(tr("Slope"));
    list.append(tr("Temperature"));
    list.append(tr("L/R Balance"));
    list.append(tr("L/R Torque Effectiveness"));
    list.append(tr("L/R Pedal Smoothness"));
    list.append(tr("Running Vertical Oscillation"));
    list.append(tr("Running Cadence"));
    list.append(tr("Running GCT"));
    list.append(tr("Gear Ratio"));
    list.append(tr("Muscle Oxygen"));
    list.append(tr("Haemoglobin Mass"));
    list.append(tr("Deoxygenated Hb"));
    list.append(tr("Oxygenated Hb"));
    box->setStrings(list);
}

ScatterWindow::ScatterWindow(Context *context) :
    GcChartWindow(context), context(context), ride(NULL), stale(false), current(NULL)
{
    //
    // reveal controls widget
    //

    // reveal controls
    QLabel *rxLabel = new QLabel(tr("X-Axis:"), this);
    rxSelector = new QxtStringSpinBox();
    addrStandardChannels(rxSelector);

    QLabel *ryLabel = new QLabel(tr("Y-Axis:"), this);
    rySelector = new QxtStringSpinBox();
    addrStandardChannels(rySelector);

    rIgnore = new QCheckBox(tr("Ignore Zero"));
    rIgnore->setChecked(true);
    rIgnore->hide();
    rFrameInterval = new QCheckBox(tr("Frame intervals"));
    rFrameInterval->setChecked(true);

    // layout reveal controls
    QHBoxLayout *r = new QHBoxLayout;
    r->setContentsMargins(0,0,0,0);
    r->addStretch();
    r->addWidget(rxLabel);
    r->addWidget(rxSelector);
    r->addWidget(ryLabel);
    r->addWidget(rySelector);
    r->addSpacing(5);
    r->addWidget(rIgnore);
    r->addWidget(rFrameInterval);
    r->addStretch();
    setRevealLayout(r);

    QWidget *c = new QWidget;
    HelpWhatsThis *helpConfig = new HelpWhatsThis(c);
    c->setWhatsThis(helpConfig->getWhatsThisText(HelpWhatsThis::ChartRides_2D));
    QFormLayout *cl = new QFormLayout(c);
    setControls(c);

    // the plot widget
    scatterPlot= new ScatterPlot(context);
    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->addWidget(scatterPlot);
    HelpWhatsThis *help = new HelpWhatsThis(scatterPlot);
    scatterPlot->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ChartRides_2D));

    setChartLayout(vlayout);

    // labels
    xLabel = new QLabel(tr("X-Axis:"), this);
    xSelector = new QComboBox;
    addStandardChannels(xSelector);
    xSelector->setCurrentIndex(0); // power
    rxSelector->setValue(0);
    cl->addRow(xLabel, xSelector);

    yLabel = new QLabel(tr("Y-Axis:"), this);
    ySelector = new QComboBox;
    addStandardChannels(ySelector);
    ySelector->setCurrentIndex(2); // heartrate
    rySelector->setValue(2);
    cl->addRow(yLabel, ySelector);

    QLabel *smoothLabel = new QLabel(tr("Smooth"), this);
    smoothLineEdit = new QLineEdit(this);
    smoothLineEdit->setFixedWidth(40);

    smoothSlider = new QSlider(Qt::Horizontal, this);
    smoothSlider->setTickPosition(QSlider::TicksBelow);
    smoothSlider->setTickInterval(10);
    smoothSlider->setMinimum(1);
    smoothSlider->setMaximum(60);
    smoothLineEdit->setValidator(new QIntValidator(smoothSlider->minimum(),
                                                   smoothSlider->maximum(),
                                                   smoothLineEdit));
    smoothSlider->setValue(0);

    QHBoxLayout *smoothLayout = new QHBoxLayout;
    smoothLayout->addWidget(smoothLineEdit);
    smoothLayout->addWidget(smoothSlider);
    cl->addRow(smoothLabel, smoothLayout);

    // selectors
    ignore = new QCheckBox(tr("Ignore Zero"));
    ignore->setChecked(true);
    cl->addRow(ignore);

    grid = new QCheckBox(tr("Show Grid"));
    grid->setChecked(true);
    cl->addRow(grid);

    frame = new QCheckBox(tr("Frame Intervals"));
    frame->setChecked(true);
    cl->addRow(frame);

    compareMode = new QComboBox(this);
    compareMode->addItem(tr("All intervals/activities"));
    compareMode->addItem(tr("First intervals/activities on X-axis"));
    compareMode->addItem(tr("First intervals/activities on Y-axis"));
    cl->addRow(new QLabel(tr("Compare mode")), compareMode);
    compareMode->setCurrentIndex(0);

    trendLine = new QComboBox(this);
    trendLine->addItem(tr("No"));
    trendLine->addItem(tr("Auto"));
    trendLine->addItem(tr("Linear"));
    cl->addRow(new QLabel(tr("Trend line")), trendLine);
    trendLine->setCurrentIndex(0);

    // the model helper -- showing model parameters etc
    QWidget *helper = new QWidget(this);
    helper->setAutoFillBackground(true);

    addHelper(QString(tr("Trend")), helper);
    helperWidget()->hide();

    // now connect up the widgets
    //connect(main, SIGNAL(rideSelected()), this, SLOT(rideSelected()));
    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(context, SIGNAL(rideChanged(RideItem*)), this, SLOT(forceReplot()));
    connect(context, SIGNAL(intervalSelected()), this, SLOT(intervalSelected()));
    connect(context, SIGNAL(intervalsChanged()), this, SLOT(intervalSelected()));
    connect(xSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(xSelectorChanged(int)));
    connect(ySelector, SIGNAL(currentIndexChanged(int)), this, SLOT(ySelectorChanged(int)));
    connect(rxSelector, SIGNAL(valueChanged(int)), this, SLOT(rxSelectorChanged(int)));
    connect(rySelector, SIGNAL(valueChanged(int)), this, SLOT(rySelectorChanged(int)));
    connect(grid, SIGNAL(stateChanged(int)), this, SLOT(setGrid()));
    //connect(legend, SIGNAL(stateChanged(int)), this, SLOT(setLegend()));
    connect(frame, SIGNAL(stateChanged(int)), this, SLOT(setFrame()));
    connect(ignore, SIGNAL(stateChanged(int)), this, SLOT(setIgnore()));
    connect(rFrameInterval, SIGNAL(stateChanged(int)), this, SLOT(setrFrame()));
    connect(rIgnore, SIGNAL(stateChanged(int)), this, SLOT(setrIgnore()));
    connect(compareMode, SIGNAL(currentIndexChanged(int)), this, SLOT(setCompareMode(int)));
    connect(trendLine, SIGNAL(currentIndexChanged(int)), this, SLOT(setTrendLine(int)));
    connect(smoothSlider, SIGNAL(valueChanged(int)), this, SLOT(setSmoothingFromSlider()));
    connect(smoothLineEdit, SIGNAL(editingFinished()), this, SLOT(setSmoothingFromLineEdit()));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    // comparing things
    connect(context, SIGNAL(compareIntervalsStateChanged(bool)), this, SLOT(compareChanged()));
    connect(context, SIGNAL(compareIntervalsChanged()), this, SLOT(compareChanged()));

    // set colors
    configChanged(CONFIG_APPEARANCE);
}

void
ScatterWindow::configChanged(qint32)
{
    setProperty("color", GColor(GCol::PLOTBACKGROUND));

    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(GColor(GCol::PLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GColor(GCol::PLOTMARKER));
    palette.setColor(QPalette::Text, GColor(GCol::PLOTMARKER));
    setPalette(palette);
}

void
ScatterWindow::forceReplot()
{
    stale=true;
    rideSelected();
}

void
ScatterWindow::rideSelected()
{
    if (!amVisible()) return;

    ride = myRideItem;

    if (ride == current && !stale) return;

    if (!ride || !ride->ride() || !ride->ride()->dataPoints().count()) {
        current = NULL;
        setIsBlank(true);
        return;
    } else
        setIsBlank(false);

    current = ride;
    stale = false;

    setData();
}

void
ScatterWindow::setGrid()
{
    settings.gridlines = grid->isChecked();
    setData();
}

void
ScatterWindow::setFrame()
{
    settings.frame = frame->isChecked();
    rFrameInterval->setChecked(frame->isChecked());
    setData();
}

void
ScatterWindow::setrFrame()
{
    settings.frame = rFrameInterval->isChecked();
    frame->setChecked(rFrameInterval->isChecked());
    setData();
}

void
ScatterWindow::setIgnore()
{
    settings.ignore = ignore->isChecked();
    rIgnore->setChecked(ignore->isChecked());
    setData();
}

void
ScatterWindow::setrIgnore()
{
    settings.ignore = rIgnore->isChecked();
    ignore->setChecked(rIgnore->isChecked());
    setData();
}

void
ScatterWindow::setCompareMode(int value)
{
    compareMode->setCurrentIndex(value);
    settings.compareMode = value;
    setData();
}

void
ScatterWindow::setTrendLine(int value)
{
    trendLine->setCurrentIndex(value);
    settings.trendLine = value;

    // need a helper any more ?
    if (value > 0) {
        helperWidget()->hide(); //show();
    } else {
        helperWidget()->hide();
    }

    setData();
}

void
ScatterWindow::setSmoothingFromSlider()
{
    smoothLineEdit->setText(QString("%1").arg(smoothSlider->value()));

    settings.smoothing = smoothSlider->value();
    setData();
}

void
ScatterWindow::setSmoothingFromLineEdit()
{
    int value = smoothLineEdit->text().toInt();

    smoothSlider->setValue(value);
    settings.smoothing = value;
    setData();
}

void
ScatterWindow::intervalSelected()
{
    if (!amVisible())
        return;
    setData();
}

void
ScatterWindow::xSelectorChanged(int value)
{
    rxSelector->setValue(value);
    setData();
}

void
ScatterWindow::rxSelectorChanged(int value)
{
    xSelector->setCurrentIndex(value);
    setData();
}

void
ScatterWindow::ySelectorChanged(int value)
{
    rySelector->setValue(value);
    setData();
}

void
ScatterWindow::rySelectorChanged(int value)
{
    ySelector->setCurrentIndex(value);
    setData();
}

void
ScatterWindow::setData()
{
    settings.ride = ride;
    settings.x = xSelector->itemData(xSelector->currentIndex()).toInt();
    settings.y = ySelector->itemData(ySelector->currentIndex()).toInt();
    settings.ignore = ignore->isChecked();
    settings.gridlines = grid->isChecked();
    settings.frame = frame->isChecked();
    settings.compareMode = compareMode->currentIndex();
    settings.trendLine = trendLine->currentIndex();
    settings.smoothing = smoothSlider->value();

    /* Not a blank state ? Just no data and we can change series ?
    if ((setting.x == MODEL_POWER || setting.y == MODEL_POWER ) && !ride->ride()->isDataPresent(RideFile::watts))
        setIsBlank(true);
    else if ((setting.x == MODEL_CADENCE || setting.y == MODEL_CADENCE ) && !ride->ride()->isDataPresent(RideFile::cad))
        setIsBlank(true);
    else if ((setting.x == MODEL_HEARTRATE || setting.y == MODEL_HEARTRATE ) && !ride->ride()->isDataPresent(RideFile::hr))
        setIsBlank(true);
    else if ((setting.x == MODEL_SPEED || setting.y == MODEL_SPEED ) && !ride->ride()->isDataPresent(RideFile::kph))
        setIsBlank(true);
    else
        setIsBlank(false);
    */

    // any intervals to plot?
    if (ride) settings.intervals = ride->intervalsSelected();
    else settings.intervals.clear();

    scatterPlot->setData(&settings);
}

void
ScatterWindow::compareChanged()
{
    /* no stale mode ?
    if (!amVisible()) {
        return;
    }*/

    setData();
    repaint();

}

bool
ScatterWindow::event(QEvent *event)
{
    // nasty nasty nasty hack to move widgets as soon as the widget geometry
    // is set properly by the layout system, by default the width is 100 and
    // we wait for it to be set properly then put our helper widget on the RHS
    if (event->type() == QEvent::Resize && geometry().width() != 100) {

        // put somewhere nice on first show
        if (firstShow) {
            firstShow = false;
            helperWidget()->move(mainWidget()->geometry().width()-275, 50);
        }

        // if off the screen move on screen
        if (helperWidget()->geometry().x() > geometry().width()) {
            helperWidget()->move(mainWidget()->geometry().width()-275, 50);
        }
    }
    return QWidget::event(event);
}


