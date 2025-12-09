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
#include <QObject>

QString interval_to_str(double secs);  // output like 1h 2m 3s
double str_to_interval(QString s);     // convert 1h 2m 3s -> 3123.0 , e.g.
QString time_to_string(double secs, bool forceMinutes=false);   // output like 1:02:03
QString time_to_string_for_sorting(double secs);   // output always xx:yy:zz
QString time_to_string_minutes(double secs); // output always hh:mm - seconds are cut-off
extern int daysToWeeks(int days);
extern int daysToMonths(int days);
enum class ShowDaysAsUnit {
    Days,
    Weeks,
    Months
};
extern ShowDaysAsUnit showDaysAs(int days);


/* takes a string containing an ISO 8601 timestamp and converts it to local time
*/
QDateTime convertToLocalTime(QString timestamp);

class DateRange : QObject
{
    Q_OBJECT

    public:
        DateRange(const DateRange& other);
        DateRange(QDate from = QDate(), QDate to = QDate(), QString name ="", QColor=QColor(127,127,127));
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
        bool pass(QDate date) const {
            if (from == QDate() && to == QDate()) return true;
            if (from == QDate() && date <= to) return true;
            if (to == QDate() && date >= from) return true;
            if (date >= from && date <= to) return true;
            return false;
        }
        bool isValid() const { return valid; }


    Q_SIGNALS:
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
        void setFromDate(QDate x) { fromDateEdit->setDate(x); }
        QDate fromDate() { return fromDateEdit->date(); }
        void setToDate(QDate x) { toDateEdit->setDate(x); }
        QDate toDate() { return toDateEdit->date(); }

        // start date till today
        void setStartDate(QDate x) { startDateEdit->setDate(x); }
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

    private Q_SLOTS:
        void setDateSettings();

    Q_SIGNALS:
        void useStandardRange();
        void useThruToday();
        void useCustomRange(DateRange);

};

#endif // _TimeUtils_h
