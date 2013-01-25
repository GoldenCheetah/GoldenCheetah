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

#include "PfPvWindow.h"
#include "MainWindow.h"
#include "PfPvPlot.h"
#include "RideItem.h"
#include "Settings.h"
#include "Colors.h"
#include <QtGui>

PfPvWindow::PfPvWindow(MainWindow *mainWindow) :
    GcWindow(mainWindow), mainWindow(mainWindow), current(NULL)
{
    setInstanceName("Pf/Pv Window");

    QWidget *c = new QWidget;
    QVBoxLayout *cl = new QVBoxLayout(c);
    setControls(c);

    // Main layout
    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->setContentsMargins(2,2,2,2);

    //
    // reveal controls widget
    //

    // reveal widget
    revealControls = new QWidget(this);
    revealControls->setFixedHeight(50);
    revealControls->setStyleSheet("background-color: rgba(100%, 100%, 100%, 100%)");
    revealControls->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    revealAnim = new QPropertyAnimation(revealControls, "pos");
    revealAnim->setDuration(500);
    revealAnim->setEasingCurve(QEasingCurve(QEasingCurve::InSine));
    revealAnim->setKeyValueAt(0,QPoint(2,-50));
    revealAnim->setKeyValueAt(0.5,QPoint(2,15));
    revealAnim->setKeyValueAt(1,QPoint(2,20));

    unrevealAnim = new QPropertyAnimation(revealControls, "pos");
    unrevealAnim->setDuration(500);
    unrevealAnim->setKeyValueAt(0,QPoint(2,20));
    unrevealAnim->setKeyValueAt(0.5,QPoint(2,15));
    unrevealAnim->setKeyValueAt(1,QPoint(2,-50));

    // layout reveal controls
    QHBoxLayout *revealLayout = new QHBoxLayout;
    revealLayout->setContentsMargins(0,0,0,0);

    rShade = new QCheckBox(tr("Shade zones"), revealControls);
    if (appsettings->value(this, GC_SHADEZONES, true).toBool() == true)
        rShade->setCheckState(Qt::Checked);
    else
        rShade->setCheckState(Qt::Unchecked);
    rMergeInterval = new QCheckBox;
    rMergeInterval->setText(tr("Merge intervals"));
    rMergeInterval->setCheckState(Qt::Unchecked);
    rMergeInterval->hide(); // lets not - its not that useful
    rFrameInterval = new QCheckBox;
    rFrameInterval->setText(tr("Frame intervals"));
    rFrameInterval->setCheckState(Qt::Checked);

    QVBoxLayout *checks = new QVBoxLayout;
    checks->addStretch();
    checks->addWidget(rShade);
    checks->addWidget(rMergeInterval);
    checks->addWidget(rFrameInterval);
    checks->addStretch();

    revealLayout->addStretch();
    revealLayout->addLayout(checks);
    revealLayout->addStretch();

    revealControls->setLayout(revealLayout);

    // hide them initially
    revealControls->hide();

    // the plot
    QVBoxLayout *vlayout = new QVBoxLayout;
    pfPvPlot = new PfPvPlot(mainWindow);
    vlayout->addWidget(pfPvPlot);

    mainLayout->addLayout(vlayout,0,0);
    mainLayout->addWidget(revealControls,0,0, Qt::AlignTop);
    revealControls->raise();
    setLayout(mainLayout);

    // allow zooming
    pfpvZoomer = new QwtPlotZoomer(pfPvPlot->canvas());
    pfpvZoomer->setRubberBand(QwtPicker::RectRubberBand);
    pfpvZoomer->setRubberBandPen(GColor(CPLOTSELECT));
    pfpvZoomer->setTrackerMode(QwtPicker::AlwaysOff);
    pfpvZoomer->setEnabled(true);
    pfpvZoomer->setMousePattern(QwtEventPattern::MouseSelect1, Qt::LeftButton);
    pfpvZoomer->setMousePattern(QwtEventPattern::MouseSelect2, Qt::RightButton, Qt::ControlModifier);
    pfpvZoomer->setMousePattern(QwtEventPattern::MouseSelect3, Qt::RightButton);

    // the controls
    QFormLayout *f = new QFormLayout;
    cl->addLayout(f);

    QLabel *qaCPLabel = new QLabel(tr("Watts:"), this);
    qaCPValue = new QLineEdit(QString("%1").arg(pfPvPlot->getCP()));
    qaCPValue->setValidator(new QIntValidator(0, 9999, qaCPValue));
    f->addRow(qaCPLabel, qaCPValue);

    QLabel *qaCadLabel = new QLabel(tr("RPM:"), this);
    qaCadValue = new QLineEdit(QString("%1").arg(pfPvPlot->getCAD()));
    qaCadValue->setValidator(new QIntValidator(0, 999, qaCadValue));
    f->addRow(qaCadLabel, qaCadValue);

    QLabel *qaClLabel = new QLabel(tr("Crank Length (m):"), this);
    qaClValue = new QLineEdit(QString("%1").arg(1000 * pfPvPlot->getCL()));
    f->addRow(qaClLabel, qaClValue);

    shadeZonesPfPvCheckBox = new QCheckBox;
    shadeZonesPfPvCheckBox->setText(tr("Shade zones"));
    if (appsettings->value(this, GC_SHADEZONES, true).toBool() == true)
        shadeZonesPfPvCheckBox->setCheckState(Qt::Checked);
    else
        shadeZonesPfPvCheckBox->setCheckState(Qt::Unchecked);
    cl->addWidget(shadeZonesPfPvCheckBox);

    mergeIntervalPfPvCheckBox = new QCheckBox;
    mergeIntervalPfPvCheckBox->setText(tr("Merge intervals"));
    mergeIntervalPfPvCheckBox->setCheckState(Qt::Unchecked);
    cl->addWidget(mergeIntervalPfPvCheckBox);

    frameIntervalPfPvCheckBox = new QCheckBox;
    frameIntervalPfPvCheckBox->setText(tr("Frame intervals"));
    frameIntervalPfPvCheckBox->setCheckState(Qt::Checked);
    cl->addWidget(frameIntervalPfPvCheckBox);
    cl->addStretch();

    connect(pfPvPlot, SIGNAL(changedCP(const QString&)),
            qaCPValue, SLOT(setText(const QString&)) );
    connect(pfPvPlot, SIGNAL(changedCAD(const QString&)),
            qaCadValue, SLOT(setText(const QString&)) );
    connect(pfPvPlot, SIGNAL(changedCL(const QString&)),
            qaClValue, SLOT(setText(const QString&)) );
    connect(qaCPValue, SIGNAL(editingFinished()),
	    this, SLOT(setQaCPFromLineEdit()));
    connect(qaCadValue, SIGNAL(editingFinished()),
	    this, SLOT(setQaCADFromLineEdit()));
    connect(qaClValue, SIGNAL(editingFinished()),
	    this, SLOT(setQaCLFromLineEdit()));
    connect(shadeZonesPfPvCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(setShadeZonesPfPvFromCheckBox()));
    connect(rShade, SIGNAL(stateChanged(int)),
            this, SLOT(setrShadeZonesPfPvFromCheckBox()));
    connect(mergeIntervalPfPvCheckBox, SIGNAL(stateChanged(int)),
                this, SLOT(setMergeIntervalsPfPvFromCheckBox()));
    connect(rMergeInterval, SIGNAL(stateChanged(int)),
                this, SLOT(setrMergeIntervalsPfPvFromCheckBox()));
    connect(frameIntervalPfPvCheckBox, SIGNAL(stateChanged(int)),
                this, SLOT(setFrameIntervalsPfPvFromCheckBox()));
    connect(rFrameInterval, SIGNAL(stateChanged(int)),
                this, SLOT(setrFrameIntervalsPfPvFromCheckBox()));
    //connect(mainWindow, SIGNAL(rideSelected()), this, SLOT(rideSelected()));
    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(mainWindow, SIGNAL(intervalSelected()), this, SLOT(intervalSelected()));
    connect(mainWindow, SIGNAL(intervalsChanged()), this, SLOT(intervalSelected()));
    connect(mainWindow, SIGNAL(zonesChanged()), this, SLOT(zonesChanged()));
    connect(mainWindow, SIGNAL(configChanged()), pfPvPlot, SLOT(configChanged()));
}

