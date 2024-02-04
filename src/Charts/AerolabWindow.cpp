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


#include "Context.h"
#include "Context.h"
#include "AerolabWindow.h"
#include "Aerolab.h"
#include "AbstractView.h"
#include "IntervalItem.h"
#include "RideItem.h"
#include "Colors.h"
#include "HelpWhatsThis.h"
#include "Units.h"
#include <QtGui>
#include <qwt_plot_zoomer.h>

AerolabWindow::AerolabWindow(Context *context) :
  GcChartWindow(context), context(context) {
    setControls(NULL);

  // Aerolab tab layout:
  QVBoxLayout *vLayout      = new QVBoxLayout;
  QHBoxLayout *cLayout      = new QHBoxLayout;

  // Plot:
  aerolab = new Aerolab(this, context);

  HelpWhatsThis *help = new HelpWhatsThis(aerolab);
  aerolab->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ChartRides_Aerolab));

  // Left controls layout:
  QVBoxLayout *leftControls  =  new QVBoxLayout;
  QFontMetrics metrics(QApplication::font());
  int labelWidth1 = metrics.horizontalAdvance("Crr") + 10;

  // Crr:
  QHBoxLayout *crrLayout = new QHBoxLayout;
  crrLabel = new QLabel(tr("Crr"), this);
  crrLabel->setFixedWidth(labelWidth1);
  crrLineEdit = new QLineEdit();
  crrLineEdit->setFixedWidth(75*dpiXFactor);
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
  crrSlider->setMaximum(25000);
  crrSlider->setValue(aerolab->intCrr());
  crrLayout->addWidget( crrLabel );
  crrLayout->addWidget( crrLineEdit );
  //crrLayout->addWidget( crrQLCDNumber );
  crrLayout->addWidget( crrSlider );

  // CdA:
  QHBoxLayout *cdaLayout = new QHBoxLayout;
  cdaLabel = new QLabel(tr("CdA"), this);
  cdaLabel->setFixedWidth(labelWidth1);
  cdaLineEdit = new QLineEdit();
  cdaLineEdit->setFixedWidth(75*dpiXFactor);
  cdaLineEdit->setText(QString("%1").arg(aerolab->getCda()) );
  /*cdaQLCDNumber    = new QLCDNumber(7);
  cdaQLCDNumber->setMode(QLCDNumber::Dec);
  cdaQLCDNumber->setSmallDecimalPoint(false);
  cdaQLCDNumber->setSegmentStyle(QLCDNumber::Flat);
  cdaQLCDNumber->display(QString("%1").arg(aerolab->getCda()) );*/
  cdaSlider = new QSlider(Qt::Horizontal);
  cdaSlider->setTickPosition(QSlider::TicksBelow);
  cdaSlider->setTickInterval(100);
  cdaSlider->setMinimum(1);
  cdaSlider->setMaximum(6000);
  cdaSlider->setValue(aerolab->intCda());
  cdaLayout->addWidget( cdaLabel );
  //cdaLayout->addWidget( cdaQLCDNumber );
  cdaLayout->addWidget( cdaLineEdit );
  cdaLayout->addWidget( cdaSlider );

  // Eta:
  QHBoxLayout *etaLayout = new QHBoxLayout;
  etaLabel = new QLabel(tr("Eta"), this);
  etaLabel->setFixedWidth(labelWidth1);
  etaLineEdit = new QLineEdit();
  etaLineEdit->setFixedWidth(75*dpiXFactor);
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

  // Eta:
  QHBoxLayout *commentLayout = new QHBoxLayout;
  commentLabel = new QLabel(tr("Comment"), this);
  commentLabel->setFixedWidth(labelWidth1);
  commentEdit = new QLineEdit();
  commentLayout->addWidget( commentLabel );
  commentLayout->addWidget( commentEdit );

  // Add to leftControls:
  leftControls->addLayout( crrLayout );
  leftControls->addLayout( cdaLayout );
  leftControls->addLayout( etaLayout );
  leftControls->addLayout( commentLayout );

  // Right controls layout:
  QVBoxLayout *rightControls  =  new QVBoxLayout;
  int labelWidth2 = metrics.horizontalAdvance("Total Mass (kg)") + 10;

  // Total mass:
  QHBoxLayout *mLayout = new QHBoxLayout;
  mLabel = new QLabel(tr("Total Mass (kg)"), this);
  mLabel->setFixedWidth(labelWidth2);
  mLineEdit = new QLineEdit();
  mLineEdit->setFixedWidth(70 * dpiXFactor);
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
  rhoLabel = new QLabel(tr("Rho (kg/m^3)"), this);
  rhoLabel->setFixedWidth(labelWidth2);
  rhoLineEdit = new QLineEdit();
  rhoLineEdit->setFixedWidth(70*dpiXFactor);
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
  eoffsetLabel = new QLabel(tr("Eoffset (m)"), this);
  eoffsetLabel->setFixedWidth(labelWidth2);
  eoffsetLineEdit = new QLineEdit();
  eoffsetLineEdit->setFixedWidth(70 * dpiXFactor);
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

  QVBoxLayout *checkboxLayout = new QVBoxLayout;
  eoffsetAuto = new QCheckBox(tr("eoffset auto"), this);
  eoffsetAuto->setCheckState(Qt::Checked);
  checkboxLayout->addWidget(eoffsetAuto);

  constantAlt = new QCheckBox(tr("Constant altitude (velodrome,...)"), this);
  checkboxLayout->addWidget(constantAlt);

  eoffsetLayout->addLayout(checkboxLayout);


  QHBoxLayout *smoothLayout = new QHBoxLayout;
  QComboBox *comboDistance = new QComboBox();
  comboDistance->addItem(tr("X Axis Shows Time"));
  comboDistance->addItem(tr("X Axis Shows Distance"));
  comboDistance->setCurrentIndex(1);
  smoothLayout->addWidget(comboDistance);

  QPushButton *btnEstCdACrr = new QPushButton(tr("&Estimate CdA and Crr"), this);
  smoothLayout->addWidget(btnEstCdACrr);

  btnSave = new QPushButton(tr("&Save parameters"), this);
  smoothLayout->addWidget(btnSave);

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
  allZoomer->setTrackerMode(QwtPicker::AlwaysOff);
  allZoomer->setEnabled(true);
  allZoomer->setMousePattern( QwtEventPattern::MouseSelect2, Qt::RightButton, Qt::ControlModifier );
  allZoomer->setMousePattern( QwtEventPattern::MouseSelect3, Qt::RightButton );

  // SIGNALs to SLOTs:
  connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
  connect(context, SIGNAL(rideChanged(RideItem*)), this, SLOT(rideSelected()));
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
  connect(commentEdit, SIGNAL(textChanged(const QString)), this, SLOT(setComment(const QString)));
  connect(eoffsetSlider, SIGNAL(valueChanged(int)), this, SLOT(setEoffsetFromSlider()));
  connect(eoffsetLineEdit, SIGNAL(textChanged(const QString)), this, SLOT(setEoffsetFromText(const QString)));
  connect(eoffsetAuto, SIGNAL(stateChanged(int)), this, SLOT(setAutoEoffset(int)));
  connect(constantAlt, SIGNAL(stateChanged(int)), this, SLOT(setConstantAlt(int)));
  connect(comboDistance, SIGNAL(currentIndexChanged(int)), this, SLOT(setByDistance(int)));
  connect(btnEstCdACrr, SIGNAL(clicked()), this, SLOT(doEstCdACrr()));
  connect(btnSave, SIGNAL(clicked()), this, SLOT(saveParametersInRide()));
  connect(context, SIGNAL(configChanged(qint32)), aerolab, SLOT(configChanged(qint32)));
  connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
  connect(context, SIGNAL(intervalSelected() ), this, SLOT(intervalSelected()));
  connect(context, SIGNAL(intervalZoom(IntervalItem*) ), this, SLOT(zoomInterval(IntervalItem*)));
  connect(allZoomer, SIGNAL( zoomed(const QRectF) ), this, SLOT(zoomChanged()));


  // Build the tab layout:
  vLayout->addWidget(aerolab);
  vLayout->addLayout(cLayout);
  setChartLayout(vLayout);


  // tooltip on hover over point
  //************************************
    aerolab->tooltip = new LTMToolTip( QwtAxis::XBottom,
                                       QwtAxis::YLeft,
                                       QwtPicker::VLineRubberBand,
                                       QwtPicker::AlwaysOn,
                                       aerolab->canvas(),
                                       ""
                                     );
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


  configChanged(CONFIG_APPEARANCE); // pickup colors etc
}

