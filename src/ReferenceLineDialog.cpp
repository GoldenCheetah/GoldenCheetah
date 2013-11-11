/*
 * Copyright (c) 2012 Damien Grauser (Damien.Grauser@pev-geneve.ch)
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

#include "ReferenceLineDialog.h"
#include "Athlete.h"
#include "Context.h"
#include "RideItem.h"

ReferenceLineDialog::ReferenceLineDialog(AllPlot *parent, Context *context, bool allowDelete) :
    parent(parent), context(context), allowDelete(allowDelete), axis(-1)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("Add Reference"));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *referenceValueLayout = new QHBoxLayout;
    QLabel *refLabel = new QLabel();
    refLabel->setText(tr("Add a reference at :"));
    refValue = new QLineEdit();
    refUnit = new QLabel();
    refUnit->setText(tr("Watts"));
    refValue->setFixedWidth(50);
    referenceValueLayout->addWidget(refLabel);
    referenceValueLayout->addWidget(refValue);
    referenceValueLayout->addWidget(refUnit);
    mainLayout->addLayout(referenceValueLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    addButton = new QPushButton(tr("&Add"));
    buttonLayout->addWidget(addButton);
    cancelButton = new QPushButton(tr("&Cancel"));
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    // below that line we will show the existing references
    // so they can be deleted

    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
}

void
ReferenceLineDialog::setValueForAxis(double value, int axis)
{
    this->axis = axis;
    refValue->setText(QString("%1").arg((int)value));
}


void
ReferenceLineDialog::cancelClicked()
{
    parent->plotTmpReference(axis, 0, 0); //unplot
    done(0);
}

void
ReferenceLineDialog::addClicked()
{
    RideFilePoint *refPoint = new RideFilePoint();
    refPoint->watts = refValue->text().toDouble();
    context->rideItem()->ride()->appendReference(*refPoint);
    context->rideItem()->setDirty(true);

    parent->refreshReferenceLinesForAllPlots();
    parent->plotTmpReference(axis, 0, 0); //unplot
    done(1);
}