void
PfPvWindow::rideSelected()
{
    if (!amVisible())
        return;


    RideItem *ride = myRideItem;
    if (ride == current || !ride || !ride->ride())
        return;

    pfPvPlot->setData(ride);

    current = ride;

    // update the QLabel widget with the CP value set in PfPvPlot::setData()
    qaCPValue->setText(QString("%1").arg(pfPvPlot->getCP()));
}

void
PfPvWindow::intervalSelected()
{
    RideItem *ride = myRideItem;
    if (!ride) return;
    pfPvPlot->showIntervals(ride);
}

void
PfPvWindow::zonesChanged()
{
    pfPvPlot->refreshZoneItems();
    pfPvPlot->replot();
    qaCPValue->setText(QString("%1").arg(pfPvPlot->getCP()));
}

void
PfPvWindow::setShadeZonesPfPvFromCheckBox()
{
    if (pfPvPlot->shadeZones() != shadeZonesPfPvCheckBox->isChecked()) {
        pfPvPlot->setShadeZones(shadeZonesPfPvCheckBox->isChecked());
        rShade->setChecked(shadeZonesPfPvCheckBox->isChecked());
    }
    pfPvPlot->replot();
}

void
PfPvWindow::setrShadeZonesPfPvFromCheckBox()
{
    if (pfPvPlot->shadeZones() != rShade->isChecked()) {
        pfPvPlot->setShadeZones(rShade->isChecked());
        shadeZonesPfPvCheckBox->setChecked(rShade->isChecked());
    }
    pfPvPlot->replot();
}