void
AerolabWindow::zoomChanged()
{
    RideItem *ride = myRideItem;
    refresh(ride, false);
}


void
AerolabWindow::configChanged(qint32)
{
  allZoomer->setRubberBandPen(GColor(GCol::PLOTSELECT));
  setProperty("color", GColor(GCol::PLOTBACKGROUND));

  QPalette palette;

  palette.setColor(QPalette::Window, GColor(GCol::PLOTBACKGROUND));

  // only change base if moved away from white plots
  // which is a Mac thing
#ifndef Q_OS_MAC
  if (GColor(GCol::PLOTBACKGROUND) != Qt::white)
#endif
  {
      palette.setColor(QPalette::Base, GAlternateColor(GCol::PLOTBACKGROUND));
      palette.setColor(QPalette::Window,  GColor(GCol::PLOTBACKGROUND));
  }

  palette.setColor(QPalette::WindowText, GInvertColor(GCol::PLOTBACKGROUND));
  palette.setColor(QPalette::Text, GInvertColor(GCol::PLOTBACKGROUND));
  setPalette(palette);
  aerolab->setPalette(palette);
  crrLabel->setPalette(palette);
  cdaLabel->setPalette(palette);
  etaLabel->setPalette(palette);
  commentLabel->setPalette(palette);
  mLabel->setPalette(palette);
  rhoLabel->setPalette(palette);
  eoffsetLabel->setPalette(palette);
  crrSlider->setPalette(palette);
  crrLineEdit->setPalette(palette);
  cdaSlider->setPalette(palette);
  cdaLineEdit->setPalette(palette);
  mSlider->setPalette(palette);
  mLineEdit->setPalette(palette);
  rhoSlider->setPalette(palette);
  rhoLineEdit->setPalette(palette);
  etaSlider->setPalette(palette);
  etaLineEdit->setPalette(palette);
  eoffsetLineEdit->setPalette(palette);
  eoffsetAuto->setPalette(palette);
  constantAlt->setPalette(palette);
  commentEdit->setPalette(palette);

#ifndef Q_OS_MAC
    aerolab->setStyleSheet(AbstractView::ourStyleSheet());
#endif
}

