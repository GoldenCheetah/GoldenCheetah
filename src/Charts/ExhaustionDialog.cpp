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

#include "ExhaustionDialog.h"
#include "Athlete.h"
#include "Context.h"
#include "RideItem.h"
#include "TimeUtils.h" // time_to_string

ExhaustionDialog::ExhaustionDialog(AllPlot *parent, Context *context, double secs, bool allowDelete) :
    parent(parent), context(context), secs(secs), allowDelete(allowDelete)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::Tool);
    setWindowTitle(tr("Exhaustion / Failure"));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *referenceValueLayout = new QHBoxLayout;
    referenceValueLayout->setSpacing(0);
    referenceValueLayout->setContentsMargins(0,0,0,0);
    QLabel *refLabel = new QLabel();
    refLabel->setText(tr("Exhausted:"));
    refValue = new QLineEdit();
    refUnit = new QLabel();
    refUnit->setText(tr("secs"));
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
    refsTable->setColumnCount(1);
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
ExhaustionDialog::deleteRef()
{

    // delete the ref that is highlighted
    QList<QTableWidgetItem*> selections = refsTable->selectedItems();

    if (selections.count()) {

        QTableWidgetItem *which = refsTable->selectedItems().first();
        int index = refsTable->row(which);

        // wipe
        context->rideItem()->ride()->removeExhaustion(index);
        context->rideItem()->setDirty(true);

        refreshTable();

        // plot needs to refresh markers
        parent->refreshExhaustionsForAllPlots();
        parent->plotTmpExhaustion(0); //unplot
    }

}

void
ExhaustionDialog::setValue(double value)
{
    refValue->setText(QString("%1").arg((int)value));
}


void
ExhaustionDialog::addClicked()
{

    RideFilePoint *refPoint = new RideFilePoint();

    refPoint->secs = refValue->text().toDouble();

    context->rideItem()->ride()->appendReference(*refPoint);
    context->rideItem()->setDirty(true);

    parent->refreshExhaustionsForAllPlots();
    parent->plotTmpExhaustion(0); //unplot

    done(1);
}

void
ExhaustionDialog::closed()
{
    parent->plotTmpExhaustion(0); //unplot
}

void
ExhaustionDialog::refreshTable()
{
    int i=0;

    // reset the table
    refsTable->clear();
    QStringList header;
    header << tr("Time");
    refsTable->setHorizontalHeaderLabels(header);

    foreach(RideFilePoint *rp, context->rideItem()->ride()->referencePoints())
        if (rp->secs > 0) i++;

    if (i>0) {

        refsTable->setRowCount(i);

        i=0; // start again
        foreach(RideFilePoint *rp, context->rideItem()->ride()->referencePoints()) {

            if (rp->secs > 0) {

                // time at which we hit task failure / exhaustion
                QTableWidgetItem *t = new QTableWidgetItem();
                t->setText(QString("%1").arg(time_to_string(rp->secs)));
                t->setFlags(t->flags() & (~Qt::ItemIsEditable));

                refsTable->setItem(i++,0,t);
            }
        }
    }
}