void
PfPvWindow::setMergeIntervalsPfPvFromCheckBox()
{
    if (pfPvPlot->mergeIntervals() != mergeIntervalPfPvCheckBox->isChecked()) {
        pfPvPlot->setMergeIntervals(mergeIntervalPfPvCheckBox->isChecked());
        rMergeInterval->setChecked(mergeIntervalPfPvCheckBox->isChecked());
    }
}

void
PfPvWindow::setrMergeIntervalsPfPvFromCheckBox()
{
    if (pfPvPlot->mergeIntervals() != rMergeInterval->isChecked()) {
        pfPvPlot->setMergeIntervals(rMergeInterval->isChecked());
        mergeIntervalPfPvCheckBox->setChecked(rMergeInterval->isChecked());
    }
}

void
PfPvWindow::setFrameIntervalsPfPvFromCheckBox()
{
    if (pfPvPlot->frameIntervals() != frameIntervalPfPvCheckBox->isChecked()) {
        pfPvPlot->setFrameIntervals(frameIntervalPfPvCheckBox->isChecked());
        rFrameInterval->setChecked(frameIntervalPfPvCheckBox->isChecked());
    }
}

void
PfPvWindow::setrFrameIntervalsPfPvFromCheckBox()
{
    if (pfPvPlot->frameIntervals() != rFrameInterval->isChecked()) {
        pfPvPlot->setFrameIntervals(rFrameInterval->isChecked());
        frameIntervalPfPvCheckBox->setChecked(rFrameInterval->isChecked());
    }
}

void
PfPvWindow::setQaCPFromLineEdit()
{
    int value = qaCPValue->text().toInt();
    pfPvPlot->setCP(value);
    pfPvPlot->replot();
}

void
PfPvWindow::setQaCADFromLineEdit()
{
    int value = qaCadValue->text().toInt();
    pfPvPlot->setCAD(value);
    pfPvPlot->replot();
}

void
PfPvWindow::setQaCLFromLineEdit()
{
    double value = qaClValue->text().toDouble();
    pfPvPlot->setCL(value);
    pfPvPlot->replot();
}