void
AerolabWindow::rideSelected() {

  RideItem *ride = myRideItem;
  if (!amVisible() || ride == NULL || ride->ride() == NULL) return;

  commentEdit->setText("");

  // Read values en Ride
  double totalMass = ride->ride()->getTag("Total Weight", "0").toDouble();
  if (totalMass>0) {
    int value = 100 * totalMass;
    aerolab->setIntTotalMass(value);
    mLineEdit->setText(QString("%1").arg(aerolab->getTotalMass()) );
    mSlider->setValue(aerolab->intTotalMass());
  }

  if (hasNewParametersInRide()) {
      double eta = ride->ride()->getTag("aerolab.Eta", "0").toDouble();
      if (eta>0) {
        int value = 10000 * eta;
        aerolab->setIntEta(value);
        etaLineEdit->setText(QString("%1").arg(aerolab->getEta()) );
        etaSlider->setValue(aerolab->intEta());
      }

      double rho = ride->ride()->getTag("aerolab.Rho", "0").toDouble();
      if (rho>0) {
        int value = 10000 * rho;
        aerolab->setIntRho(value);
        rhoLineEdit->setText(QString("%1").arg(aerolab->getRho()) );
        rhoSlider->setValue(aerolab->intRho());
      }

      double eoffset = ride->ride()->getTag("aerolab.EOffset", "0").toDouble();
      if (eoffset!=0) {
        int value = 100 * eoffset;
        aerolab->setIntEoffset(value);
        eoffsetLineEdit->setText(QString("%1").arg(aerolab->getEoffset()) );
        eoffsetSlider->setValue(aerolab->intEoffset());
      }

      double cda = ride->ride()->getTag("aerolab.Cda", "0").toDouble();
      if (cda>0) {
        int value = 10000 * cda;
        aerolab->setIntCda(value);
        cdaLineEdit->setText(QString("%1").arg(aerolab->getCda()) );
        cdaSlider->setValue(aerolab->intCda());
      }

      double crr = ride->ride()->getTag("aerolab.Crr", "0").toDouble();
      if (crr>0) {
        int value = 1000000 * crr;
        aerolab->setIntCrr(value);
        crrLineEdit->setText(QString("%1").arg(aerolab->getCrr()) );
        crrSlider->setValue(aerolab->intCrr());
      }

      QString comment = ride->ride()->getTag("aerolab.Comment", "");
      if (comment.length()>0) {
        commentEdit->setText(comment);
      }
    }

  if (!ride)
    return;



  refresh(ride, true);

  allZoomer->setZoomBase();
}

