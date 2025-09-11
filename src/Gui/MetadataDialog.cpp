
/*
 * Metadata Dialog Copyright (c) 2024 Paul Johnson (paulj49457@gmail.com)
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

#include "MetadataDialog.h"
#include "Context.h"
#include "Athlete.h"
#include "RideCache.h"
#include "RideMetadata.h"

#include <QFormLayout>

MetadataDialog::MetadataDialog(Context* context, const QString& fieldName, const QString& value, QPoint pos) :
    QDialog(context->mainWindow), context_(context), completer_(nullptr), pos_(pos)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("Metadata Editor"));

    // Find the metadata field defintion for this tile
    foreach(FieldDefinition field, GlobalContext::context()->rideMetadata->getFields()) {
        if (field.name == fieldName) {
            field_ = field;
            break;
        }
    }

    metaLabel_ = new QLabel(field_.name);

    switch (field_.type) {

    case GcFieldType::FIELD_TEXT: // text
    case GcFieldType::FIELD_SHORTTEXT: { // shorttext
        metaEdit_ = new QLineEdit(this);
        completer_ = field_.getCompleter(this, context_->athlete->rideCache);
        if (completer_) ((QLineEdit*)metaEdit_)->setCompleter(completer_);
        ((QLineEdit*)metaEdit_)->setMinimumWidth(225);
        ((QLineEdit*)metaEdit_)->setText(value);
        } break;

    case GcFieldType::FIELD_TEXTBOX: { // textbox
        metaEdit_ = new GTextEdit(this);
        // use special style sheet ..
        ((GTextEdit*)metaEdit_)->setObjectName("metadata");
        // rich text hangs 'fontd' for some users
        ((GTextEdit*)metaEdit_)->setAcceptRichText(false);
        ((GTextEdit*)metaEdit_)->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        ((GTextEdit*)metaEdit_)->setMinimumSize(QSize(300, 100));
        ((GTextEdit*)metaEdit_)->setText(value);
        } break;

    case GcFieldType::FIELD_INTEGER: { // integer
        metaEdit_ = new QSpinBox(this);
        ((QSpinBox*)metaEdit_)->setMinimum(-9999999);
        ((QSpinBox*)metaEdit_)->setMaximum(9999999);
        ((QSpinBox*)metaEdit_)->setButtonSymbols(QAbstractSpinBox::NoButtons);
        ((QSpinBox*)metaEdit_)->setMinimumWidth(150);
        ((QSpinBox*)metaEdit_)->setValue(value.toInt());
        } break;

    case GcFieldType::FIELD_DOUBLE: { // double
        metaEdit_ = new QDoubleSpinBox(this);
        ((QDoubleSpinBox*)metaEdit_)->setButtonSymbols(QAbstractSpinBox::NoButtons);
        ((QDoubleSpinBox*)metaEdit_)->setMinimum(-9999999.99);
        ((QDoubleSpinBox*)metaEdit_)->setMaximum(9999999.99);
        ((QDoubleSpinBox*)metaEdit_)->setMinimumWidth(150);
        ((QDoubleSpinBox*)metaEdit_)->setValue(value.toDouble());
        } break;

    case GcFieldType::FIELD_DATE: { // date
        metaEdit_ = new QDateEdit(this);
        ((QDateEdit*)metaEdit_)->setButtonSymbols(QAbstractSpinBox::NoButtons);
        ((QDateEdit*)metaEdit_)->setDisplayFormat("dd/MM/yyyy");
        ((QDateEdit*)metaEdit_)->setCalendarPopup(true);
        ((QDateEdit*)metaEdit_)->setMinimumWidth(130);
        if (value == "") {
            ((QDateEdit*)metaEdit_)->setDate(GC_MIN_EDIT_DATE);
        } else {
            QDate date(/* year*/value.mid(6, 4).toInt(),
                       /* month */value.mid(3, 2).toInt(),
                       /* day */value.mid(0, 2).toInt());
            ((QDateEdit*)metaEdit_)->setDate(date);
        } } break;

    case GcFieldType::FIELD_TIME: { // time
        metaEdit_ = new QTimeEdit(this);
        ((QTimeEdit*)metaEdit_)->setButtonSymbols(QAbstractSpinBox::NoButtons);
        ((QTimeEdit*)metaEdit_)->setDisplayFormat("hh:mm:ss");
        ((QTimeEdit*)metaEdit_)->setMinimumWidth(100);
        if (value == "") {
            ((QTimeEdit*)metaEdit_)->setTime(QTime(0,0,0));
        } else {
            QTime time(/* hours*/ value.mid(0, 2).toInt(),
                       /* minutes */ value.mid(3, 2).toInt(),
                       /* seconds */ value.mid(6, 2).toInt());
            ((QTimeEdit*)metaEdit_)->setTime(time);
        } } break;

    case GcFieldType::FIELD_CHECKBOX: { // check
        metaEdit_ = new QCheckBox(this);
        ((QCheckBox*)metaEdit_)->setChecked((value == "1") ? true : false);
        } break;
    default:
        qDebug() << "Metadata field type not set"; break;
    }

    // Metadata
    QHBoxLayout* metaDataLayout = new QHBoxLayout();
    metaDataLayout->setContentsMargins(0, 0, 0, 0);
    metaDataLayout->addSpacing(5);
    metaDataLayout->addWidget(metaLabel_);
    metaDataLayout->addSpacing(5);
    metaDataLayout->addWidget(metaEdit_,10);
    metaDataLayout->addStretch();

    // buttons
    QHBoxLayout* buttons = new QHBoxLayout;
    QPushButton* cancel = new QPushButton(tr("Cancel"), this);
    QPushButton* ok = new QPushButton(tr("Ok"), this);
    buttons->addStretch(75);
    buttons->addWidget(cancel);
    buttons->addWidget(ok);

    // Layout
    QFormLayout* layout = new QFormLayout(this);
    layout->addRow(metaDataLayout);
    layout->addRow(buttons);

    adjustSize(); // Window to contents

    connect(ok, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
}

