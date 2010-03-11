/*
 * Copyright (c) 2009 Andy M. Froncioni (me@andyfroncioni.com)
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


#include "MainWindow.h"
#include "AerolabWindow.h"
#include "Aerolab.h"
#include "RideItem.h"
#include <QtGui>
#include <qwt_plot_zoomer.h>

AerolabWindow::AerolabWindow(MainWindow *mainWindow) :
  QWidget(mainWindow), mainWindow(mainWindow) {

  // Aerolab tab layout:
  QVBoxLayout *vLayout      = new QVBoxLayout;
  QHBoxLayout *cLayout      = new QHBoxLayout;

  // Plot:
  aerolab = new Aerolab(this);

  // Left controls layout:
  QVBoxLayout *leftControls  =  new QVBoxLayout;
  QFontMetrics metrics(QApplication::font());
  int labelWidth1 = metrics.width("Crr") + 10;

  // Crr:
  QHBoxLayout *crrLayout = new QHBoxLayout;
  QLabel *crrLabel = new QLabel(tr("Crr"), this);
  crrLabel->setFixedWidth(labelWidth1);
  crrQLCDNumber    = new QLCDNumber(7);
  crrQLCDNumber->setMode(QLCDNumber::Dec);
  crrQLCDNumber->setSmallDecimalPoint(false);
  crrQLCDNumber->setSegmentStyle(QLCDNumber::Flat);
  crrQLCDNumber->display(QString("%1").arg(aerolab->getCrr()) );
  crrSlider = new QSlider(Qt::Horizontal);
  crrSlider->setTickPosition(QSlider::TicksBelow);
  crrSlider->setTickInterval(1000);
  crrSlider->setMinimum(1000);
  crrSlider->setMaximum(10000);
  crrSlider->setValue(aerolab->intCrr());
  crrLayout->addWidget( crrLabel );
  crrLayout->addWidget( crrQLCDNumber );
  crrLayout->addWidget( crrSlider );

  // CdA:
  QHBoxLayout *cdaLayout = new QHBoxLayout;
  QLabel *cdaLabel = new QLabel(tr("CdA"), this);
  cdaLabel->setFixedWidth(labelWidth1);
  cdaQLCDNumber    = new QLCDNumber(7);
  cdaQLCDNumber->setMode(QLCDNumber::Dec);
  cdaQLCDNumber->setSmallDecimalPoint(false);
  cdaQLCDNumber->setSegmentStyle(QLCDNumber::Flat);
  cdaQLCDNumber->display(QString("%1").arg(aerolab->getCda()) );
  cdaSlider = new QSlider(Qt::Horizontal);
  cdaSlider->setTickPosition(QSlider::TicksBelow);
  cdaSlider->setTickInterval(100);
  cdaSlider->setMinimum(1500);
  cdaSlider->setMaximum(6000);
  cdaSlider->setValue(aerolab->intCda());
  cdaLayout->addWidget( cdaLabel );
  cdaLayout->addWidget( cdaQLCDNumber );
  cdaLayout->addWidget( cdaSlider );

  // Eta:
  QHBoxLayout *etaLayout = new QHBoxLayout;
  QLabel *etaLabel = new QLabel(tr("Eta"), this);
  etaLabel->setFixedWidth(labelWidth1);
  etaQLCDNumber    = new QLCDNumber(7);
  etaQLCDNumber->setMode(QLCDNumber::Dec);
  etaQLCDNumber->setSmallDecimalPoint(false);
  etaQLCDNumber->setSegmentStyle(QLCDNumber::Flat);
  etaQLCDNumber->display(QString("%1").arg(aerolab->getEta()) );
  etaSlider = new QSlider(Qt::Horizontal);
  etaSlider->setTickPosition(QSlider::TicksBelow);
  etaSlider->setTickInterval(1000);
  etaSlider->setMinimum(8000);
  etaSlider->setMaximum(12000);
  etaSlider->setValue(aerolab->intEta());
  etaLayout->addWidget( etaLabel );
  etaLayout->addWidget( etaQLCDNumber );
  etaLayout->addWidget( etaSlider );

  // Add to leftControls:
  leftControls->addLayout( crrLayout );
  leftControls->addLayout( cdaLayout );
  leftControls->addLayout( etaLayout );

  // Right controls layout:
  QVBoxLayout *rightControls  =  new QVBoxLayout;
  int labelWidth2 = metrics.width("Total Mass (kg)") + 10;

  // Total mass:
  QHBoxLayout *mLayout = new QHBoxLayout;
  QLabel *mLabel = new QLabel(tr("Total Mass (kg)"), this);
  mLabel->setFixedWidth(labelWidth2);
  mQLCDNumber    = new QLCDNumber(7);
  mQLCDNumber->setMode(QLCDNumber::Dec);
  mQLCDNumber->setSmallDecimalPoint(false);
  mQLCDNumber->setSegmentStyle(QLCDNumber::Flat);
  mQLCDNumber->display(QString("%1").arg(aerolab->getTotalMass()) );
  mSlider = new QSlider(Qt::Horizontal);
  mSlider->setTickPosition(QSlider::TicksBelow);
  mSlider->setTickInterval(1000);
  mSlider->setMinimum(3500);
  mSlider->setMaximum(15000);
  mSlider->setValue(aerolab->intTotalMass());
  mLayout->addWidget( mLabel );
  mLayout->addWidget( mQLCDNumber );
  mLayout->addWidget( mSlider );

  // Rho:
  QHBoxLayout *rhoLayout = new QHBoxLayout;
  QLabel *rhoLabel = new QLabel(tr("Rho (kg/m^3)"), this);
  rhoLabel->setFixedWidth(labelWidth2);
  rhoQLCDNumber    = new QLCDNumber(7);
  rhoQLCDNumber->setMode(QLCDNumber::Dec);
  rhoQLCDNumber->setSmallDecimalPoint(false);
  rhoQLCDNumber->setSegmentStyle(QLCDNumber::Flat);
  rhoQLCDNumber->display(QString("%1").arg(aerolab->getRho()) );
  rhoSlider = new QSlider(Qt::Horizontal);
  rhoSlider->setTickPosition(QSlider::TicksBelow);
  rhoSlider->setTickInterval(1000);
  rhoSlider->setMinimum(9000);
  rhoSlider->setMaximum(14000);
  rhoSlider->setValue(aerolab->intRho());
  rhoLayout->addWidget( rhoLabel );
  rhoLayout->addWidget( rhoQLCDNumber );
  rhoLayout->addWidget( rhoSlider );

  // Elevation offset:
  QHBoxLayout *eoffsetLayout = new QHBoxLayout;
  QLabel *eoffsetLabel = new QLabel(tr("Eoffset (m)"), this);
  eoffsetLabel->setFixedWidth(labelWidth2);
  eoffsetQLCDNumber    = new QLCDNumber(7);
  eoffsetQLCDNumber->setMode(QLCDNumber::Dec);
  eoffsetQLCDNumber->setSmallDecimalPoint(false);
  eoffsetQLCDNumber->setSegmentStyle(QLCDNumber::Flat);
  eoffsetQLCDNumber->display(QString("%1").arg(aerolab->getEoffset()) );
  eoffsetSlider = new QSlider(Qt::Horizontal);
  eoffsetSlider->setTickPosition(QSlider::TicksBelow);
  eoffsetSlider->setTickInterval(1000);
  eoffsetSlider->setMinimum(-30000);
  eoffsetSlider->setMaximum(30000);
  eoffsetSlider->setValue(aerolab->intEoffset());
  eoffsetLayout->addWidget( eoffsetLabel );
  eoffsetLayout->addWidget( eoffsetQLCDNumber );
  eoffsetLayout->addWidget( eoffsetSlider );

  // Add to leftControls:
  rightControls->addLayout( mLayout );
  rightControls->addLayout( rhoLayout );
  rightControls->addLayout( eoffsetLayout );


  // Assemble controls layout:
  cLayout->addLayout(leftControls);
  cLayout->addLayout(rightControls);

  // Zoomer:
  allZoomer = new QwtPlotZoomer(aerolab->canvas());
  allZoomer->setRubberBand(QwtPicker::RectRubberBand);
  allZoomer->setRubberBandPen(QColor(Qt::black));
  allZoomer->setSelectionFlags(QwtPicker::DragSelection
                            | QwtPicker::CornerToCorner);
  allZoomer->setTrackerMode(QwtPicker::AlwaysOff);
  allZoomer->setEnabled(true);

  // SIGNALs to SLOTs:
  connect(mainWindow, SIGNAL(rideSelected()), this, SLOT(rideSelected()));
  connect(crrSlider, SIGNAL(valueChanged(int)),this, SLOT(setCrrFromSlider()));
  connect(cdaSlider, SIGNAL(valueChanged(int)), this, SLOT(setCdaFromSlider()));
  connect(mSlider, SIGNAL(valueChanged(int)),this, SLOT(setTotalMassFromSlider()));
  connect(rhoSlider, SIGNAL(valueChanged(int)), this, SLOT(setRhoFromSlider()));
  connect(etaSlider, SIGNAL(valueChanged(int)), this, SLOT(setEtaFromSlider()));
  connect(eoffsetSlider, SIGNAL(valueChanged(int)), this, SLOT(setEoffsetFromSlider()));

  // Build the tab layout:
  vLayout->addWidget(aerolab);
  vLayout->addLayout(cLayout);
  setLayout(vLayout);

}

void
AerolabWindow::rideSelected() {

  if (mainWindow->activeTab() != this)
    return;

  RideItem *ride = mainWindow->rideItem();

  if (!ride)
    return;

  aerolab->setData(ride, true);
  allZoomer->setZoomBase();
}

void
AerolabWindow::setCrrFromSlider() {

  if (aerolab->intCrr() != crrSlider->value()) {
    aerolab->setIntCrr(crrSlider->value());
    crrQLCDNumber->display(QString("%1").arg(aerolab->getCrr()));
    RideItem *ride = mainWindow->rideItem();
    aerolab->setData(ride, false);
  }
}

void
AerolabWindow::setCdaFromSlider() {

  if (aerolab->intCda() != cdaSlider->value()) {
    aerolab->setIntCda(cdaSlider->value());
    cdaQLCDNumber->display(QString("%1").arg(aerolab->getCda()));
    RideItem *ride = mainWindow->rideItem();
    aerolab->setData(ride, false);
  }
}

void
AerolabWindow::setTotalMassFromSlider() {

  if (aerolab->intTotalMass() != mSlider->value()) {
    aerolab->setIntTotalMass(mSlider->value());
    mQLCDNumber->display(QString("%1").arg(aerolab->getTotalMass()));
    RideItem *ride = mainWindow->rideItem();
    aerolab->setData(ride, false);
  }
}

void
AerolabWindow::setRhoFromSlider() {

  if (aerolab->intRho() != rhoSlider->value()) {
    aerolab->setIntRho(rhoSlider->value());
    rhoQLCDNumber->display(QString("%1").arg(aerolab->getRho()));
    RideItem *ride = mainWindow->rideItem();
    aerolab->setData(ride, false);
  }
}

void
AerolabWindow::setEtaFromSlider() {

  if (aerolab->intEta() != etaSlider->value()) {
    aerolab->setIntEta(etaSlider->value());
    etaQLCDNumber->display(QString("%1").arg(aerolab->getEta()));
    RideItem *ride = mainWindow->rideItem();
    aerolab->setData(ride, false);
  }
}

void
AerolabWindow::setEoffsetFromSlider() {

  if (aerolab->intEoffset() != eoffsetSlider->value()) {
    aerolab->setIntEoffset(eoffsetSlider->value());
    eoffsetQLCDNumber->display(QString("%1").arg(aerolab->getEoffset()));
    RideItem *ride = mainWindow->rideItem();
    aerolab->setData(ride, false);
  }
}


void
AerolabWindow::zoomInterval(IntervalItem *which) {
  QwtDoubleRect rect;

  if (!aerolab->byDistance()) {
    rect.setLeft(which->start/60);
    rect.setRight(which->stop/60);
  } else {
    rect.setLeft(which->startKM);
    rect.setRight(which->stopKM);
  }
  rect.setTop(aerolab->veCurve->maxYValue()*1.1);
  rect.setBottom(aerolab->veCurve->minYValue()-10);
  allZoomer->zoom(rect);
}