void
AerolabWindow::setCrrFromText(const QString text) {
  int value = 1000000 * text.toDouble();
  if (aerolab->intCrr() != value) {
    aerolab->setIntCrr(value);
    //crrQLCDNumber->display(QString("%1").arg(aerolab->getCrr()));
    crrSlider->setValue(aerolab->intCrr());
    RideItem *ride = context->rideItem();
    refresh(ride, false);
  }
}

void
AerolabWindow::setCrrFromSlider() {

  if (aerolab->intCrr() != crrSlider->value()) {
    aerolab->setIntCrr(crrSlider->value());
    //crrQLCDNumber->display(QString("%1").arg(aerolab->getCrr()));
    crrLineEdit->setText(QString("%1").arg(aerolab->getCrr()) );
    RideItem *ride = myRideItem;
    refresh(ride, false);
  }
}

void
AerolabWindow::setCdaFromText(const QString text) {
  int value = 10000 * text.toDouble();
  if (aerolab->intCda() != value) {
    aerolab->setIntCda(value);
    //cdaQLCDNumber->display(QString("%1").arg(aerolab->getCda()));
    cdaSlider->setValue(aerolab->intCda());
    RideItem *ride = myRideItem;
    refresh(ride, false);
  }
}

void
AerolabWindow::setCdaFromSlider() {

  if (aerolab->intCda() != cdaSlider->value()) {
    aerolab->setIntCda(cdaSlider->value());
    //cdaQLCDNumber->display(QString("%1").arg(aerolab->getCda()));
    cdaLineEdit->setText(QString("%1").arg(aerolab->getCda()) );
    RideItem *ride = myRideItem;
    refresh(ride, false);
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
    RideItem *ride = context->rideItem();
    refresh(ride, false);
  }
}

void
AerolabWindow::setTotalMassFromSlider() {

  if (aerolab->intTotalMass() != mSlider->value()) {
    aerolab->setIntTotalMass(mSlider->value());
    //mQLCDNumber->display(QString("%1").arg(aerolab->getTotalMass()));
    mLineEdit->setText(QString("%1").arg(aerolab->getTotalMass()) );
    RideItem *ride = myRideItem;
    refresh(ride, false);
  }
}

void
AerolabWindow::setRhoFromText(const QString text) {
  int value = 10000 * text.toDouble();
  if (aerolab->intRho() != value) {
    aerolab->setIntRho(value);
    //rhoQLCDNumber->display(QString("%1").arg(aerolab->getRho()));
    rhoSlider->setValue(aerolab->intRho());
    RideItem *ride = myRideItem;
    refresh(ride, false);
  }
}

void
AerolabWindow::setRhoFromSlider() {

  if (aerolab->intRho() != rhoSlider->value()) {
    aerolab->setIntRho(rhoSlider->value());
    //rhoQLCDNumber->display(QString("%1").arg(aerolab->getRho()));
    rhoLineEdit->setText(QString("%1").arg(aerolab->getRho()) );
    RideItem *ride = myRideItem;
    refresh(ride, false);
  }
}

