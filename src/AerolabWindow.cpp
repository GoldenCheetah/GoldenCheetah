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
#include "Colors.h"
#include <QtGui>
#include <qwt_plot_zoomer.h>

AerolabWindow::AerolabWindow(MainWindow *mainWindow) :
  QWidget(mainWindow), mainWindow(mainWindow) {

  // Aerolab tab layout:
  QVBoxLayout *vLayout      = new QVBoxLayout;
  QHBoxLayout *cLayout      = new QHBoxLayout;

  // Plot:
  aerolab = new Aerolab(this, mainWindow);

  // Left controls layout:
  QVBoxLayout *leftControls  =  new QVBoxLayout;
  QFontMetrics metrics(QApplication::font());
  int labelWidth1 = metrics.width("Crr") + 10;

  // Crr:
  QHBoxLayout *crrLayout = new QHBoxLayout;
  QLabel *crrLabel = new QLabel(tr("Crr"), this);
  crrLabel->setFixedWidth(labelWidth1);
  crrLineEdit = new QLineEdit();
  crrLineEdit->setFixedWidth(70);
  crrLineEdit->setText(QString("%1").arg(aerolab->getCrr()) );
  /*crrQLCDNumber    = new QLCDNumber(7);
  crrQLCDNumber->setMode(QLCDNumber::Dec);
  crrQLCDNumber->setSmallDecimalPoint(false);
  crrQLCDNumber->setSegmentStyle(QLCDNumber::Flat);
  crrQLCDNumber->display(QString("%1").arg(aerolab->getCrr()) );*/
  crrSlider = new QSlider(Qt::Horizontal);
  crrSlider->setTickPosition(QSlider::TicksBelow);
  crrSlider->setTickInterval(1000);
  crrSlider->setMinimum(1000);
  crrSlider->setMaximum(10000);
  crrSlider->setValue(aerolab->intCrr());
  crrLayout->addWidget( crrLabel );
  crrLayout->addWidget( crrLineEdit );
  //crrLayout->addWidget( crrQLCDNumber );
  crrLayout->addWidget( crrSlider );

  // CdA:
  QHBoxLayout *cdaLayout = new QHBoxLayout;
  QLabel *cdaLabel = new QLabel(tr("CdA"), this);
  cdaLabel->setFixedWidth(labelWidth1);
  cdaLineEdit = new QLineEdit();
  cdaLineEdit->setFixedWidth(70);
  cdaLineEdit->setText(QString("%1").arg(aerolab->getCda()) );
  /*cdaQLCDNumber    = new QLCDNumber(7);
  cdaQLCDNumber->setMode(QLCDNumber::Dec);
  cdaQLCDNumber->setSmallDecimalPoint(false);
  cdaQLCDNumber->setSegmentStyle(QLCDNumber::Flat);
  cdaQLCDNumber->display(QString("%1").arg(aerolab->getCda()) );*/
  cdaSlider = new QSlider(Qt::Horizontal);
  cdaSlider->setTickPosition(QSlider::TicksBelow);
  cdaSlider->setTickInterval(100);
  cdaSlider->setMinimum(1500);
  cdaSlider->setMaximum(6000);
  cdaSlider->setValue(aerolab->intCda());
  cdaLayout->addWidget( cdaLabel );
  //cdaLayout->addWidget( cdaQLCDNumber );
  cdaLayout->addWidget( cdaLineEdit );
  cdaLayout->addWidget( cdaSlider );

  // Eta:
  QHBoxLayout *etaLayout = new QHBoxLayout;
  QLabel *etaLabel = new QLabel(tr("Eta"), this);
  etaLabel->setFixedWidth(labelWidth1);
  etaLineEdit = new QLineEdit();
  etaLineEdit->setFixedWidth(70);
  etaLineEdit->setText(QString("%1").arg(aerolab->getEta()) );
  /*etaQLCDNumber    = new QLCDNumber(7);
  etaQLCDNumber->setMode(QLCDNumber::Dec);
  etaQLCDNumber->setSmallDecimalPoint(false);
  etaQLCDNumber->setSegmentStyle(QLCDNumber::Flat);
  etaQLCDNumber->display(QString("%1").arg(aerolab->getEta()) );*/
  etaSlider = new QSlider(Qt::Horizontal);
  etaSlider->setTickPosition(QSlider::TicksBelow);
  etaSlider->setTickInterval(1000);
  etaSlider->setMinimum(8000);
  etaSlider->setMaximum(12000);
  etaSlider->setValue(aerolab->intEta());
  etaLayout->addWidget( etaLabel );
  etaLayout->addWidget( etaLineEdit );
  //etaLayout->addWidget( etaQLCDNumber );
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
  mLineEdit = new QLineEdit();
  mLineEdit->setFixedWidth(70);
  mLineEdit->setText(QString("%1").arg(aerolab->getTotalMass()) );
  /*mQLCDNumber    = new QLCDNumber(7);
  mQLCDNumber->setMode(QLCDNumber::Dec);
  mQLCDNumber->setSmallDecimalPoint(false);
  mQLCDNumber->setSegmentStyle(QLCDNumber::Flat);
  mQLCDNumber->display(QString("%1").arg(aerolab->getTotalMass()) );*/
  mSlider = new QSlider(Qt::Horizontal);
  mSlider->setTickPosition(QSlider::TicksBelow);
  mSlider->setTickInterval(1000);
  mSlider->setMinimum(3500);
  mSlider->setMaximum(15000);
  mSlider->setValue(aerolab->intTotalMass());
  mLayout->addWidget( mLabel );
  mLayout->addWidget( mLineEdit );
  //mLayout->addWidget( mQLCDNumber );
  mLayout->addWidget( mSlider );

  // Rho:
  QHBoxLayout *rhoLayout = new QHBoxLayout;
  QLabel *rhoLabel = new QLabel(tr("Rho (kg/m^3)"), this);
  rhoLabel->setFixedWidth(labelWidth2);
  rhoLineEdit = new QLineEdit();
  rhoLineEdit->setFixedWidth(70);
  rhoLineEdit->setText(QString("%1").arg(aerolab->getRho()) );
  /*rhoQLCDNumber    = new QLCDNumber(7);
  rhoQLCDNumber->setMode(QLCDNumber::Dec);
  rhoQLCDNumber->setSmallDecimalPoint(false);
  rhoQLCDNumber->setSegmentStyle(QLCDNumber::Flat);
  rhoQLCDNumber->display(QString("%1").arg(aerolab->getRho()) );*/
  rhoSlider = new QSlider(Qt::Horizontal);
  rhoSlider->setTickPosition(QSlider::TicksBelow);
  rhoSlider->setTickInterval(1000);
  rhoSlider->setMinimum(9000);
  rhoSlider->setMaximum(14000);
  rhoSlider->setValue(aerolab->intRho());
  rhoLayout->addWidget( rhoLabel );
  rhoLayout->addWidget( rhoLineEdit );
  //rhoLayout->addWidget( rhoQLCDNumber );
  rhoLayout->addWidget( rhoSlider );

  // Elevation offset:
  QHBoxLayout *eoffsetLayout = new QHBoxLayout;
  QLabel *eoffsetLabel = new QLabel(tr("Eoffset (m)"), this);
  eoffsetLabel->setFixedWidth(labelWidth2);
  eoffsetLineEdit = new QLineEdit();
  eoffsetLineEdit->setFixedWidth(70);
  eoffsetLineEdit->setText(QString("%1").arg(aerolab->getEoffset()) );
  /*eoffsetQLCDNumber    = new QLCDNumber(7);
  eoffsetQLCDNumber->setMode(QLCDNumber::Dec);
  eoffsetQLCDNumber->setSmallDecimalPoint(false);
  eoffsetQLCDNumber->setSegmentStyle(QLCDNumber::Flat);
  eoffsetQLCDNumber->display(QString("%1").arg(aerolab->getEoffset()) );*/
  eoffsetSlider = new QSlider(Qt::Horizontal);
  eoffsetSlider->setTickPosition(QSlider::TicksBelow);
  eoffsetSlider->setTickInterval(20000);
  eoffsetSlider->setMinimum(-30000);
  eoffsetSlider->setMaximum(250000);
  eoffsetSlider->setValue(aerolab->intEoffset());
  eoffsetLayout->addWidget( eoffsetLabel );
  eoffsetLayout->addWidget( eoffsetLineEdit );
  //eoffsetLayout->addWidget( eoffsetQLCDNumber );
  eoffsetLayout->addWidget( eoffsetSlider );

  QCheckBox *eoffsetAuto = new QCheckBox(tr("eoffset auto"), this);
  eoffsetAuto->setCheckState(Qt::Checked);
  eoffsetLayout->addWidget(eoffsetAuto);

  QHBoxLayout *smoothLayout = new QHBoxLayout;
  QComboBox *comboDistance = new QComboBox();
  comboDistance->addItem(tr("X Axis Shows Time"));
  comboDistance->addItem(tr("X Axis Shows Distance"));
  comboDistance->setCurrentIndex(1);
  smoothLayout->addWidget(comboDistance);

  // Add to leftControls:
  rightControls->addLayout( mLayout );
  rightControls->addLayout( rhoLayout );
  rightControls->addLayout( eoffsetLayout );
  rightControls->addLayout( smoothLayout );


  // Assemble controls layout:
  cLayout->addLayout(leftControls);
  cLayout->addLayout(rightControls);

  // Zoomer:
  allZoomer = new QwtPlotZoomer(aerolab->canvas());
  allZoomer->setRubberBand(QwtPicker::RectRubberBand);
  allZoomer->setSelectionFlags(QwtPicker::DragSelection
                            | QwtPicker::CornerToCorner);
  allZoomer->setTrackerMode(QwtPicker::AlwaysOff);
  allZoomer->setEnabled(true);
  allZoomer->setMousePattern( QwtEventPattern::MouseSelect2, Qt::RightButton, Qt::ControlModifier );
  allZoomer->setMousePattern( QwtEventPattern::MouseSelect3, Qt::RightButton );

  // SIGNALs to SLOTs:
  connect(mainWindow, SIGNAL(rideSelected()), this, SLOT(rideSelected()));
  connect(crrSlider, SIGNAL(valueChanged(int)),this, SLOT(setCrrFromSlider()));
  connect(crrLineEdit, SIGNAL(textChanged(const QString)), this, SLOT(setCrrFromText(const QString)));
  connect(cdaSlider, SIGNAL(valueChanged(int)), this, SLOT(setCdaFromSlider()));
  connect(cdaLineEdit, SIGNAL(textChanged(const QString)), this, SLOT(setCdaFromText(const QString)));
  connect(mSlider, SIGNAL(valueChanged(int)),this, SLOT(setTotalMassFromSlider()));
  connect(mLineEdit, SIGNAL(textChanged(const QString)), this, SLOT(setTotalMassFromText(const QString)));
  connect(rhoSlider, SIGNAL(valueChanged(int)), this, SLOT(setRhoFromSlider()));
  connect(rhoLineEdit, SIGNAL(textChanged(const QString)), this, SLOT(setRhoFromText(const QString)));
  connect(etaSlider, SIGNAL(valueChanged(int)), this, SLOT(setEtaFromSlider()));
  connect(etaLineEdit, SIGNAL(textChanged(const QString)), this, SLOT(setEtaFromText(const QString)));
  connect(eoffsetSlider, SIGNAL(valueChanged(int)), this, SLOT(setEoffsetFromSlider()));
  connect(eoffsetLineEdit, SIGNAL(textChanged(const QString)), this, SLOT(setEoffsetFromText(const QString)));
  connect(eoffsetAuto, SIGNAL(stateChanged(int)), this, SLOT(setAutoEoffset(int)));
  connect(comboDistance, SIGNAL(currentIndexChanged(int)), this, SLOT(setByDistance(int)));
  connect(mainWindow, SIGNAL(configChanged()), aerolab, SLOT(configChanged()));
  connect(mainWindow, SIGNAL(configChanged()), this, SLOT(configChanged()));
  connect(mainWindow, SIGNAL( intervalSelected() ), this, SLOT(intervalSelected()));
  connect(allZoomer, SIGNAL( zoomed(const QwtDoubleRect) ), this, SLOT(zoomChanged()));


  // Build the tab layout:
  vLayout->addWidget(aerolab);
  vLayout->addLayout(cLayout);
  setLayout(vLayout);


  // tooltip on hover over point
  //************************************
    aerolab->tooltip = new LTMToolTip( QwtPlot::xBottom,
                                       QwtPlot::yLeft,
                                       QwtPicker::PointSelection,
                                       QwtPicker::VLineRubberBand,
                                       QwtPicker::AlwaysOn,
                                       aerolab->canvas(),
                                       ""
                                     );
    aerolab->tooltip->setSelectionFlags( QwtPicker::PointSelection | QwtPicker::RectSelection | QwtPicker::DragSelection);
    aerolab->tooltip->setRubberBand( QwtPicker::VLineRubberBand );
    aerolab->tooltip->setMousePattern( QwtEventPattern::MouseSelect1, Qt::LeftButton, Qt::ShiftModifier );
    aerolab->tooltip->setTrackerPen( QColor( Qt::black ) );
    QColor inv( Qt::white );
    inv.setAlpha( 0 );
    aerolab->tooltip->setRubberBandPen( inv );
    aerolab->tooltip->setEnabled( true );
    aerolab->_canvasPicker = new LTMCanvasPicker( aerolab );

    connect( aerolab->_canvasPicker, SIGNAL( pointHover( QwtPlotCurve*, int ) ),
             aerolab,                SLOT  ( pointHover( QwtPlotCurve*, int ) ) );


  configChanged(); // pickup colors etc
}

