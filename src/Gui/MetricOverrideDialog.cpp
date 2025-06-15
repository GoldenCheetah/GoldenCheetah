
/*
 * Metric Override Dialog Copyright (c) 2025 Paul Johnson (paulj49457@gmail.com)
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

#include "MetricOverrideDialog.h"
#include "Context.h"
#include "RideCache.h"
#include "RideMetric.h"

#include <QFormLayout>
#include <QDebug>

MetricOverrideDialog::MetricOverrideDialog(Context* context, const QString& fieldName, const double value, QPoint pos) :
    QDialog(context->mainWindow), context_(context), fieldName_(fieldName), pos_(pos)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("Metric Override Editor"));

    QString units = GlobalContext::context()->specialFields.rideMetric(fieldName_)->units(GlobalContext::context()->useMetricUnits);

    if (GlobalContext::context()->specialFields.rideMetric(fieldName_)->isDate()) {
        dlgMetricType_ = DialogMetricType::DATE;
    } else if (units == "seconds" || units == tr("seconds")) {
        dlgMetricType_ = DialogMetricType::SECS_TIME;
    } else if (units == "minutes" || units == tr("minutes")) {
        dlgMetricType_ = DialogMetricType::MINS_TIME;
    } else {
        dlgMetricType_ = DialogMetricType::DOUBLE;
    }

    // if metric is a time or date remove any units, since the field will be a QTimeEdit or QDateEdit field
    units = (dlgMetricType_ == DialogMetricType::DOUBLE) ? QString(" (%1)").arg(units) : "";

    // we need to show what units we use for weight...
    if (fieldName_ == "Weight") {
        units = GlobalContext::context()->useMetricUnits ? tr(" (kg)") : tr(" (lbs)");
    }

    metricLabel_ = new QLabel(QString("%1%2").arg(GlobalContext::context()->specialFields.displayName(fieldName_)).arg(units));

    switch (dlgMetricType_) {
        case DialogMetricType::DATE: {
            metricEdit_ = new QDateEdit(this);
            dynamic_cast<QDateEdit*>(metricEdit_)->setDisplayFormat("dd MMM yyyy"); // same format as metric tile
            dynamic_cast<QDateEdit*>(metricEdit_)->setDate(QDate(1900,1,1).addDays(value));
        } break;

        case DialogMetricType::SECS_TIME: {
            metricEdit_ = new QTimeEdit(this);
            dynamic_cast<QTimeEdit*>(metricEdit_)->setDisplayFormat("h:mm:ss"); // same format as metric tile
            dynamic_cast<QTimeEdit*>(metricEdit_)->setTime(QTime(0,0,0,0).addSecs(value));
        } break;

        case DialogMetricType::MINS_TIME: {

            metricEdit_ = new QTimeEdit(this);
            dynamic_cast<QTimeEdit*>(metricEdit_)->setDisplayFormat("mm:ss"); // same format as metric tile
            dynamic_cast<QTimeEdit*>(metricEdit_)->setTime(QTime(0,0,0,0).addSecs(value*60));
        } break;

        case DialogMetricType::DOUBLE: {

            metricEdit_ = new QDoubleSpinBox(this);
            dynamic_cast<QDoubleSpinBox*>(metricEdit_)->setButtonSymbols(QAbstractSpinBox::NoButtons);
            dynamic_cast<QDoubleSpinBox*>(metricEdit_)->setMinimum(-9999999.99);
            dynamic_cast<QDoubleSpinBox*>(metricEdit_)->setMaximum(9999999.99);
            dynamic_cast<QDoubleSpinBox*>(metricEdit_)->setMinimumWidth(150);
            dynamic_cast<QDoubleSpinBox*>(metricEdit_)->setValue(value);
        } break;
        default: {
            qDebug() << "Unsupported type in metric override dialog";
        } break;
    }

    QHBoxLayout* metricDataLayout = new QHBoxLayout();
    metricDataLayout->setContentsMargins(0, 0, 0, 0);
    metricDataLayout->addSpacing(5);
    metricDataLayout->addWidget(metricLabel_);
    metricDataLayout->addSpacing(5);
    metricDataLayout->addWidget(metricEdit_,10);
    metricDataLayout->addStretch();

    // buttons
    QHBoxLayout* buttons = new QHBoxLayout;
    QPushButton* cancel = new QPushButton(tr("Cancel"), this);
    QPushButton* clear = new QPushButton(tr("Clear"), this);
    QPushButton* set = new QPushButton(tr("Set"), this);
    buttons->addStretch(75);
    buttons->addWidget(cancel);
    buttons->addWidget(clear);
    buttons->addWidget(set);

    // Layout
    QFormLayout* layout = new QFormLayout(this);
    layout->addRow(metricDataLayout);
    layout->addRow(buttons);

    adjustSize(); // Window to contents

    connect(set, SIGNAL(clicked()), this, SLOT(setClicked()));
    connect(clear, SIGNAL(clicked()), this, SLOT(clearClicked()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
}

MetricOverrideDialog::~MetricOverrideDialog()
{
    delete metricLabel_;
    delete metricEdit_; // QWidget destructor is virtual
}

void
MetricOverrideDialog::showEvent(QShowEvent*)
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
MetricOverrideDialog::setClicked()
{
    // get the current value from the metricEdit_ into a string
    QString text;
    switch (dlgMetricType_) {
        case DialogMetricType::DATE: {
            text = QString("%1").arg(QDate(1900, 01, 01).daysTo(dynamic_cast<QDateEdit*>(metricEdit_)->date()));
        } break;
        case DialogMetricType::SECS_TIME: {
            text = QString("%1").arg(QTime(0, 0, 0, 0).secsTo(dynamic_cast<QTimeEdit*>(metricEdit_)->time()));
        } break;
        case DialogMetricType::MINS_TIME: {
            text = QString("%1").arg((QTime(0, 0, 0, 0).secsTo(dynamic_cast<QTimeEdit*>(metricEdit_)->time())) / 60.0);
        } break;
        case DialogMetricType::DOUBLE: {
            text = QString("%1").arg(dynamic_cast<QDoubleSpinBox*>(metricEdit_)->value());

            // convert from imperial to metric if needed
            if (!GlobalContext::context()->useMetricUnits) {
                double value = text.toDouble() * (1 / GlobalContext::context()->specialFields.rideMetric(fieldName_)->conversion());
                value -= GlobalContext::context()->specialFields.rideMetric(fieldName_)->conversionSum();
                text = QString("%1").arg(value);
            }
        } break;
        default: {
            qDebug() << "Unsupported type in metric override dialog";
        } break;
    }

    // update metric override QMap!
    QMap<QString, QString> override;
    override.insert("value", text);

    // check for compatability metrics
    QString symbol = GlobalContext::context()->specialFields.metricSymbol(fieldName_);
    if (fieldName_ == "TSS") symbol = "coggan_tss";
    if (fieldName_ == "NP") symbol = "coggan_np";
    if (fieldName_ == "IF") symbol = "coggan_if";
    
    RideItem* rideI = context_->rideItem();
    if (!rideI) { qDebug() << "rideI error in metric override dialog"; done(QDialog::Rejected); return; }

    // Update the metadata value in the ride item.
    rideI->ride()->metricOverrides.insert(symbol, override);

    // rideItem is now dirty!
    rideI->setDirty(true);

    // refresh as state has changed
    rideI->notifyRideMetadataChanged();

    done(QDialog::Accepted); // our work is done!
}

void
MetricOverrideDialog::clearClicked() {

    QString symbol = GlobalContext::context()->specialFields.metricSymbol(fieldName_);
    if (fieldName_ == "TSS") symbol = "coggan_tss";
    if (fieldName_ == "NP") symbol = "coggan_np";
    if (fieldName_ == "IF") symbol = "coggan_if";

    RideItem* rideI = context_->rideItem();
    if (!rideI) { qDebug() << "rideI error in metric override dialog"; done(QDialog::Rejected); return; }

    if (rideI->ride()->metricOverrides.contains(symbol)) {

        // remove existing override for this metric
        rideI->ride()->metricOverrides.remove(symbol);

        // rideItem is now dirty!
        rideI->setDirty(true);

        // get refresh done, coz overrides state has changed
        rideI->notifyRideMetadataChanged();

        done(QDialog::Accepted); // our work is done!

    } else {

        done(QDialog::Rejected); // our work is done!
    }
}

void
MetricOverrideDialog::cancelClicked()
{
    done(QDialog::Rejected); // no work to do
}