void
AerolabWindow::setEtaFromText(const QString text) {
  int value = 10000 * text.toDouble();
  if (aerolab->intEta() != value) {
    aerolab->setIntEta(value);
    //etaQLCDNumber->display(QString("%1").arg(aerolab->getEta()));
    etaSlider->setValue(aerolab->intEta());
    RideItem *ride = myRideItem;
    refresh(ride, false);
  }
}

void
AerolabWindow::setComment(const QString ) {
    if (hasNewParametersInRide()) {
        btnSave->setEnabled(true);
    } else
        btnSave->setEnabled(false);
}

void
AerolabWindow::setEtaFromSlider() {

  if (aerolab->intEta() != etaSlider->value()) {
    aerolab->setIntEta(etaSlider->value());
    //etaQLCDNumber->display(QString("%1").arg(aerolab->getEta()));
    etaLineEdit->setText(QString("%1").arg(aerolab->getEta()) );
    RideItem *ride = myRideItem;
    refresh(ride, false);
  }
}

void
AerolabWindow::setEoffsetFromText(const QString text) {
  int value = 100 * text.toDouble();
  if (aerolab->intEoffset() != value) {
    aerolab->setIntEoffset(value);
    //eoffsetQLCDNumber->display(QString("%1").arg(aerolab->getEoffset()));
    eoffsetSlider->setValue(aerolab->intEoffset());
    RideItem *ride = myRideItem;
    refresh(ride, false);
  }
}

void
AerolabWindow::setEoffsetFromSlider() {

  if (aerolab->intEoffset() != eoffsetSlider->value()) {
    aerolab->setIntEoffset(eoffsetSlider->value());
    //eoffsetQLCDNumber->display(QString("%1").arg(aerolab->getEoffset()));
    eoffsetLineEdit->setText(QString("%1").arg(aerolab->getEoffset()) );
    RideItem *ride = myRideItem;
    refresh(ride, false);
  }
}

void
AerolabWindow::setAutoEoffset(int value)
{
    aerolab->setAutoEoffset(value);
}

void
AerolabWindow::setConstantAlt(int value)
{
    aerolab->setConstantAlt(value);
    // refresh
    RideItem *ride = myRideItem;
    refresh(ride, false);
}

void
AerolabWindow::setByDistance(int value)
{
    aerolab->setByDistance(value);
    // refresh
    RideItem *ride = myRideItem;
    refresh(ride, false);
}

void
AerolabWindow::doEstCdACrr()
{
    RideItem *ride = context->rideItem();
    /* Estimate Crr&Cda */
    const QString errMsg = aerolab->estimateCdACrr(ride);
    if (errMsg.isEmpty()) {
        /* Update Crr/Cda values values in UI */
        crrLineEdit->setText(QString("%1").arg(aerolab->getCrr()) );
        crrSlider->setValue(aerolab->intCrr());
        cdaLineEdit->setText(QString("%1").arg(aerolab->getCda()) );
        cdaSlider->setValue(aerolab->intCda());
        /* Refresh */
        refresh(ride, false);
    } else {
        /* report error: insufficient data to estimate Cda&Crr */
        QMessageBox::warning(this, tr("Estimate CdA and Crr"), errMsg);
    }
}


void
AerolabWindow::zoomInterval(IntervalItem *which) {
  QRectF rect;

  if (!aerolab->byDistance()) {
    rect.setLeft(which->start/60);
    rect.setRight(which->stop/60);
  } else {
    rect.setLeft(which->startKM * (GlobalContext::context()->useMetricUnits ? 1 : MILES_PER_KM));
    rect.setRight(which->stopKM * (GlobalContext::context()->useMetricUnits ? 1 : MILES_PER_KM));
  }
  rect.setTop(aerolab->veCurve->maxYValue()*1.1);
  rect.setBottom(aerolab->veCurve->minYValue()-10);
  allZoomer->zoom(rect);

  aerolab->recalc(false);
}

