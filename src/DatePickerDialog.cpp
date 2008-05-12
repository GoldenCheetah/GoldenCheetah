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

#include "DatePickerDialog.h"
#include <QtGui>
    
void DatePickerDialog::setupUi(QDialog *DatePickerDialog)
{
    if (DatePickerDialog->objectName().isEmpty())
        DatePickerDialog->setObjectName(QString::fromUtf8("DatePickerDialog"));
    DatePickerDialog->setWindowModality(Qt::WindowModal);
    DatePickerDialog->setAcceptDrops(false);
    DatePickerDialog->setModal(true);

    QGridLayout *mainGrid = new QGridLayout(this); // a 2 x n grid
    lblOccur = new QLabel("When did this ride occur?", this);
    mainGrid->addWidget(lblOccur, 0,0);
    dateTimeEdit = new QDateTimeEdit(this);
    mainGrid->addWidget(dateTimeEdit,0,1);
    lblBrowse = new QLabel("Choose a CSV file to upload", this);
    mainGrid->addWidget(lblBrowse, 1,0);
    btnBrowse = new QPushButton(this);
    mainGrid->addWidget(btnBrowse,2,0);
    txtBrowse = new QLineEdit(this);
    mainGrid->addWidget(txtBrowse,2,1);

    btnOK = new QPushButton(this);
    mainGrid->addWidget(btnOK, 3,0);
    btnCancel = new QPushButton(this);
    mainGrid->addWidget(btnCancel, 3,1);

    DatePickerDialog->setWindowTitle(
        QApplication::translate("DatePickerDialog", "Choose a date", 0, 
                                QApplication::UnicodeUTF8));

    btnBrowse->setText(
        QApplication::translate("DatePickerDialog", "File to import...", 0, 
                                QApplication::UnicodeUTF8));
    btnOK->setText(
        QApplication::translate("DatePickerDialog", "OK", 0, 
                                QApplication::UnicodeUTF8));
    btnCancel->setText(
        QApplication::translate("DatePickerDialog", "Cancel", 0, 
                                QApplication::UnicodeUTF8));

    connect(btnOK, SIGNAL(clicked()), this, SLOT(on_btnOK_clicked()));
    connect(btnBrowse, SIGNAL(clicked()), this, SLOT(on_btnBrowse_clicked()));
    connect(btnCancel, SIGNAL(clicked()), this, SLOT(on_btnCancel_clicked()));

    Q_UNUSED(DatePickerDialog);
} 

DatePickerDialog::DatePickerDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
}

void DatePickerDialog::on_btnOK_clicked()
{
    canceled = false;
    date = dateTimeEdit->dateTime();
    accept();
}

void DatePickerDialog::on_btnBrowse_clicked()
{
    fileName = QFileDialog::getOpenFileName(
        this, tr("Import CSV"), QDir::homePath(),
        tr("Comma Seperated Values (*.csv)"));
    txtBrowse->setText(fileName);

}

void DatePickerDialog::on_btnCancel_clicked()
{
    canceled = true;
    reject();
}

