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

void ToolsDialog::setupUi(QDialog *ToolsDialog)
{
    if (ToolsDialog->objectName().isEmpty())
        ToolsDialog->setObjectName(QString::fromUtf8("ToolsDialog"));
    ToolsDialog->setWindowModality(Qt::WindowModal);
    ToolsDialog->setAcceptDrops(false);
    ToolsDialog->setModal(true);

    QGridLayout *mainGrid = new QGridLayout(this); // a 2 x n grid
    lblBest3Min = new QLabel("Your Best 3 Minutes:", this);
    lblBest3Min->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    txtBest3Min = new QLineEdit(this);
    txtBest3Min->setInputMask("999");
    lblBest20Min = new QLabel("Your Best 20 Minutes:", this);
    lblBest20Min->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    txtBest20Min = new QLineEdit(this);
    txtBest20Min->setInputMask("999");
    lblCP = new QLabel(this);
    lblCP->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    lblCP->setText(QString("Critical Power:"));
    txtCP = new QLineEdit(this);
    txtCP->setEnabled(false);

    btnOK = new QPushButton(this);
    btnCalculate = new QPushButton(this);
    mainGrid->addWidget(lblBest3Min, 0, 0);
    mainGrid->addWidget(lblBest3Min, 0, 0);
    mainGrid->addWidget(txtBest3Min, 0, 1);

    mainGrid->addWidget(lblBest20Min, 1, 0);
    mainGrid->addWidget(txtBest20Min, 1, 1);

    mainGrid->addWidget(lblCP, 2, 0);
    mainGrid->addWidget(txtCP, 2, 1);

    mainGrid->addWidget(btnCalculate, 3, 0);
    mainGrid->addWidget(btnOK, 3, 1);




    ToolsDialog->setWindowTitle(
        QApplication::translate("ToolsDialog", "Critical Power Calculator", 0,
                                QApplication::UnicodeUTF8));

    btnOK->setText(
        QApplication::translate("ToolsDialog", "OK", 0,
                                QApplication::UnicodeUTF8));
    btnCalculate->setText(
        QApplication::translate("ToolsDialog", "Calculate", 0,
                                QApplication::UnicodeUTF8));

    connect(btnOK, SIGNAL(clicked()), this, SLOT(on_btnOK_clicked()));
    connect(btnCalculate, SIGNAL(clicked()), this, SLOT(on_btnCalculate_clicked()));

    Q_UNUSED(ToolsDialog);
}

ToolsDialog::ToolsDialog(QWidget *parent)
        : QDialog(parent)
{
    setupUi(this);
}

void ToolsDialog::on_btnOK_clicked()
{
    accept();
}

void ToolsDialog::on_btnCalculate_clicked()
{
    int CP = (txtBest20Min->text().toInt() * 20 * 60 - (txtBest3Min->text().toInt() * 3 * 60)) / (20 * 60 - 3 * 60);
    QString strCP;
    txtCP->setText(strCP.setNum(CP));

}

