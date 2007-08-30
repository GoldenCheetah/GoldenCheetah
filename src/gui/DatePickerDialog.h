//
// C++ Interface: DatePickerDialog
//
// Description: 
//
//
// Author: Justin F. Knotzke <jknotzke@shampoo.ca>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include <QDateTime>
#include <QtGui>

class DatePickerDialog : public QDialog
{
    Q_OBJECT

public:
    DatePickerDialog(QWidget *parent = 0);
    QString fileName;
    QDateTime date;
    bool canceled;
    QString currentText;
    QLabel *lblOccur;
    QDateTimeEdit *dateTimeEdit;
    QHBoxLayout *hboxLayout1;
    QLabel *lblImport;
    QLabel *lblBrowse;
    QPushButton *btnBrowse;
    QLineEdit *txtBrowse;
    QPushButton *btnOK;
    QPushButton *btnCancel;
    void setupUi(QDialog*);

private slots:
    void on_btnOK_clicked();
    void on_btnBrowse_clicked();
    void on_btnCancel_clicked();
};