void
AerolabWindow::zoomChanged()
{
    RideItem *ride = mainWindow->rideItem();
    aerolab->setData(ride, false);
}


void
AerolabWindow::configChanged()
{
  allZoomer->setRubberBandPen(GColor(CPLOTSELECT));
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
AerolabWindow::setCrrFromText(const QString text) {
  int value = 1000000 * text.toDouble();
  if (aerolab->intCrr() != value) {
    aerolab->setIntCrr(value);
    //crrQLCDNumber->display(QString("%1").arg(aerolab->getCrr()));
    crrSlider->setValue(aerolab->intCrr());
    RideItem *ride = mainWindow->rideItem();
    aerolab->setData(ride, false);
  }
}

void
AerolabWindow::setCrrFromSlider() {

  if (aerolab->intCrr() != crrSlider->value()) {
    aerolab->setIntCrr(crrSlider->value());
    //crrQLCDNumber->display(QString("%1").arg(aerolab->getCrr()));
    crrLineEdit->setText(QString("%1").arg(aerolab->getCrr()) );
    RideItem *ride = mainWindow->rideItem();
    aerolab->setData(ride, false);
  }
}

void
AerolabWindow::setCdaFromText(const QString text) {
  int value = 10000 * text.toDouble();
  if (aerolab->intCda() != value) {
    aerolab->setIntCda(value);
    //cdaQLCDNumber->display(QString("%1").arg(aerolab->getCda()));
    cdaSlider->setValue(aerolab->intCda());
    RideItem *ride = mainWindow->rideItem();
    aerolab->setData(ride, false);
  }
}

void
AerolabWindow::setCdaFromSlider() {

  if (aerolab->intCda() != cdaSlider->value()) {
    aerolab->setIntCda(cdaSlider->value());
    //cdaQLCDNumber->display(QString("%1").arg(aerolab->getCda()));
    cdaLineEdit->setText(QString("%1").arg(aerolab->getCda()) );
    RideItem *ride = mainWindow->rideItem();
    aerolab->setData(ride, false);
  }
}

void
AerolabWindow::setTotalMassFromText(const QString text) {
  int value = 100 * text.toDouble();
  if (value == 0)
      value = 1; // mass can not be zero !
  if (aerolab->intTotalMass() != value) {
    aerolab->setIntTotalMass(value);
    //mQLCDNumber->display(QString("%1").arg(aerolab->getTotalMass()));
    mSlider->setValue(aerolab->intTotalMass());
    RideItem *ride = mainWindow->rideItem();
    aerolab->setData(ride, false);
  }
}

void
AerolabWindow::setTotalMassFromSlider() {

  if (aerolab->intTotalMass() != mSlider->value()) {
    aerolab->setIntTotalMass(mSlider->value());
    //mQLCDNumber->display(QString("%1").arg(aerolab->getTotalMass()));
    mLineEdit->setText(QString("%1").arg(aerolab->getTotalMass()) );
    RideItem *ride = mainWindow->rideItem();
    aerolab->setData(ride, false);
  }
}

void
AerolabWindow::setRhoFromText(const QString text) {
  int value = 10000 * text.toDouble();
  if (aerolab->intRho() != value) {
    aerolab->setIntRho(value);
    //rhoQLCDNumber->display(QString("%1").arg(aerolab->getRho()));
    rhoSlider->setValue(aerolab->intRho());
    RideItem *ride = mainWindow->rideItem();
    aerolab->setData(ride, false);
  }
}

void
AerolabWindow::setRhoFromSlider() {

  if (aerolab->intRho() != rhoSlider->value()) {
    aerolab->setIntRho(rhoSlider->value());
    //rhoQLCDNumber->display(QString("%1").arg(aerolab->getRho()));
    rhoLineEdit->setText(QString("%1").arg(aerolab->getRho()) );
    RideItem *ride = mainWindow->rideItem();
    aerolab->setData(ride, false);
  }
}

void
AerolabWindow::setEtaFromText(const QString text) {
  int value = 10000 * text.toDouble();
  if (aerolab->intEta() != value) {
    aerolab->setIntEta(value);
    //etaQLCDNumber->display(QString("%1").arg(aerolab->getEta()));
    etaSlider->setValue(aerolab->intEta());
    RideItem *ride = mainWindow->rideItem();
    aerolab->setData(ride, false);
  }
}

void
AerolabWindow::setEtaFromSlider() {

  if (aerolab->intEta() != etaSlider->value()) {
    aerolab->setIntEta(etaSlider->value());
    //etaQLCDNumber->display(QString("%1").arg(aerolab->getEta()));
    etaLineEdit->setText(QString("%1").arg(aerolab->getEta()) );
    RideItem *ride = mainWindow->rideItem();
    aerolab->setData(ride, false);
  }
}

void
AerolabWindow::setEoffsetFromText(const QString text) {
  int value = 100 * text.toDouble();
  if (aerolab->intEoffset() != value) {
    aerolab->setIntEoffset(value);
    //eoffsetQLCDNumber->display(QString("%1").arg(aerolab->getEoffset()));
    eoffsetSlider->setValue(aerolab->intEoffset());
    RideItem *ride = mainWindow->rideItem();
    aerolab->setData(ride, false);
  }
}

void
AerolabWindow::setEoffsetFromSlider() {

  if (aerolab->intEoffset() != eoffsetSlider->value()) {
    aerolab->setIntEoffset(eoffsetSlider->value());
    //eoffsetQLCDNumber->display(QString("%1").arg(aerolab->getEoffset()));
    eoffsetLineEdit->setText(QString("%1").arg(aerolab->getEoffset()) );
    RideItem *ride = mainWindow->rideItem();
    aerolab->setData(ride, false);
  }
}

void
AerolabWindow::setAutoEoffset(int value)
{
    aerolab->setAutoEoffset(value);
}

void
AerolabWindow::setByDistance(int value)
{
    aerolab->setByDistance(value);
    // refresh
    RideItem *ride = mainWindow->rideItem();
    aerolab->setData(ride, false);
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

  aerolab->recalc(false);
}

void AerolabWindow::intervalSelected()
{
    if ( mainWindow->activeTab() != this )
    {
        return;
    }

    RideItem *ride = mainWindow->rideItem();
    if ( !ride )
    {
        return;
    }

    // set the elevation data
    aerolab->setData( ride, true );
}

double AerolabWindow::getCanvasTop() const
{
    const QwtDoubleRect &canvasRect = allZoomer->zoomRect();
    return canvasRect.top();
}

double AerolabWindow::getCanvasBottom() const
{
    const QwtDoubleRect &canvasRect = allZoomer->zoomRect();
    return canvasRect.bottom();
}