void AerolabWindow::intervalSelected()
{
    RideItem *ride = myRideItem;
    if (!amVisible() || ride == NULL || ride->ride() == NULL) return;

    // set the elevation data
    refresh(ride, true);
}

double AerolabWindow::getCanvasTop() const
{
    const QRectF &canvasRect = allZoomer->zoomRect();
    return canvasRect.top();
}

double AerolabWindow::getCanvasBottom() const
{
    const QRectF &canvasRect = allZoomer->zoomRect();
    return canvasRect.bottom();
}

void AerolabWindow::saveParametersInRide()
{
    if (hasNewParametersInRide()) {
        rideItem()->ride()->setTag("aerolab.Cda", QString("%1").arg(aerolab->getCda()));
        rideItem()->ride()->setTag("aerolab.Crr", QString("%1").arg(aerolab->getCrr()));
        rideItem()->ride()->setTag("Total Weight", QString("%1").arg(aerolab->getTotalMass()));
        rideItem()->ride()->setTag("aerolab.Eta", QString("%1").arg(aerolab->getEta()));
        rideItem()->ride()->setTag("aerolab.Rho", QString("%1").arg(aerolab->getRho()));
        rideItem()->ride()->setTag("aerolab.EOffset", QString("%1").arg(aerolab->getEoffset()));
        rideItem()->ride()->setTag("aerolab.Comment", QString("%1").arg(commentEdit->text()));
        rideItem()->setDirty(true);

        context->mainWindow->saveSilent(context, rideItem());
        btnSave->setEnabled(false);
     }
}

bool AerolabWindow::hasNewParametersInRide()
{
    if (rideItem() == NULL || rideItem()->ride() == NULL) return false;

    bool newValues = false;

    if (rideItem()->ride()->getTag("Total Weight", "0").toDouble() != aerolab->getTotalMass()) {
        newValues = true;
        //qDebug() << "new Total Weight" << rideItem()->ride()->getTag("Total Weight", "0").toDouble() << aerolab->getTotalMass();
    }
    if (rideItem()->ride()->getTag("aerolab.Eta", "0").toDouble() != aerolab->getEta()) {
        newValues = true;
        //qDebug() << "new Eta" << rideItem()->ride()->getTag("aerolab.Eta", "0").toDouble() << aerolab->getEta();
    }
    if (rideItem()->ride()->getTag("aerolab.Rho", "0").toDouble() != aerolab->getRho()) {
        newValues = true;
        //qDebug() << "new Rho" << rideItem()->ride()->getTag("aerolab.Rho", "0").toDouble() << aerolab->getRho();
    }
    if (rideItem()->ride()->getTag("aerolab.EOffset", "0").toDouble() != aerolab->getEoffset()) {
        newValues = true;
        //qDebug() << "new EOffset" << rideItem()->ride()->getTag("aerolab.EOffset", "0").toDouble() << aerolab->getEoffset();
    }
    if (rideItem()->ride()->getTag("aerolab.Cda", "0").toDouble() != aerolab->getCda()) {
        newValues = true;
        //qDebug() << "new Cda" << rideItem()->ride()->getTag("aerolab.Cda", "0").toDouble() << aerolab->getCda();
    }
    if (rideItem()->ride()->getTag("aerolab.Crr", "0").toDouble() != aerolab->getCrr()) {
        newValues = true;
        //qDebug() << "new Crr" << rideItem()->ride()->getTag("aerolab.Crr", "0").toDouble() << aerolab->getCrr();
    }
    if (rideItem()->ride()->getTag("aerolab.Comment", "") != commentEdit->text()) {
        newValues = true;
        //qDebug() << "new Comment" << rideItem()->ride()->getTag("aerolab.Comment", "") << commentEdit->text();
    }

    return newValues;
}

void AerolabWindow::refresh(RideItem *_rideItem, bool newzoom) {
    if (hasNewParametersInRide()) {
        btnSave->setEnabled(true);
    } else
        btnSave->setEnabled(false);

    aerolab->setData(_rideItem, newzoom);
}
