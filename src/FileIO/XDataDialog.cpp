/*
 * Copyright (c) 2016 Mark Liversedge <liversedge@gmail.com>
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

#include "XDataDialog.h"
#include "ActionButtonBox.h"
#include "RideItem.h"
#include "RideFile.h"
#include "RideFileCommand.h"
#include "Colors.h"

#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>

///
/// XDataDialog
///
XDataDialog::XDataDialog(QWidget *parent) : QDialog(parent), item(NULL)
{

    setWindowTitle("XData");
    setMinimumWidth(300*dpiXFactor);
    setMinimumHeight(450*dpiXFactor);

    //setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint | Qt::Tool);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // create all the widgets
    QLabel *xlabel = new QLabel(tr("xData"));
    xdataTable = new QTableWidget(this);
#ifdef Q_OS_MAX
    xdataTable->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    xdataTable->setColumnCount(1);
    xdataTable->horizontalHeader()->setStretchLastSection(true);
    xdataTable->horizontalHeader()->hide();
    xdataTable->setSortingEnabled(false);
    xdataTable->verticalHeader()->hide();
    xdataTable->setShowGrid(false);
    xdataTable->setSelectionMode(QAbstractItemView::SingleSelection);
    xdataTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    QLabel *xslabel = new QLabel(tr("Data Series"));
    xdataSeriesTable = new QTableWidget(this);
#ifdef Q_OS_MAX
    xdataSeriesTable->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    xdataSeriesTable->setColumnCount(1);
    xdataSeriesTable->horizontalHeader()->setStretchLastSection(true);
    xdataSeriesTable->horizontalHeader()->hide();
    xdataSeriesTable->setSortingEnabled(false);
    xdataSeriesTable->verticalHeader()->hide();
    xdataSeriesTable->setShowGrid(false);
    xdataSeriesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    xdataSeriesTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    ActionButtonBox *xdataActionButtons = new ActionButtonBox(ActionButtonBox::AddDeleteGroup);

    ActionButtonBox *xdataSeriesActionButtons = new ActionButtonBox(ActionButtonBox::AddDeleteGroup);

    // lay it out
    mainLayout->addWidget(xlabel);
    mainLayout->addWidget(xdataTable);
    mainLayout->addWidget(xdataActionButtons);

    mainLayout->addWidget(xslabel);
    mainLayout->addWidget(xdataSeriesTable);
    mainLayout->addWidget(xdataSeriesActionButtons);

    connect(xdataTable, SIGNAL(currentItemChanged(QTableWidgetItem*,QTableWidgetItem*)), this, SLOT(xdataSelected()));
    xdataActionButtons->defaultConnect(xdataTable);
    connect(xdataActionButtons, &ActionButtonBox::addRequested, this, &XDataDialog::addXDataClicked);
    connect(xdataActionButtons, &ActionButtonBox::deleteRequested, this, &XDataDialog::removeXDataClicked);
    xdataSeriesActionButtons->defaultConnect(xdataSeriesTable);
    connect(xdataSeriesActionButtons, &ActionButtonBox::addRequested, this, &XDataDialog::addXDataSeriesClicked);
    connect(xdataSeriesActionButtons, &ActionButtonBox::deleteRequested, this, &XDataDialog::removeXDataSeriesClicked);
}

void XDataDialog::close()
{
}

void XDataDialog::setRideItem(RideItem *item)
{
    this->item = item;

    xdataTable->clear();
    xdataSeriesTable->clear();

    // add a row for each xdata
    if (item && item->ride() && item->ride()->xdata().count()) {

        QMapIterator<QString,XDataSeries *> it(item->ride()->xdata());
        it.toFront();
        int n=0;

        xdataTable->setRowCount(item->ride()->xdata().count());
        while(it.hasNext()) {
            it.next();

            QString name = it.key();
            QTableWidgetItem *add = new QTableWidgetItem();
            add->setFlags(add->flags() & (~Qt::ItemIsEditable));
            add->setText(name);

            xdataTable->setItem(n++, 0, add);
        }

        if (xdataTable->currentRow()==0) {
            xdataSelected();
        } else {
            xdataTable->selectRow(0);
        }
    }
}

void XDataDialog::xdataSelected()
{
    // update xdata series table to reflect the selection
    xdataSeriesTable->clear();

    // lets find the one we have selected...
    int n=0, index=xdataTable->currentIndex().row();
    const XDataSeries *series = NULL;

    QMapIterator<QString,XDataSeries *> it(item->ride()->xdata());
    it.toFront();

    xdataTable->setRowCount(item->ride()->xdata().count());
    while(it.hasNext()) {
        it.next();

        series= it.value();
        if (n++==index) break;
    }

    // lets populate
    if (series) {

        n=0;
        xdataSeriesTable->setRowCount(series->valuename.count());
        foreach(QString name, series->valuename) {

            // add a row for each name
            QTableWidgetItem *add = new QTableWidgetItem();
            add->setFlags(add->flags() & (~Qt::ItemIsEditable));
            add->setText(name);

            xdataSeriesTable->setItem(n++, 0, add);
        }
        if (n) xdataSeriesTable->selectRow(0);
    }
}

void
XDataDialog::removeXDataClicked()
{
    // pressed minus on an xdata so do via ridefilecommand
    if (item && item->ride() && xdataTable->currentIndex().row() >=0 && xdataTable->currentIndex().row() < xdataTable->rowCount()) {
        // lets zap it via the ride file command
        item->ride()->command->removeXData(xdataTable->item(xdataTable->currentIndex().row(),0)->text());

    }
}

void
XDataDialog::removeXDataSeriesClicked()
{
    QString xdata;
    if (item && item->ride() && xdataTable->currentIndex().row() >=0 && xdataTable->currentIndex().row() < xdataTable->rowCount()) {
        // lets zap it via the ride file command
        xdata = xdataTable->item(xdataTable->currentIndex().row(),0)->text();

    }
    // pressed minus on an xdata so do via ridefilecommand
    if (item && item->ride() && xdataSeriesTable->currentIndex().row() >=0 && xdataSeriesTable->currentIndex().row() < xdataSeriesTable->rowCount()) {
        // lets zap it via the ride file command
        item->ride()->command->removeXDataSeries(xdata, xdataSeriesTable->item(xdataSeriesTable->currentIndex().row(),0)->text());

    }
}

void
XDataDialog::addXDataClicked()
{
    XDataSeries add;
    XDataSettingsDialog *dialog = new XDataSettingsDialog(this, add);
    int ret = dialog->exec();

    if (ret==QDialog::Accepted) {
        item->ride()->command->addXData(new XDataSeries(add));
    }
}

void
XDataDialog::addXDataSeriesClicked()
{
    // lets find the one we have selected...
    int index=xdataTable->currentIndex().row();
    if (index <0) return;
    QString xdata = xdataTable->item(index,0)->text();
    const XDataSeries *series = item->ride()->xdata(xdata);
    if (series == NULL) return;

    QString name, unit;
    XDataSeriesSettingsDialog *dialog = new  XDataSeriesSettingsDialog(this, name, unit);
    int ret = dialog->exec();

    if (ret == QDialog::Accepted) {
        item->ride()->command->addXDataSeries(xdata, name, unit);
    }
}

///
/// XDataSettingsDialog
///
XDataSettingsDialog::XDataSettingsDialog(QWidget *parent, XDataSeries &series) : QDialog(parent), series(series)
{
    setWindowTitle("XData Settings");
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint | Qt::Tool);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QGridLayout *grid = new QGridLayout();
    mainLayout->addLayout(grid);


    QLabel *xdataLabel = new QLabel(tr("xData"), this);
    xdataName = new QLineEdit(this);

    grid->addWidget(xdataLabel, 0, 0);
    grid->addWidget(xdataName, 0, 1);
    grid->addWidget(new QLabel("",this), 1, 0);

    grid->addWidget(new QLabel(tr("Data Series"), this), 2, 0);
    grid->addWidget(new QLabel(tr("Units"), this), 2, 1);
    for (int i=0; i<8; i++) {
        xdataSeriesName[i] = new QLineEdit(this);
        xdataUnitName[i] = new QLineEdit(this);
        grid->addWidget(new QLabel(QString(tr("Series %1")).arg(i+1)), 3+i, 0);
        grid->addWidget(xdataSeriesName[i], 3+i, 1);
        grid->addWidget(xdataUnitName[i], 3+i, 2);
    }
    grid->addWidget(new QLabel("",this), 11, 0);
    mainLayout->addStretch();

    cancelButton = new QPushButton(tr("Cancel"), this);
    okButton = new QPushButton(tr("OK"), this);
    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addStretch();
    buttons->addWidget(cancelButton);
    buttons->addWidget(okButton);
    mainLayout->addLayout(buttons);

    connect(okButton, SIGNAL(clicked(bool)), this, SLOT(okClicked()));
    connect(cancelButton, SIGNAL(clicked(bool)), this, SLOT(reject()));
}

void XDataSettingsDialog::okClicked()
{
    // lets just check we have something etc
    if (xdataName->text() == "") {
        QMessageBox::warning(0, tr("Error"), tr("XData name is blank"));
        return;
    } else {
        series.name = xdataName->text();
    }

    series.valuename.clear();
    for(int i=0; i<8; i++) {
        if (xdataSeriesName[i]->text() != "") {
            series.valuename << xdataSeriesName[i]->text();
            series.unitname << xdataUnitName[i]->text();
        }
    }

    if (series.valuename.count() >0) accept();
    else {
        QMessageBox::warning(0, tr("Error"), tr("Must have at least one data series."));
        return;
    }
}


///
/// XDataSeriesSettingsDialog
///
XDataSeriesSettingsDialog::XDataSeriesSettingsDialog(QWidget *parent, QString &name, QString &unit) : QDialog(parent), name(name), unit(unit)
{
    setWindowTitle("XData Series Name");
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint | Qt::Tool);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QFormLayout *form = new QFormLayout();
    mainLayout->addLayout(form);


    QLabel *nameLabel = new QLabel(tr("Name"), this);
    nameEdit = new QLineEdit(this);
    nameEdit->setText(name);
    form->addRow(nameLabel, nameEdit);

    QLabel *unitLabel = new QLabel(tr("Units"), this);
    unitEdit = new QLineEdit(this);
    unitEdit->setText(unit);
    form->addRow(unitLabel, unitEdit);

    form->addRow(new QLabel("",this), new QLabel("", this));
    mainLayout->addStretch();

    cancelButton = new QPushButton(tr("Cancel"), this);
    okButton = new QPushButton(tr("OK"), this);
    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addStretch();
    buttons->addWidget(cancelButton);
    buttons->addWidget(okButton);
    mainLayout->addLayout(buttons);

    connect(okButton, SIGNAL(clicked(bool)), this, SLOT(okClicked()));
    connect(cancelButton, SIGNAL(clicked(bool)), this, SLOT(reject()));
}

void XDataSeriesSettingsDialog::okClicked()
{
    // lets just check we have something etc
    if (nameEdit->text() == "") {
        QMessageBox::warning(0, tr("Error"), tr("Name is blank"));
        return;
    } else {
        name = nameEdit->text();
        unit = unitEdit->text();
    }

    accept();
}
