/*
 * Copyright (c) 2007 Justin F. Knotzke (jknotzke@shampoo.ca)
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

#include "ToolsDialog.h"
#include <QtGui>

typedef QDoubleSpinBox* QDoubleSpinBoxPtr;

static
QHBoxLayout *setupMinsSecs(ToolsDialog *dialog,
                           QDoubleSpinBoxPtr &minsSpinBox,
                           QDoubleSpinBoxPtr &secsSpinBox,
                           QDoubleSpinBoxPtr &wattsSpinBox,
                           double maxMin, double defaultMin)
{
    QHBoxLayout *result = new QHBoxLayout;
    minsSpinBox = new QDoubleSpinBox(dialog);
    minsSpinBox->setDecimals(0);
    minsSpinBox->setRange(0.0, maxMin);
    minsSpinBox->setSuffix(" mins");
    minsSpinBox->setSingleStep(1.0);
    minsSpinBox->setWrapping(false);
    minsSpinBox->setAlignment(Qt::AlignRight);
    minsSpinBox->setValue(defaultMin);
    result->addWidget(minsSpinBox);
    secsSpinBox = new QDoubleSpinBox(dialog);
    secsSpinBox->setDecimals(0);
    secsSpinBox->setRange(0.0, 59.0);
    secsSpinBox->setSuffix(" secs");
    secsSpinBox->setSingleStep(1.0);
    secsSpinBox->setWrapping(true);
    secsSpinBox->setAlignment(Qt::AlignRight);
    result->addWidget(secsSpinBox);
    wattsSpinBox = new QDoubleSpinBox(dialog);
    wattsSpinBox->setDecimals(0);
    wattsSpinBox->setRange(0.0, 3000.0);
    wattsSpinBox->setSuffix(" watts");
    wattsSpinBox->setSingleStep(1.0);
    wattsSpinBox->setWrapping(false);
    wattsSpinBox->setAlignment(Qt::AlignRight);
    result->addWidget(wattsSpinBox);
    return result;
}

ToolsDialog::ToolsDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Critical Power Estimator"));
    setAttribute(Qt::WA_DeleteOnClose);

    setFixedSize(300, 240);

    QVBoxLayout *mainVBox = new QVBoxLayout(this);

    mainVBox->addWidget(new QLabel(tr("Your best short effort (3-5 min):")));
    mainVBox->addLayout(setupMinsSecs(this, shortMinsSpinBox, shortSecsSpinBox,
                                      shortWattsSpinBox, 5.0, 3.0));

    mainVBox->addWidget(new QLabel(tr("Your best long effort (15-60 min):")));
    mainVBox->addLayout(setupMinsSecs(this, longMinsSpinBox, longSecsSpinBox,
                                      longWattsSpinBox, 60.0, 20.0));
    mainVBox->addStretch();

    QHBoxLayout *cpHBox = new QHBoxLayout;
    cpHBox->addStretch();
    cpHBox->addWidget(new QLabel(tr("Your critical power:")));
    txtCP = new QLineEdit(this);
    txtCP->setAlignment(Qt::AlignRight);
    txtCP->setReadOnly(true);
    cpHBox->addWidget(txtCP, Qt::AlignLeft);
    cpHBox->addStretch();
    mainVBox->addLayout(cpHBox);

    QHBoxLayout *wpHBox = new QHBoxLayout;
    wpHBox->addStretch();
    wpHBox->addWidget(new QLabel(tr("Your W':")));
    txtWP = new QLineEdit(this);
    txtWP->setAlignment(Qt::AlignRight);
    txtWP->setReadOnly(true);
    wpHBox->addWidget(txtWP, Qt::AlignLeft);
    wpHBox->addStretch();
    mainVBox->addLayout(wpHBox);

    mainVBox->addStretch();

    QHBoxLayout *buttonHBox = new QHBoxLayout;
    btnCalculate = new QPushButton(this);
    btnCalculate->setText(tr("Estimate CP"));
    buttonHBox->addStretch();
    buttonHBox->addWidget(btnCalculate);
    btnOK = new QPushButton(this);
    btnOK->setText(tr("Done"));
    buttonHBox->addWidget(btnOK);
    mainVBox->addLayout(buttonHBox);

    connect(btnOK, SIGNAL(clicked()), this, SLOT(on_btnOK_clicked()));
    connect(btnCalculate, SIGNAL(clicked()), this, SLOT(on_btnCalculate_clicked()));
}

void ToolsDialog::on_btnOK_clicked()
{
    accept();
}

void ToolsDialog::on_btnCalculate_clicked()
{
    double shortSecs =
        shortMinsSpinBox->value() * 60.0 + shortSecsSpinBox->value();
    double shortWatts = shortWattsSpinBox->value();
    double longSecs =
        longMinsSpinBox->value() * 60.0 + longSecsSpinBox->value();
    double longWatts = longWattsSpinBox->value();
    double CP =
        (longSecs * longWatts - shortSecs * shortWatts)
        / (longSecs - shortSecs);
    txtCP->setText(QString("%1 watts").arg(static_cast<int>(round(CP))));

    double Wprime = ((shortSecs * (shortWatts-CP)) +
                    (longSecs * (longWatts-CP))) /2;
    txtWP->setText(QString("%1 kJ").arg(static_cast<int>(round(Wprime/1000))));
}

