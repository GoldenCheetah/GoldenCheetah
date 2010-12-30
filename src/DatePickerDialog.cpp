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
#include "Settings.h"
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

	// preset dialog to today's date  -thm
	QDateTime *dt = new QDateTime;
	date = dt->currentDateTime();
	dateTimeEdit->setDateTime(date);

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
        QApplication::translate("DatePickerDialog", "Import CSV file", 0,
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

	// disable date picker and OK button until a file has been selected
	dateTimeEdit->setEnabled(FALSE);
	lblOccur->setEnabled(FALSE);
	btnOK->setEnabled(FALSE);

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
    //First check to see if the Library folder exists where the executable is (for USB sticks)
    QVariant lastDirVar = appsettings->value(this, GC_SETTINGS_LAST_IMPORT_PATH);
    QString lastDir = (lastDirVar != QVariant())
        ? lastDirVar.toString() : QDir::homePath();
    fileName = QFileDialog::getOpenFileName(
        this, tr("Import CSV"), lastDir,
        tr("Comma Separated Values (*.csv)"));
    if (!fileName.isEmpty()) {
        lastDir = QFileInfo(fileName).absolutePath();
        appsettings->setValue(GC_SETTINGS_LAST_IMPORT_PATH, lastDir);

        // Find the datetimestamp from the filename.
        // If we can't, use the creation time.
        // eg. GoldenCheetah YYYY_MM_DD_HH_MM_SS.csv
        //     Ergomo YYYYMMDD_HHMMSS_NAME_SURNAME.CSV
        QFileInfo *qfi = new QFileInfo(fileName);
        QString name = qfi->baseName();
        QRegExp rxGoldenCheetah("^(19|20)\\d\\d_[01]\\d_[0123]\\d_[012]\\d_[012345]\\d_[012345]\\d$");
        QRegExp rxErgomo("^(19|20)\\d\\d[01]\\d[0123]\\d_[012]\\d[012345]\\d[012345]\\d_[A-Z_]+$");
        if (rxGoldenCheetah.indexIn(name) == 0) {
            date = QDateTime::fromString(name.left(19), "yyyy_MM_dd_hh_mm_ss");
        } else if (rxErgomo.indexIn(name) == 0) {
            date = QDateTime::fromString(name.left(15), "yyyyMMdd_hhmmss");
        } else {
            date = qfi->created();
        }
	// and put it into the datePicker dialog
	dateTimeEdit->setDateTime(date);

    }
    txtBrowse->setText(fileName);

	// allow date to be changed, and enable OK button
	dateTimeEdit->setEnabled(TRUE);
	lblOccur->setEnabled(TRUE);
	btnOK->setEnabled(TRUE);
}

void DatePickerDialog::on_btnCancel_clicked()
{
    canceled = true;
    reject();
}

