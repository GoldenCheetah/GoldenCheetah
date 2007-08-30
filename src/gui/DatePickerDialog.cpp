//
// Author: Justin F. Knotzke <jknotzke@shampoo.ca>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//

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

