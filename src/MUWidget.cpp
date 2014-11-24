/*
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
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

#include "MUWidget.h"
#include "MUPool.h"
#include "Context.h"
#include "Athlete.h"
#include "Colors.h"

MUWidget::MUWidget(CriticalPowerWindow *parent, Context *context) 
       : QWidget(parent), context(context), parent(parent)
{

    setAutoFillBackground(true);

    QHBoxLayout *main = new QHBoxLayout(this);
    main->setSpacing(5);
    main->setContentsMargins(2,2,2,2);

    // mass
    massTitle = new QLabel(tr("Mass"), this);
    mass = new QDoubleSpinBox(this);
    mass->setDecimals(2);
    mass->setValue(12.00f);
    mass->setSingleStep(0.01f);
    mass->setMaximum(20.00);
    mass->setMinimum(5.00);
#if 0
    // derived Max power and Min Power (aka Cliff Young Power)
    maxTitle = new QLabel(tr("P-max"), this);
    minTitle = new QLabel(tr("P-min"), this);
    max = new QLabel("", this);
    min = new QLabel("", this);
#endif

    // range for tau
    tauTitle = new QLabel(tr("Tau"), this);
    tau0 = new QDoubleSpinBox(this);
    tau0->setDecimals(0);
    tau0->setSingleStep(1);
    tau0->setValue(MU_TAU0);

    tau1 = new QDoubleSpinBox(this);
    tau1->setDecimals(0);
    tau1->setSingleStep(1);
    tau1->setValue(MU_TAU1); 

    // range for pmax
    pmaxTitle = new QLabel(tr("Pmax"), this);
    pmax0 = new QDoubleSpinBox(this);
    pmax0->setDecimals(0);
    pmax0->setSingleStep(1);
    pmax0->setMaximum(300.00);
    pmax0->setMinimum(0.00);
    pmax0->setValue(MU_MAXP0);

    pmax1 = new QDoubleSpinBox(this);
    pmax1->setDecimals(0);
    pmax1->setSingleStep(1);
    pmax1->setMaximum(300.00);
    pmax1->setMinimum(0.00);
    pmax1->setValue(MU_MAXP1);

    // range for pmin
    pminTitle = new QLabel(tr("Pmin"), this);
    pmin0 = new QDoubleSpinBox(this);
    pmin0->setDecimals(0);
    pmin0->setSingleStep(1);
    pmin0->setMaximum(300.00);
    pmin0->setMinimum(0.00);
    pmin0->setValue(MU_MINP0);

    pmin1 = new QDoubleSpinBox(this);
    pmin1->setDecimals(0);
    pmin1->setSingleStep(1);
    pmin1->setMaximum(300.00);
    pmin1->setMinimum(0.00);
    pmin1->setValue(MU_MINP1);

    // alpha
    alphaTitle = new QLabel(tr("Alpha"), this);
    alpha = new QDoubleSpinBox(this);
    alpha->setDecimals(2);
    alpha->setSingleStep(0.01f);
    alpha->setValue(MU_ALPHA);

    // layout all the input widgets
    QGridLayout *grid = new QGridLayout;

#if 0
    grid->addWidget(maxTitle, 0, 2);
    grid->addWidget(minTitle, 0, 3);
    grid->addWidget(max, 1, 2);
    grid->addWidget(min, 1, 3);
#endif

    grid->addWidget(massTitle, 1, 0);
    grid->addWidget(mass, 1, 1);

    grid->addWidget(tauTitle, 2, 0);
    grid->addWidget(tau0, 2, 1);
    grid->addWidget(tau1, 2, 2);

    grid->addWidget(pmaxTitle, 3, 0);
    grid->addWidget(pmax0, 3, 1);
    grid->addWidget(pmax1, 3, 2);

    grid->addWidget(pminTitle, 4, 0);
    grid->addWidget(pmin0, 4, 1);
    grid->addWidget(pmin1, 4, 2);

    grid->addWidget(alphaTitle, 5,0);
    grid->addWidget(alpha, 5,1);

    // add the plot
    QVBoxLayout *rhs = new QVBoxLayout;
    rhs->addLayout(grid);
    rhs->addStretch();
    main->addWidget(muPlot = new MUPlot(this, parent, context));
    main->addLayout(rhs);

    // when config changes
    configChanged();

    connect(mass, SIGNAL(valueChanged(double)), muPlot, SLOT(setMUSet()));
    connect(tau0, SIGNAL(valueChanged(double)), muPlot, SLOT(setMUSet()));
    connect(tau1, SIGNAL(valueChanged(double)), muPlot, SLOT(setMUSet()));
    connect(pmax0, SIGNAL(valueChanged(double)), muPlot, SLOT(setMUSet()));
    connect(pmax1, SIGNAL(valueChanged(double)), muPlot, SLOT(setMUSet()));
    connect(pmin0, SIGNAL(valueChanged(double)), muPlot, SLOT(setMUSet()));
    connect(pmin1, SIGNAL(valueChanged(double)), muPlot, SLOT(setMUSet()));
    connect(alpha, SIGNAL(valueChanged(double)), muPlot, SLOT(setMUSet()));
}

// set colours mostly
void
MUWidget::configChanged()
{
    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    setPalette(palette);

    QColor bgColor = GColor(CPLOTBACKGROUND);
    QColor fgColor = GCColor::invertColor(bgColor);
    QColor border = bgColor;
    border = border.darker(300);

    QString style = QString("QLabel { color: %2; background: %1; }"
            "QDoubleSpinBox { color: %2; border: %3; background: %1; background-color: %1; }").arg(bgColor.name()).arg(fgColor.name()).arg(border.name());

    setStyleSheet(style);

#if 0
    // edit/labels use style
    maxTitle->setStyleSheet(style);
    minTitle->setStyleSheet(style);
    massTitle->setStyleSheet(style);
    mass->setStyleSheet(style);
    max->setStyleSheet(style);
    min->setStyleSheet(style);
    tauTitle->setStyleSheet(style);
    tau0->setStyleSheet(style);
    tau1->setStyleSheet(style);
    pmaxTitle->setStyleSheet(style);
    pmax0->setStyleSheet(style);
    pmax1->setStyleSheet(style);
    pminTitle->setStyleSheet(style);
    pmin0->setStyleSheet(style);
    pmin1->setStyleSheet(style);
    alphaTitle->setStyleSheet(style);
    alpha->setStyleSheet(style);
#endif
}
