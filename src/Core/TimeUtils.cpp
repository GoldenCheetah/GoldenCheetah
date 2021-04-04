/*
 * Copyright (c) 2007 Sean C. Rhea (srhea@srhea.net)
 * Copyright (c) 2012 Mark Liversedge (liversedge@gmail.com)
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

#include "TimeUtils.h"
#include "Colors.h"
#include <cmath>
#include <QRegExpValidator>
#include <QFormLayout>
#include <QLabel>

QString time_to_string(double secs, bool forceMinutes)
{
    // negs are bad
    if (secs<0) secs=0;

    QString result;
    if (!forceMinutes && secs < 60) result = QString("%1").arg(secs); // special case for < 60s
    else{
        unsigned rounded = static_cast<unsigned>(round(secs));
        bool needs_colon = false;
        if (rounded >= 3600) {
            result += QString("%1").arg(rounded / 3600);
            rounded %= 3600;
            needs_colon = true;
        }
        if (needs_colon)
            result += ":";
        result += QString("%1").arg(rounded / 60, 2, 10, QLatin1Char('0'));
        rounded %= 60;
        result += ":";
        result += QString("%1").arg(rounded, 2, 10, QLatin1Char('0'));
    }
    return result;
}

QString time_to_string_for_sorting(double secs)
{
    // negs are bad
    if (secs<0) secs=0;

    QString result;
    if (secs < 60) result = QString("00:00:%1").arg(secs); // special case for < 60s
    else{
        unsigned rounded = static_cast<unsigned>(round(secs));
        if (rounded >= 3600) {
            result += QString("%1").arg(rounded / 3600, 2, 10, QLatin1Char('0'));
            rounded %= 3600;
            result += ":";
        } else {
            result += "00:";
        }
        result += QString("%1").arg(rounded / 60, 2, 10, QLatin1Char('0'));
        rounded %= 60;
        result += ":";
        result += QString("%1").arg(rounded, 2, 10, QLatin1Char('0'));
    }
    return result;
}

QString time_to_string_minutes(double secs)
{
    // negs are bad
    if (secs<0) secs=0;

    QString result;
    if (secs < 60) result = QString("00:00"); // special case for < 60s
    else{
        unsigned rounded = static_cast<unsigned>(round(secs));
        if (rounded >= 3600) {
            result += QString("%1").arg(rounded / 3600, 2, 10, QLatin1Char('0'));
            rounded %= 3600;
            result += ":";
        } else {
            result += "00:";
        }
        result += QString("%1").arg(rounded / 60, 2, 10, QLatin1Char('0'));
    }
    return result;
}

double str_to_interval(QString s)
{
    QRegExp rx("(\\d+\\s*h)?\\s*(\\d{1,2}\\s*m)?\\s*(\\d{1,2})(\\.\\d+)?\\s*s");
    rx.indexIn(s);
    QString hour = rx.cap(1);
    QString min = rx.cap(2);
    QString sec = rx.cap(3) + rx.cap(4);
    hour.chop(1);
    min.chop(1);
    return 3600.0 * hour.toUInt() + 60.0 * min.toUInt() + sec.toDouble();
}

QString interval_to_str(double secs)
{
    if (secs < 60.0)
        return QString("%1s").arg(secs, 0, 'f', 0, QLatin1Char('0'));
    QString result;
    unsigned rounded = static_cast<unsigned>(round(secs));
    bool needs_colon = false;
    if (rounded >= 3600) {
        result += QString("%1h").arg(rounded / 3600);
        rounded %= 3600;
        needs_colon = true;
    }
    if (needs_colon || rounded >= 60) {
        if (needs_colon)
            result += " ";
        //result += QString("%1m").arg(rounded / 60, 2, 10, QLatin1Char('0'));
        result += QString("%1m").arg(rounded / 60, 2, 10);
        rounded %= 60;
        needs_colon = true;
    }
    if (needs_colon)
        result += " ";
    //result += QString("%1s").arg(rounded, 2, 10, QLatin1Char('0'));
    result += QString("%1s").arg(rounded, 2, 10);
    return result;
}

QDateTime convertToLocalTime(QString timestamp)
{
    //check if the last character is Z designating the timezone to be UTC
    //otherwise assume the timestamp is already in local time and simply convert it
    //ex: 2002-05-30T09:30:10+06:00
    //see http://www.w3schools.com/Schema/schema_dtypes_date.asp
    //for more on this date format

    if (timestamp.size()>=1 && timestamp[static_cast<unsigned int>(timestamp.size())-1].toLower()=='z') {

        // Z at end indicates the time is in fact UTC
        QDateTime datetime = QDateTime::fromString(timestamp, Qt::ISODate);
        datetime.setTimeSpec(Qt::UTC);
        datetime=datetime.toLocalTime();
        return datetime;

    } else if (timestamp.size()>=20 && (timestamp[static_cast<unsigned int>(timestamp.size())-6]=='+' 
                                    || timestamp[static_cast<unsigned int>(timestamp.size())-6]=='-')) {

        // contains timezone offset in hours
        return QDateTime::fromString(timestamp, Qt::ISODate);
    } else if (timestamp.size() == 19 && timestamp[10].toLower() == 't') {

        // ISO format without timezone offset
        return QDateTime::fromString(timestamp, Qt::ISODate);
    } else if (timestamp.size() == 23 && timestamp[10].toLower() == 't') {

        // ISO extended format without timezone offset but with milliseconds
        return QDateTime::fromString(timestamp, Qt::ISODateWithMs);
    }

    // if not sure, just fallback to basic method
    return QDateTime::fromString(timestamp);
}

DateRange::DateRange(QDate from, QDate to, QString name, QColor color) : QObject()
{
    this->from=from;
    this->to=to;
    this->name=name;
    this->color=color;
    valid = from.isValid() && to.isValid();
}

DateRange::DateRange(const DateRange &other) : QObject()
{
    from=other.from;
    to=other.to;
    name=other.name;
    color=other.color;
    id = other.id;
    valid = from.isValid() && to.isValid();
}

DateRange& DateRange::operator=(const DateRange &other)
{
    from=other.from;
    to=other.to;
    name=other.name;
    color=other.color;
    valid = from.isValid() && to.isValid();
    emit changed(from, to);

    return *this;
}

DateSettingsEdit::DateSettingsEdit(QWidget *parent) : parent(parent), active(false)
{
    setContentsMargins(0,0,0,0);
    QFormLayout *mainLayout = new QFormLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(5 *dpiXFactor);

    QFont sameFont;
#ifdef Q_OS_MAC
    sameFont.setPointSize(sameFont.pointSize() + 2);
#endif

    radioSelected = new QRadioButton(tr("Current selection"), this);
    radioSelected->setChecked(true);
    radioSelected->setFont(sameFont);
    QHBoxLayout *selected = new QHBoxLayout; // use same layout mechanism as custom so they align
    selected->addWidget(radioSelected);
    selected->addStretch();
    mainLayout->addRow(selected);

    radioToday = new QRadioButton(tr("Current selection thru today"), this);
    radioToday->setChecked(false);
    radioToday->setFont(sameFont);
    QHBoxLayout *today = new QHBoxLayout; // use same layout mechanism as custom so they align
    today->addWidget(radioToday);
    today->addStretch();
    mainLayout->addRow(today);

    radioFrom = new QRadioButton(tr("From"), this);
    radioFrom->setChecked(false);
    radioFrom->setFont(sameFont);
    startDateEdit = new QDateEdit(this);
    startDateEdit->setDate(QDate::currentDate().addMonths(-3));
    startDateEdit ->setCalendarPopup(true);
    QHBoxLayout *from = new QHBoxLayout;
    from->addWidget(radioFrom);
    from->addWidget(startDateEdit);
    from->addWidget(new QLabel(tr("to today")));
    from->addStretch();
    mainLayout->addRow(from);

    radioCustom = new QRadioButton(tr("Between"), this);
    radioCustom->setFont(sameFont);
    radioCustom->setChecked(false);
    fromDateEdit = new QDateEdit(this);
    fromDateEdit ->setCalendarPopup(true);
    toDateEdit = new QDateEdit(this);
    toDateEdit ->setCalendarPopup(true);
    QHBoxLayout *custom = new QHBoxLayout;
    custom->addWidget(radioCustom);
    custom->addWidget(fromDateEdit);
    custom->addWidget(new QLabel(tr("and")));
    custom->addWidget(toDateEdit);
    custom->addStretch();
    mainLayout->addRow(custom);

    radioLast = new QRadioButton(tr("Last"), this);
    radioLast->setFont(sameFont);
    radioLast->setChecked(false);
    lastn = new QDoubleSpinBox(this);
    lastn->setSingleStep(1.0);
    lastn->setDecimals(0);
    lastn->setMinimum(0);
    lastn->setMaximum(999);
    lastn->setValue(7);
    lastnx = new QComboBox(this);
    lastnx->addItem(tr("days"));
    lastnx->addItem(tr("weeks"));
    lastnx->addItem(tr("months"));
    lastnx->addItem(tr("years"));
    lastnx->setCurrentIndex(0);
    QHBoxLayout *last = new QHBoxLayout;
    last->addWidget(radioLast);
    last->addWidget(lastn);
    last->addWidget(lastnx);
    last->addStretch();
    mainLayout->addRow(last);

    radioThis = new QRadioButton(tr("This"), this);
    radioThis->setFont(sameFont);
    radioThis->setChecked(false);
    thisperiod = new QComboBox(this);
    thisperiod->addItem(tr("week"));
    thisperiod->addItem(tr("month"));
    thisperiod->addItem(tr("year"));
    thisperiod->setCurrentIndex(0);
    prevperiod = new QDoubleSpinBox(this);
    prevperiod->setSingleStep(1.0);
    prevperiod->setDecimals(0);
    prevperiod->setMinimum(0);
    prevperiod->setMaximum(999);
    prevperiod->setValue(0);
    QHBoxLayout *thisl = new QHBoxLayout;
    thisl->addWidget(radioThis);
    thisl->addWidget(thisperiod);
    thisl->addWidget(new QLabel(tr("prior")));
    thisl->addWidget(prevperiod);
    thisl->addStretch();
    mainLayout->addRow(thisl);

    // switched between one or other
    connect(radioSelected, SIGNAL(toggled(bool)), this, SLOT(setDateSettings()));
    connect(radioToday, SIGNAL(toggled(bool)), this, SLOT(setDateSettings()));
    connect(radioCustom, SIGNAL(toggled(bool)), this, SLOT(setDateSettings()));
    connect(radioLast, SIGNAL(toggled(bool)), this, SLOT(setDateSettings()));
    connect(radioFrom, SIGNAL(toggled(bool)), this, SLOT(setDateSettings()));
    connect(radioThis, SIGNAL(toggled(bool)), this, SLOT(setDateSettings()));
    connect(fromDateEdit, SIGNAL(editingFinished()), this, SLOT(setDateSettings()));
    connect(toDateEdit, SIGNAL(editingFinished()), this, SLOT(setDateSettings()));
    connect(startDateEdit, SIGNAL(editingFinished()), this, SLOT(setDateSettings()));
    connect(lastn, SIGNAL(editingFinished()), this, SLOT(setDateSettings()));
    connect(lastnx, SIGNAL(currentIndexChanged(int)), this, SLOT(setDateSettings()));
    connect(thisperiod, SIGNAL(currentIndexChanged(int)), this, SLOT(setDateSettings()));
    connect(prevperiod, SIGNAL(editingFinished()), this, SLOT(setDateSettings()));
}

void
DateSettingsEdit::setDateSettings()
{
    if (active) return;

    // first lets disable everything
    active = true;
    fromDateEdit->setEnabled(false);
    toDateEdit->setEnabled(false);
    startDateEdit->setEnabled(false);
    thisperiod->setEnabled(false);
    prevperiod->setEnabled(false);
    lastn->setEnabled(false);
    lastnx->setEnabled(false);

    // the date selection types have changed
    if (radioSelected->isChecked()) {

        // current selection
        emit useStandardRange();

    } else if (radioCustom->isChecked()) {

        // between x and y
        fromDateEdit->setEnabled(true);
        toDateEdit->setEnabled(true);

        // set date range using custom values
        emit useCustomRange(DateRange(fromDateEdit->date(), toDateEdit->date()));

    } else if (radioToday->isChecked()) {

        // current selected thru to today
        emit useThruToday();

    } else if (radioLast->isChecked()) {

        // last n 'weeks etc'
        lastn->setEnabled(true);
        lastnx->setEnabled(true);

        QDate from;
        QDate today = QDate::currentDate();

        // calculate range up to today...
        switch(lastnx->currentIndex()) {
            case 0 : // days
                from = today.addDays(lastn->value() * -1);
                break;

            case 1 :  // weeks
                from = today.addDays(lastn->value() * -7);
                break;

            case 2 :  // months
                from = today.addMonths(lastn->value() * -1);
                break;

            case 3 : // years
                from = today.addYears(lastn->value() * -1);
                break;
        }

        emit useCustomRange(DateRange(from, today));

    } else if (radioFrom->isChecked()) {

        // from date - today
        startDateEdit->setEnabled(true);
        emit useCustomRange(DateRange(startDateEdit->date(), QDate::currentDate()));

    } else if (radioThis->isChecked()) {

        thisperiod->setEnabled(true);
        prevperiod->setEnabled(true);

        QDate today = QDate::currentDate();
        QDate from, to;

        switch(thisperiod->currentIndex()) {

        case 0 : // weeks
            {
                int dow = today.dayOfWeek(); // 1-7, where 1=monday
                from = today.addDays(-1 * (dow-1));
                to = from.addDays(6);
                // prevperiods
                from = from.addDays(prevperiod->value() * -7);
                to = to.addDays(prevperiod->value() * -7);
            }
            break;

        case 1 : // months
            from = QDate(today.year(), today.month(), 1);
            to = from.addMonths(1).addDays(-1);
            from = from.addMonths(prevperiod->value() * -1);
            to = to.addMonths(prevperiod->value() * -1);
            break;

        case 2 : // years
            from = QDate(today.year(), 1, 1);
            to = from.addYears(1).addDays(-1);
            from = from.addYears(prevperiod->value() * -1);
            to = to.addYears(prevperiod->value() * -1);
            break;

        }
        emit useCustomRange(DateRange(from, to));
    }
    active = false;
}

int
DateSettingsEdit::mode()
{
    if (radioSelected->isChecked()) return 0;
    if (radioFrom->isChecked()) return 1;
    if (radioCustom->isChecked()) return 2;
    if (radioLast->isChecked()) return 3;
    if (radioToday->isChecked()) return 4;
    if (radioThis->isChecked()) return 5;

    return 0; // keep compiler happy
}

void
DateSettingsEdit::setMode(int x)
{
    active = true;
    radioSelected->setChecked(false);
    radioFrom->setChecked(false);
    radioCustom->setChecked(false);
    radioLast->setChecked(false);
    radioToday->setChecked(false);
    radioThis->setChecked(false);
    active = false;

    switch(x) {
    case 0:
        radioSelected->setChecked(true);
        break;
    case 1:
        radioFrom->setChecked(true);
        break;
    case 2:
        radioCustom->setChecked(true);
        break;
    case 3:
        radioLast->setChecked(true);
        break;
    case 4:
        radioToday->setChecked(true);
        break;
    case 5:
        radioThis->setChecked(true);
        break;
    }
}
