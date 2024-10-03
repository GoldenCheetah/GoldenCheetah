/*
 * Copyright (c) 2009 Justin F. Knotzke (jknotzke@shampoo.ca)
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

#include "Season.h"
#include <QString>
#include <QDebug>


//////////////////////////////////////////////////////////////
// SeasonEvent
//

QStringList
SeasonEvent::priorityList()
{
    return QStringList()<<" "<<tr("A")<<tr("B")<<tr("C")<<tr("D")<<tr("E");
}


//////////////////////////////////////////////////////////////
// SeasonOffset
//

SeasonOffset::SeasonOffset()
{
}


SeasonOffset::SeasonOffset(int _years, int _months, int _weeks, bool _align)
{
    years = _years;
    months = _months;
    weeks = _weeks;
    align = _align;
}


SeasonOffset::SeasonOffset
(std::pair<SeasonOffsetType, int> item, bool _align)
{
    switch (item.first) {
    case year:
        years = item.second;
        break;
    case month:
        months = item.second;
        break;
    case week:
        weeks = item.second;
        break;
    default:
        break;
    }
    align = _align;
}


bool
SeasonOffset::operator==
(const SeasonOffset& offset) const
{
    return    years == offset.years
           && months == offset.months
           && weeks == offset.weeks
           && align == offset.align;
}


bool
SeasonOffset::operator!=
(const SeasonOffset& offset) const
{
    return ! (*this == offset);
}


QDate SeasonOffset::getDate(QDate reference) const
{
    if (align) {
        if (years <= 0) {
            return QDate(reference.year(), 1, 1).addYears(years);
        } else if (months <= 0) {
            return QDate(reference.year(), reference.month(), 1).addMonths(months);
        } else if (weeks <= 0) {
            return reference.addDays((Qt::Monday - reference.dayOfWeek()) + 7 * weeks);
        }
    } else {
        if (years <= 0) {
            return reference.addYears(years);
        } else if (months <= 0) {
            return reference.addMonths(months);
        } else if (weeks <= 0) {
            return reference.addDays(7 * weeks);
        }
    }
    return QDate();
}


std::pair<SeasonOffset::SeasonOffsetType, int>
SeasonOffset::getSignificantItem
() const
{
    if (years <= 0) {
        return std::make_pair<SeasonOffsetType, int>(year, int(years));
    } else if (months <= 0) {
        return std::make_pair<SeasonOffsetType, int>(month, int(months));
    } else if (weeks <= 0) {
        return std::make_pair<SeasonOffsetType, int>(week, int(weeks));
    }
    return std::make_pair<SeasonOffsetType, int>(invalid, 0);
}


bool
SeasonOffset::isValid
() const
{
    return years <= 0 || months <= 0 || weeks <= 0;
}


//////////////////////////////////////////////////////////////
// SeasonLength
//

SeasonLength::SeasonLength()
{
}


SeasonLength::SeasonLength(int _years, int _months, int _days)
{
    years = std::max(0, _years);
    months = std::max(0, _months);
    days = std::max(0, _days);
}


bool SeasonLength::operator==(const SeasonLength& length) const
{
    return years == length.years && months == length.months && days == length.days;
}


bool SeasonLength::operator!=(const SeasonLength& length) const
{
    return ! (*this == length);
}


QDate SeasonLength::addTo(QDate start) const
{
    QDate date = start.addYears(years).addMonths(months).addDays(days - 1);
    if (date > start) {
        return date;
    } else {
        return start;
    }
}


QDate SeasonLength::substractFrom(QDate end) const
{
    QDate date = end.addYears(-years).addMonths(-months).addDays(1 - days);
    if (date < end) {
        return date;
    } else {
        return end;
    }
}


int
SeasonLength::getYears
() const
{
    return years;
}


int
SeasonLength::getMonths
() const
{
    return months;
}


int
SeasonLength::getDays
() const
{
    return days;
}


bool
SeasonLength::isValid
() const
{
    return years >= 0 || months >= 0 || days >= 0;
}


//////////////////////////////////////////////////////////////
// Season
//

static QList<QString> _setSeasonTypes()
{
    QList<QString> returning;
    returning << "Season"
              << "Cycle"
              << "Adhoc"
              << "System";
    return returning;
}
QList<QString> Season::types = _setSeasonTypes();


Season::Season()
{
    _id = QUuid::createUuid();
}


void
Season::resetTimeRange
()
{
    _absoluteStart = QDate();
    _absoluteEnd = QDate();
    _offsetStart = SeasonOffset();
    _offsetEnd = SeasonOffset();
    _length = SeasonLength();
    _ytd = false;
}


QDate Season::getStart() const
{
    return getStart(QDate::currentDate());
}


QDate Season::getEnd() const
{
    return getEnd(QDate::currentDate());
}


QDate Season::getStart(QDate reference) const
{
    QDate offsetStart = _offsetStart.getDate(reference);

    if (_absoluteStart.isValid()) {
        return _absoluteStart;
    } else if (offsetStart.isValid()) {
        return offsetStart;
    } else if (_length.isValid()) {
        if (_absoluteEnd.isValid()) {
            return _length.substractFrom(_absoluteEnd);
        } else if (_offsetEnd.isValid()) {
            return _length.substractFrom(_offsetEnd.getDate(reference));
        } else {
            return _length.substractFrom(reference);
        }
    } else {
        return reference;
    }
}


QDate Season::getEnd(QDate reference) const
{
    QDate offsetEnd = _offsetEnd.getDate(reference);

    if (_absoluteEnd.isValid()) {
        return _absoluteEnd;
    } else if (offsetEnd.isValid()) {
        return offsetEnd;
    } if (_ytd) {
        QDate start = getStart(reference);
        QDate ytdEnd(start.year(), reference.month(), reference.day());
        if (   ! ytdEnd.isValid()
            && QDate::isLeapYear(reference.year())
            && reference.month() == 2
            && reference.day() == 29) {
            ytdEnd.setDate(start.year(), 2, 28);
        }
        if (ytdEnd < start) {
            ytdEnd = ytdEnd.addYears(1);
        }
        return ytdEnd;
    } else if (_length.isValid()) {
        if (_absoluteStart.isValid()) {
            return _length.addTo(_absoluteStart);
        } else if (_offsetStart.isValid()) {
            return _length.addTo(_offsetStart.getDate(reference));
        } else {
            return reference;
        }
    } else {
        return reference;
    }
}


void Season::setName(QString _name)
{
    name = _name;
}


QString
Season::getName
() const
{
    return name;
}


void Season::setType(int _type)
{
    type = _type;
}


int Season::getType() const
{
    return type;
}


void Season::setAbsoluteStart(QDate start)
{
    _offsetStart = SeasonOffset();
    _absoluteStart = start;
}


QDate
Season::getAbsoluteStart
() const
{
    return _absoluteStart;
}


void Season::setAbsoluteEnd(QDate end)
{
    _offsetEnd = SeasonOffset();
    _absoluteEnd = end;
    _ytd = false;
}


QDate
Season::getAbsoluteEnd
() const
{
    return _absoluteEnd;
}


void
Season::setOffsetStart
(int offsetYears, int offsetMonths, int offsetWeeks, bool align)
{
    setOffsetStart(SeasonOffset(offsetYears, offsetMonths, offsetWeeks, align));
}


void
Season::setOffsetStart
(SeasonOffset offset)
{
    _offsetStart = offset;
    _absoluteStart = QDate();
}


SeasonOffset
Season::getOffsetStart() const
{
    return _offsetStart;
}


SeasonOffset
Season::getOffsetEnd
() const
{
    return _offsetEnd;
}


void
Season::setOffsetEnd
(int offsetYears, int offsetMonths, int offsetWeeks, bool align)
{
    setOffsetEnd(SeasonOffset(offsetYears, offsetMonths, offsetWeeks, align));
}


void
Season::setOffsetEnd
(SeasonOffset offset)
{
    _offsetEnd = offset;
    _absoluteEnd = QDate();
    _ytd = false;
}


void Season::setOffsetAndLength(int offsetYears, int offsetMonths, int offsetWeeks, int years, int months, int days)
{
    type = temporary;
    _offsetStart = SeasonOffset(offsetYears, offsetMonths, offsetWeeks);
    _length = SeasonLength(years, months, days);
    _absoluteStart = QDate();
    _absoluteEnd = QDate();
    _ytd = false;
}


void Season::setLengthOnly(int years, int months, int days)
{
    type = temporary;
    _offsetStart = SeasonOffset();
    _length = SeasonLength(years, months, days);
    _absoluteStart = QDate();
    _absoluteEnd = QDate();
    _ytd = false;
}


void Season::setLength(int years, int months, int days)
{
    setLength(SeasonLength(years, months, days));
}


void
Season::setLength
(SeasonLength length)
{
    _length = length;
    _ytd = false;
}


void Season::setYtd()
{
    _length = SeasonLength();
    _absoluteEnd = QDate();
    _ytd = true;
}


bool
Season::isYtd
() const
{
    return _ytd;
}


bool
Season::isAbsolute
() const
{
    return    (   _absoluteStart.isValid()
               && _absoluteEnd.isValid())
           || (   (   _absoluteStart.isValid()
                   || _absoluteEnd.isValid())
               && _length.isValid());
}


bool
Season::hasPhaseOrEvent
() const
{
    return phases.length() == 0 && events.length() == 0;
}


bool Season::LessThanForStarts(const Season &a, const Season &b)
{
    return a.getStart().toJulianDay() < b.getStart().toJulianDay();
}


//////////////////////////////////////////////////////////////
// Phase
//

static QList<QString> _setPhaseTypes()
{
    QList<QString> returning;
    returning << "Phase"
              << "Prep"
              << "Base"
              << "Build"
              << "Peak"
              << "Camp";
    return returning;
}
QList<QString> Phase::types = _setPhaseTypes();


Phase::Phase() : Season()
{
    type = phase;  // by default phase are of type phase
}


Phase::Phase(QString _name, QDate start, QDate end) : Season()
{
    type = phase;  // by default phase are of type phase
    name = _name;
    _absoluteStart = start;
    _absoluteEnd = end;
}
