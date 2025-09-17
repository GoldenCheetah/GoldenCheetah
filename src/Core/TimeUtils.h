/*
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
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

#ifndef _TimeUtils_h
#define _TimeUtils_h

#include <QDate>
#include <QUuid>
#include <QDateEdit>
#include <QColor>
#include <QDateTime>
#include <QString>
#include <QRadioButton>
#include <QDoubleSpinBox>
#include <QComboBox>

// the following time constants are widely used within GC:
static const QDate GC_EPOCH(1900,01,01);
static const QDate GC_UNIX_EPOCH(1970, 1, 1);
static const QDate GC_VERSION_CHK_EPOCH(1990, 1, 1);
static const QDate GC_INFINITY(9999,12,31); // maximum value for Python datetime module

// these will probably need to be reviewed and updated at some future point
static const QDate GC_MIN_EDIT_DATE(2000,01,01); // default minimum value for time editing fields
static const QDate GC_MAX_EDIT_DATE(2030,01,01); // default maximum value for time editing fields


QString interval_to_str(double secs);  // output like 1h 2m 3s
double str_to_interval(const QString& s);     // convert 1h 2m 3s -> 3123.0 , e.g.
QString time_to_string(double secs, bool forceMinutes=false);   // output like 1:02:03
QString time_to_string_for_sorting(double secs);   // output always xx:yy:zz
QString time_to_string_minutes(double secs); // output always hh:mm - seconds are cut-off

/* takes a string containing an ISO 8601 timestamp and converts it to local time
*/
QDateTime convertToLocalTime(const QString& timestamp);

class DateRange : QObject
{
    Q_OBJECT

    public:
        DateRange(const DateRange& other);
        DateRange(const QDate& from = QDate(), const QDate& to = QDate(), const QString& name ="", const QColor& color =QColor(127,127,127));
        DateRange& operator=(const DateRange &);
        bool operator!=(const DateRange&other) const {
            if (other.from != from || other.to != to || other.name != name) return true;
            return false;
        }
        bool operator==(const DateRange&other) const {
            if (other.from == from && other.to == to && other.name == name) return true;
            return false;
        }

        QDate from, to;
        QString name;
        QColor color; // used by R code only
        QUuid id;

        // does this date fall in the range selection ?
        bool pass(const QDate& date) const {
            if (from == QDate() && to == QDate()) return true;
            if (from == QDate() && date <= to) return true;
            if (to == QDate() && date >= from) return true;
            if (date >= from && date <= to) return true;
            return false;
        }
        bool isValid() const { return valid; }


    signals:
        void changed(QDate from, QDate to);

    protected:
        bool valid;
};
Q_DECLARE_METATYPE(DateRange)

class DateSettingsEdit : public QWidget
{
    Q_OBJECT

    private:
        QWidget *parent;
        bool active;

        // editing components
        QRadioButton *radioSelected, *radioToday, *radioCustom, *radioLast, *radioFrom, *radioThis;
        QDateEdit *fromDateEdit, *toDateEdit, *startDateEdit;
        QDoubleSpinBox *lastn,*prevperiod;
        QComboBox *lastnx;
        QComboBox *thisperiod;

    public:

        DateSettingsEdit(QWidget *parent);

        void setMode(int);
        int mode();

        // betweem from and to
        void setFromDate(const QDate& x) { fromDateEdit->setDate(x); }
        QDate fromDate() { return fromDateEdit->date(); }
        void setToDate(const QDate& x) { toDateEdit->setDate(x); }
        QDate toDate() { return toDateEdit->date(); }

        // start date till today
        void setStartDate(const QDate& x) { startDateEdit->setDate(x); }
        QDate startDate() { return startDateEdit->date(); }

        // last n of days/weeks/months/years
        int lastN() { return lastn->value(); }
        void setLastN(int x)  { lastn->setValue(x); }
        int lastNX() { return lastnx->currentIndex(); }
        void setLastNX(int x)  { lastnx->setCurrentIndex(x); }

        // this week/month/year
        int thisN() { return thisperiod->currentIndex(); }
        void setThisN(int x) { thisperiod->setCurrentIndex(x); }
        int prevN() { return prevperiod->value(); }
        void setPrevN(int x) { return prevperiod->setValue(x); }

    private slots:
        void setDateSettings();

    signals:
        void useStandardRange();
        void useThruToday();
        void useCustomRange(DateRange);

};

#endif // _TimeUtils_h