MetadataDialog::~MetadataDialog()
{
    delete metaLabel_;
    delete metaEdit_; // QWidget destructor is virtual
    if (completer_) delete completer_;
}

void
MetadataDialog::showEvent(QShowEvent*)
{

    QSize gcWindowSize = context_->mainWindow->size();
    QPoint gcWindowPosn = context_->mainWindow->pos();

    int xLimit = gcWindowPosn.x() + gcWindowSize.width() - geometry().width() - 10;
    int yLimit = gcWindowPosn.y() + gcWindowSize.height() - geometry().height() - 10;

    int xDialog = (pos_.x() > xLimit) ? xLimit : pos_.x();
    int yDialog = (pos_.y() > yLimit) ? yLimit : pos_.y();

    move(xDialog, yDialog);
}

void
MetadataDialog::okClicked()
{
    // get values from the metaEdit_
    QString text;

    // get the current value into a string
    switch (field_.type) {
    case GcFieldType::FIELD_TEXT:
    case GcFieldType::FIELD_SHORTTEXT: text = ((QLineEdit*)metaEdit_)->text(); break;
    case GcFieldType::FIELD_TEXTBOX: text = ((GTextEdit*)metaEdit_)->document()->toPlainText(); break;
    case GcFieldType::FIELD_INTEGER: text = QString("%1").arg(((QSpinBox*)metaEdit_)->value()); break;
    case GcFieldType::FIELD_DOUBLE: text = QString("%1").arg(((QDoubleSpinBox*)metaEdit_)->value()); break;
    case GcFieldType::FIELD_CHECKBOX: text = ((QCheckBox*)metaEdit_)->isChecked() ? "1" : "0"; break;
    case GcFieldType::FIELD_DATE:
        if (((QDateEdit*)metaEdit_)->date().isValid()) {
            text = ((QDateEdit*)metaEdit_)->date().toString("dd/MM/yyyy");
        }
        break;
    case GcFieldType::FIELD_TIME:
        if (((QTimeEdit*)metaEdit_)->time().isValid()) {
            text = ((QTimeEdit*)metaEdit_)->time().toString("hh:mm:ss"); break;
        }
        break;
    default:
        qDebug() << "Metadata field type not set"; break;
    }

    RideItem* rideI = context_->rideItem();
    if (!rideI) { qDebug() << "rideI error in metadata popup"; return; }

    RideFile* rideF = rideI->ride();
    if (!rideF) { qDebug() << "rideF error in metadata popup"; return; }

    // Update special field
    if ((field_.name == "Device") && (rideF->deviceType() != text)) {

        // Update the device value in the ride file.
        rideF->setDeviceType(text);

        // rideFile is now dirty!
        rideI->setDirty(true);

        // refresh as state has changed
        rideI->notifyRideMetadataChanged();

    } else if (rideF->getTag(field_.name, "") != text) {
 
        // Update the metadata value in the ride file.
        rideF->setTag(field_.name, text);

        // rideFile is now dirty!
        rideI->setDirty(true);

        // refresh as state has changed
        rideI->notifyRideMetadataChanged();
    }

    done(QDialog::Accepted); // our work is done!
}

void
MetadataDialog::cancelClicked()
{
    done(QDialog::Rejected); // no work to do
}
