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

ReferenceLineDialog::ReferenceLineDialog(AllPlot *parent, Context *context, RideFile::SeriesType serie, bool allowDelete) :
    parent(parent), context(context), serie(serie), allowDelete(allowDelete), axis(-1)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::Tool);
    setWindowTitle(tr("References"));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *referenceValueLayout = new QHBoxLayout;
    referenceValueLayout->setSpacing(0);
    referenceValueLayout->setContentsMargins(0,0,0,0);
    QLabel *refLabel = new QLabel();
    refLabel->setText(tr("Reference:"));
    refValue = new QLineEdit();
    refUnit = new QLabel();
    if (serie == RideFile::watts)
        refUnit->setText(tr("Watts"));
    else if (serie == RideFile::hr)
        refUnit->setText(tr("BPM"));
    refValue->setFixedWidth(50 *dpiXFactor);
    referenceValueLayout->addWidget(refLabel);
    referenceValueLayout->addWidget(refValue);
    referenceValueLayout->addWidget(refUnit);

    addButton = new QPushButton(tr(" + "));
#ifndef Q_OS_MAC
    addButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    addButton->setStyleSheet("QPushButton { padding: 0px; }");
#endif
    referenceValueLayout->addStretch();
    referenceValueLayout->addWidget(addButton);
    mainLayout->addLayout(referenceValueLayout);
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));


    // custom table
    refsTable = new QTableWidget(this);
#ifdef Q_OS_MAX
    refsTable->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    refsTable->setColumnCount(2);
    refsTable->horizontalHeader()->setStretchLastSection(true);
    refsTable->setSortingEnabled(false);
    refsTable->verticalHeader()->hide();
    refsTable->setShowGrid(false);
    refsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    refsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(refsTable);

    deleteRefButton = new QPushButton(" - ");
#ifndef Q_OS_MAC
    deleteRefButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    deleteRefButton->setStyleSheet("QPushButton { padding: 0px; }");
#endif
    connect(deleteRefButton, SIGNAL(clicked()), this, SLOT(deleteRef()));

    QHBoxLayout *refButtons = new QHBoxLayout;
    refButtons->setSpacing(2 *dpiXFactor);
    refButtons->addStretch();
    refButtons->addWidget(deleteRefButton);
    mainLayout->addLayout(refButtons);

    // hide all the delete widgets if there's nothing to delete!
    if (!allowDelete || context->rideItem()->ride()->referencePoints().count() == 0) {

        refsTable->hide();
        deleteRefButton->hide();

    } else {

        refreshTable();
    }

    // zapped
    connect(this, SIGNAL(rejected()), this, SLOT(closed()));
}

void
ReferenceLineDialog::deleteRef()
{
    // delete the ref that is highlighted
    QList<QTableWidgetItem*> selections = refsTable->selectedItems();

    if (selections.count()) {

        QTableWidgetItem *which = refsTable->selectedItems().first();
        int index = refsTable->row(which);

        // wipe
        context->rideItem()->ride()->removeReference(index);
        context->rideItem()->setDirty(true);

        refreshTable();

        // plot needs to refresh markers
        parent->refreshReferenceLinesForAllPlots();
        parent->plotTmpReference(axis, 0, 0, RideFile::watts); //unplot
    }
}

void
ReferenceLineDialog::setValueForAxis(double value, int axis)
{
    this->axis = axis;
    refValue->setText(QString("%1").arg((int)value));
}


void
ReferenceLineDialog::addClicked()
{
    RideFilePoint *refPoint = new RideFilePoint();

    if (serie == RideFile::watts)
        refPoint->watts = refValue->text().toDouble();
    else if (serie == RideFile::hr)
        refPoint->hr = refValue->text().toDouble();

    context->rideItem()->ride()->appendReference(*refPoint);
    context->rideItem()->setDirty(true);

    parent->refreshReferenceLinesForAllPlots();
    parent->plotTmpReference(axis, 0, 0, RideFile::watts); //unplot
    done(1);
}

void
ReferenceLineDialog::closed()
{
    parent->plotTmpReference(axis, 0, 0, RideFile::watts); //unplot
}

void
ReferenceLineDialog::refreshTable()
{
        int i=0;

        // reset the table
        refsTable->clear();
        QStringList header;
        header << tr("Series") << tr("Value"); 
        refsTable->setHorizontalHeaderLabels(header);
        refsTable->setRowCount(context->rideItem()->ride()->referencePoints().count());

        foreach(RideFilePoint *rp, context->rideItem()->ride()->referencePoints()) {

            // watts and hr only at this point
            QTableWidgetItem *t = new QTableWidgetItem();

            if (rp->watts > 0)
                t->setText(tr("Power"));
            else if (rp->hr > 0)
                t->setText(tr("Heart Rate"));

            t->setFlags(t->flags() & (~Qt::ItemIsEditable));
            refsTable->setItem(i,0,t);

            t = new QTableWidgetItem();

            if (rp->watts > 0)
                t->setText(QString("%1").arg(rp->watts));
            else if (rp->hr > 0)
               t->setText(QString("%1").arg(rp->hr));

            t->setFlags(t->flags() & (~Qt::ItemIsEditable));
            refsTable->setItem(i,1,t);

            i++;

        }
}
