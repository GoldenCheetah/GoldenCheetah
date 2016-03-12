/*
 * Copyright (c) Mark Liversedge (liversedge@gmail.com)
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

#include "SolveCPDialog.h"
#include "HelpWhatsThis.h"
#include "Settings.h"

#include "Context.h"
#include "Athlete.h"
#include "RideCache.h"
#include "RideItem.h"

#include "CPSolver.h"

#include <QFont>
#include <QFontMetrics>

SolveCPDialog::SolveCPDialog(QWidget *parent, Context *context) : QDialog(parent), context(context)
{
    setWindowTitle(tr("Critical Power Solver"));
    setAttribute(Qt::WA_DeleteOnClose);
    setMinimumSize(QSize(730, 370));

    //
    // Widget creation
    //
    solver = new CPSolver(context);

    QFont bolden;
    bolden.setWeight(QFont::Bold);

    dataLabel = new QLabel(tr("Activity Selection"), this);
    dataLabel->setAlignment(Qt::AlignHCenter);
    dataLabel->setFont(bolden);
    selectCheckBox = new QCheckBox(tr("Select/Deselect All"), this);

    // list all the activities that contain exhaustion points
    dataTable = new QTreeWidget(this);

    progressLabel = new QLabel(tr("Progress"), this);
    progressLabel->setAlignment(Qt::AlignHCenter);
    progressLabel->setFont(bolden);

    // headings
    itLabel = new QLabel(tr("Iteration"));
    cpLabel = new QLabel(tr("CP"));
    wLabel = new QLabel(tr("W'"));
    tLabel = new QLabel(tr("tau"));
    sumLabel = new QLabel("ΣW'bal²");

    currentLabel = new QLabel(tr("Current"));
    bestLabel = new QLabel(tr("Best"));

    // current value
    citLabel = new QLabel(tr(""));
    ccpLabel = new QLabel(tr(""));
    cwLabel = new QLabel(tr(""));
    ctLabel = new QLabel(tr(""));
    csumLabel = new QLabel("");

    // best so far
    bitLabel = new QLabel(tr(""));
    bcpLabel = new QLabel(tr(""));
    bwLabel = new QLabel(tr(""));
    btLabel = new QLabel(tr(""));
    bsumLabel = new QLabel("");

    // fix the label heights and alignment
    QFont def;
    QFontMetrics fm(def);
    currentLabel->setFixedHeight(fm.height());
    bestLabel->setFixedHeight(fm.height());
    itLabel->setFixedHeight(fm.height());
    cpLabel->setFixedHeight(fm.height());
    wLabel->setFixedHeight(fm.height());
    tLabel->setFixedHeight(fm.height());
    sumLabel->setFixedHeight(fm.height());
    citLabel->setFixedHeight(fm.height());
    ccpLabel->setFixedHeight(fm.height());
    cwLabel->setFixedHeight(fm.height());
    ctLabel->setFixedHeight(fm.height());
    csumLabel->setFixedHeight(fm.height());
    bitLabel->setFixedHeight(fm.height());
    bcpLabel->setFixedHeight(fm.height());
    bwLabel->setFixedHeight(fm.height());
    btLabel->setFixedHeight(fm.height());
    bsumLabel->setFixedHeight(fm.height());
    itLabel->setAlignment(Qt::AlignHCenter);
    cpLabel->setAlignment(Qt::AlignHCenter);
    wLabel->setAlignment(Qt::AlignHCenter);
    tLabel->setAlignment(Qt::AlignHCenter);
    sumLabel->setAlignment(Qt::AlignHCenter);
    citLabel->setAlignment(Qt::AlignHCenter);
    ccpLabel->setAlignment(Qt::AlignHCenter);
    cwLabel->setAlignment(Qt::AlignHCenter);
    ctLabel->setAlignment(Qt::AlignHCenter);
    csumLabel->setAlignment(Qt::AlignHCenter);
    bitLabel->setAlignment(Qt::AlignHCenter);
    bcpLabel->setAlignment(Qt::AlignHCenter);
    bwLabel->setAlignment(Qt::AlignHCenter);
    btLabel->setAlignment(Qt::AlignHCenter);
    bsumLabel->setAlignment(Qt::AlignHCenter);

    citLabel->setText("-");
    ccpLabel->setText("-");
    cwLabel->setText("-");
    ctLabel->setText("-");
    csumLabel->setText("-");
    bitLabel->setText("-");
    bcpLabel->setText("-");
    bwLabel->setText("-");
    btLabel->setText("-");
    bsumLabel->setText("-");

    // visualise
    solverDisplay = new QWidget(this);
    solverDisplay->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    solverDisplay->setBackgroundRole(QPalette::Light);

    solve = new QPushButton(tr("Solve"));
    close = new QPushButton(tr("Close"));

    //
    // Layout the widget
    //

    // main widget with buttons at the bottom
    QVBoxLayout *fullLayout = new QVBoxLayout(this);
    QHBoxLayout *mainLayout = new QHBoxLayout;
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    fullLayout->addLayout(mainLayout);
    fullLayout->addLayout(buttonLayout);

    // data on left, progress on the right
    QVBoxLayout *dataLayout = new QVBoxLayout;
    QVBoxLayout *progressLayout = new QVBoxLayout;
    mainLayout->addLayout(dataLayout);
    mainLayout->addLayout(progressLayout);
    mainLayout->setStretchFactor(dataLayout,10);
    mainLayout->setStretchFactor(progressLayout,15);

    // data layout on left
    dataLayout->addWidget(dataLabel);
    dataLayout->addWidget(selectCheckBox);
    dataLayout->addWidget(dataTable);

    // progress layout, labels then progress viz
    progressLayout->addWidget(progressLabel);
    QGridLayout *gridLayout = new QGridLayout;
    progressLayout->addLayout(gridLayout);
    progressLayout->addWidget(solverDisplay);

    // all the labels...
    gridLayout->addWidget(itLabel, 0, 1);
    gridLayout->addWidget(cpLabel, 0, 2);
    gridLayout->addWidget(wLabel, 0, 3);
    gridLayout->addWidget(tLabel, 0, 4);
    gridLayout->addWidget(sumLabel, 0, 5);

    gridLayout->addWidget(currentLabel, 1, 0);
    gridLayout->addWidget(citLabel, 1, 1);
    gridLayout->addWidget(ccpLabel, 1, 2);
    gridLayout->addWidget(cwLabel, 1, 3);
    gridLayout->addWidget(ctLabel, 1, 4);
    gridLayout->addWidget(csumLabel, 1, 5);

    gridLayout->addWidget(bestLabel, 2, 0);
    gridLayout->addWidget(bitLabel, 2, 1);
    gridLayout->addWidget(bcpLabel, 2, 2);
    gridLayout->addWidget(bwLabel, 2, 3);
    gridLayout->addWidget(btLabel, 2, 4);
    gridLayout->addWidget(bsumLabel, 2, 5);

    // buttons
    buttonLayout->addStretch();
    buttonLayout->addWidget(solve);
    buttonLayout->addWidget(close);

    //
    // Connect the dots
    //
    connect(selectCheckBox, SIGNAL(stateChanged(int)), this, SLOT(selectAll()));
    connect(solve, SIGNAL(clicked()), this, SLOT(solveClicked()));
    connect(close, SIGNAL(clicked()), this, SLOT(closeClicked()));
    connect(solver, SIGNAL(current(int,WBParms,double)), this, SLOT(current(int,WBParms,double)));
    connect(solver, SIGNAL(newBest(int,WBParms,double)), this, SLOT(newBest(int,WBParms,double)));

    //
    // Prepare
    //
    QStringList headers;
    headers << " " << "Date" << "Min W'bal" << "Solved";
    dataTable->setColumnCount(4);
    dataTable->setHeaderLabels(headers);
    dataTable->headerItem()->setTextAlignment(0, Qt::AlignLeft);
    dataTable->headerItem()->setTextAlignment(1, Qt::AlignHCenter);
    dataTable->headerItem()->setTextAlignment(2, Qt::AlignHCenter);
    dataTable->headerItem()->setTextAlignment(3, Qt::AlignHCenter);

    // get a list
    foreach(RideItem *item, context->athlete->rideCache->rides()) {
        int te = item->getForSymbol("ride_te");
        if (te) items<<item;
    }

    // most recent first !
    for (int k=items.count()-1; k>=0; k--) {

        RideItem *item = items[k];

        // we have one
        QTreeWidgetItem *t = new QTreeWidgetItem(dataTable);
        t->setText(1, item->dateTime.date().toString("dd MMM yy"));
        t->setText(2, item->getStringForSymbol("skiba_wprime_low"));
        t->setText(3, "-");

        t->setTextAlignment(0, Qt::AlignLeft);
        t->setTextAlignment(1, Qt::AlignHCenter);
        t->setTextAlignment(2, Qt::AlignHCenter);
        t->setTextAlignment(3, Qt::AlignHCenter);

        // remember which rideitem this is for
        t->setData(0, Qt::UserRole, qVariantFromValue(static_cast<void*>(item)));

        // checkbox
        QCheckBox *check = new QCheckBox(this);
        dataTable->setItemWidget(t, 0, check);
    }

    dataTable->setColumnWidth(0,50);
    dataTable->resizeColumnToContents(1);
    dataTable->resizeColumnToContents(2);
    dataTable->resizeColumnToContents(3);
#ifdef Q_OS_MAC
    dataTable->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
}

void
SolveCPDialog::selectAll()
{
    for (int i=0; i<dataTable->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *it = const_cast<QTreeWidgetItem*>(dataTable->invisibleRootItem()->child(i));
        QCheckBox *check = static_cast<QCheckBox*>(dataTable->itemWidget(it,0));
        check->setChecked(selectCheckBox->isChecked());
    }
}

void
SolveCPDialog::newBest(int k,WBParms p,double sum)
{
    bitLabel->setText(QString("%1").arg(k));
    bcpLabel->setText(QString("%1").arg(p.CP));
    bwLabel->setText(QString("%1").arg(p.W));
    btLabel->setText(QString("%1").arg(p.TAU));
    bsumLabel->setText(QString("%1").arg(sum, 0, 'f', 3));

    QApplication::processEvents();
}


void
SolveCPDialog::current(int k,WBParms p,double sum)
{
    citLabel->setText(QString("%1").arg(k));
    ccpLabel->setText(QString("%1").arg(p.CP));
    cwLabel->setText(QString("%1").arg(p.W));
    ctLabel->setText(QString("%1").arg(p.TAU));
    csumLabel->setText(QString("%1").arg(sum, 0, 'f', 3));

    if (!(k++%50) || k == 99999)  QApplication::processEvents();
}


void
SolveCPDialog::solveClicked()
{
    if (solve->text() == tr("Stop")) {
        solve->setText(tr("Solve"));
        solver->stop();
        return;
    }

    // loop through the table and collect the rides to solve
    QList<RideItem*> solveme;

    // get the rides selected
    for(int i=0; i<dataTable->invisibleRootItem()->childCount(); i++) {

        QTreeWidgetItem *it = dataTable->invisibleRootItem()->child(i);


        // is it selected?
        if (static_cast<QCheckBox*>(dataTable->itemWidget(it, 0))->isChecked())
            solveme << static_cast<RideItem*>(it->data(0, Qt::UserRole).value<void *>());
    }

    // if we got something to solve then kick it off
    if (solveme.count()) {

        // reset and reinitialise before kicking off
        solver->reset();
        solver->setData(solveme);
        solve->setText(tr("Stop"));
        solver->start();
    }
    return;
}

void
SolveCPDialog::closeClicked()
{
    solver->stop();// in case its still running!
    accept();
}

