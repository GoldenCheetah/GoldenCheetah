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

SeasonEvent::SeasonEvent
(QString name, QDate date, int priority, QString description, QString id, int targetLTS, int targetCP, int targetFTP)
: name(name), date(date), priority(priority), description(description), id(id), targetLTS(targetLTS), targetCP(targetCP), targetFTP(targetFTP)
{
    if (this->id.isEmpty()) {
        this->id = QUuid::createUuid().toString();
    }
}


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


bool Season::isMacrocycle() const
{
    QList<Phase>::const_iterator it = std::find_if(phases.cbegin(), phases.cend(), [](const Phase &phase) {
        return phase.getType() == Phase::mesocycle;
    });
    return isAbsolute() && it != phases.cend();
}


void Season::setModelType(Season::ModelType modeltype)
{
    _model = modeltype;
}


Season::ModelType Season::getModelType() const
{
    return _model;
}


void Season::setFirstDayOfWeek(Qt::DayOfWeek dow)
{
    _firstDayOfWeek = dow;
}


Qt::DayOfWeek Season::getFirstDayOfWeek() const
{
    return _firstDayOfWeek;
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


bool
Season::canHavePhasesOrEvents
() const
{
    return   (   getType() == Season::season
              || getType() == Season::cycle
              || getType() == Season::adhoc)
           && isAbsolute();
}


bool Season::LessThanForStarts(const Season &a, const Season &b)
{
    return a.getStart().toJulianDay() < b.getStart().toJulianDay();
}


//////////////////////////////////////////////////////////////
// Mesocycle

Microcycle::Microcycle
(int freq, int load, float intensity, int time, const QString &description)
{
    setFrequency(freq);
    setLoad(load);
    setIntensity(intensity);
    setTime(time);
    setDescription(description);
}

bool
Microcycle::setFrequency
(int freq)
{
    if (freq < 0) {
        return false;
    }
    this->freq = freq;
    return true;
}


int
Microcycle::getFrequency
() const
{
    return freq;
}


bool
Microcycle::setLoad
(int load)
{
    if (load < 0) {
        return false;
    }
    this->load = load;
    return true;
}


int
Microcycle::getLoad
() const
{
    return load;
}


bool
Microcycle::setIntensity
(float intensity)
{
    if (intensity < 0.0f) {
        return false;
    }
    this->intensity = intensity;
    return true;
}


float
Microcycle::getIntensity
() const
{
    return intensity;
}


bool
Microcycle::setTime
(int time)
{
    if (time < 0) {
        return false;
    }
    this->time = time;
    return true;
}


int
Microcycle::getTime
() const
{
    return time;
}


void
Microcycle::setDescription
(const QString &description)
{
    this->desc = description;
}


QString
Microcycle::getDescription
() const
{
    return desc;
}


//////////////////////////////////////////////////////////////
// Slot


QString
Slot::statusToString
(Slot::StatusType status)
{
    switch (status) {
        case StatusType::generated:
            return QStringLiteral("generated");
        case StatusType::adjusted:
            return QStringLiteral("adjusted");
        default:
            return QStringLiteral("generated");
    }
}


Slot::StatusType
Slot::stringToStatus
(const QString &str)
{
    if (str == QStringLiteral("generated")) {
        return StatusType::generated;
    } else if (str == QStringLiteral("adjusted")) {
        return StatusType::adjusted;
    }
    return StatusType::generated;
}


Slot::Slot
(int dayOffset, const QString &sport, const StatusType status, int load, float intensity, int time, const QString &description)
{
    setDayOffset(dayOffset);
    setSport(sport);
    setStatus(status);
    setLoad(load);
    setIntensity(intensity);
    setTime(time);
    setDescription(description);
}


bool
Slot::setDayOffset
(int offset)
{
    if (offset < 0) {
        return false;
    }
    this->dayOffset = offset;
    return true;

}


int
Slot::getDayOffset
() const
{
    return dayOffset;
}


bool
Slot::setSport
(const QString &sport)
{
    this->sport = sport;
    return true;
}


QString
Slot::getSport
() const
{
    return sport;
}


bool
Slot::setStatus
(const Slot::StatusType &status)
{
    this->status = status;
    return true;
}


Slot::StatusType
Slot::getStatus
() const
{
    return status;
}


bool
Slot::setLoad
(int load)
{
    if (load < 0) {
        return false;
    }
    this->load = load;
    return true;
}


int
Slot::getLoad
() const
{
    return load;
}


bool
Slot::hasLoad
() const
{
    return load > 0;
}

bool
Slot::setIntensity
(float intensity)
{
    if (intensity < 0) {
        return false;
    }
    this->intensity = intensity;
    return true;
}


float
Slot::getIntensity
() const
{
    return intensity;
}


bool
Slot::hasIntensity
() const
{
    return intensity > 0.0f;
}


bool
Slot::setTime
(int time)
{
    if (time < 0) {
        return false;
    }
    this->time = time;
    return true;
}


int
Slot::getTime
() const
{
    return time;
}


bool
Slot::hasTime
() const
{
    return time > 0;
}


bool
Slot::setDescription
(const QString &description)
{
    this->description = description;
    return true;
}


QString
Slot::getDescription
() const
{
    return description;
}


bool
Slot::hasDescription
() const
{
    return ! description.isEmpty();
}


const QVarLengthArray<QString, 1>&
Slot::getActivities
() const
{
    return activityIds;
}


bool
Slot::hasActivity
(const QString &id) const
{
    return activityIds.contains(id);
}


void
Slot::addActivity
(const QString &id)
{
    if (! id.isEmpty() && ! activityIds.contains(id)) {
        activityIds.append(id);
    }
}


void
Slot::setActivities
(const QStringList &ids)
{
    activityIds.clear();
    for (const QString &id : ids) {
        activityIds.append(id);
    }
}


bool
Slot::removeActivity
(const QString &id)
{
    return activityIds.removeOne(id);
}


void
Slot::clearActivities
()
{
    activityIds.clear();
}


//////////////////////////////////////////////////////////////
// Mesocycle

bool
Mesocycle::setStart
(const QDate &date)
{
    if (date.isValid()) {
        start = date;
        return true;
    } else {
        return false;
    }
}


void
Mesocycle::setTargetLTS
(int targetLTS)
{
    this->targetLTS = std::max(0, targetLTS);
}


int
Mesocycle::getTargetLTS
() const
{
    return targetLTS;
}


void
Mesocycle::setTargetCP
(int targetCP)
{
    this->targetCP = std::max(0, targetCP);
}


int
Mesocycle::getTargetCP
() const
{
    return targetCP;
}


void
Mesocycle::setTargetFTP
(int targetFTP)
{
    this->targetFTP = std::max(0, targetFTP);
}


int
Mesocycle::getTargetFTP
() const {
    return targetFTP;
}


int
Mesocycle::toMicrocycle
(const QDate &date) const
{
    return toMicrocycle(start.daysTo(date));
}


int
Mesocycle::toMicrocycle
(int offset) const
{
    int maxIndex = getMaxMicrocycles() - 1;
    if (maxIndex < 0) {
        return 0;
    }
    return std::clamp(offset / 7, 0, maxIndex);
}


bool
Mesocycle::setMaxMicrocycles
(int maxMicrocycles)
{
    if (maxMicrocycles > 0) {
        this->maxMicrocycles = maxMicrocycles;
        microcycles.erase(microcycles.upperBound(maxMicrocycles - 1), microcycles.end());
        const int maxOffset = maxMicrocycles * 7;
        slots_.removeIf([maxOffset](const Slot &slot) {
            return slot.getDayOffset() >= maxOffset;
        });
        return true;
    } else {
        return false;
    }
}


int
Mesocycle::getMaxMicrocycles
() const
{
    return maxMicrocycles;
}


bool
Mesocycle::hasMicrocycle
(int microcycle) const
{
    return microcycle >= 0 && microcycle < getMaxMicrocycles();
}


Microcycle*
Mesocycle::getMicrocycle
(int microcycle)
{
    if (hasMicrocycle(microcycle)) {
        return &microcycles[microcycle];
    }
    return nullptr;
}


Microcycle const*
Mesocycle::getMicrocycle
(int microcycle) const
{
    if (! hasMicrocycle(microcycle)) {
        return nullptr;
    }
    QMap<int, Microcycle>::const_iterator it = microcycles.constFind(microcycle);
    if (it != microcycles.constEnd()) {
        return &it.value();
    }
    return &dummyMicrocycle;
}


int
Mesocycle::getMicrocyclesFrequency
() const
{
    return std::accumulate(microcycles.cbegin(), microcycles.cend(), 0, [](int total, const Microcycle &microcycle) {
        return total + microcycle.getFrequency();
    });
}


int
Mesocycle::getMicrocyclesLoad
() const
{
    return std::accumulate(microcycles.cbegin(), microcycles.cend(), 0, [](int total, const Microcycle &microcycle) {
        return total + microcycle.getLoad();
    });
}


float
Mesocycle::getMicrocyclesIntensity
() const
{
    float weightedSum = 0.0f;
    float totalLoad = 0.0f;
    for (const Microcycle &microcycle : microcycles) {
        weightedSum += microcycle.getIntensity() * microcycle.getLoad();
        totalLoad += microcycle.getLoad();
    }
    return totalLoad > 0.0f ? weightedSum / totalLoad : 0.0f;
}


int
Mesocycle::getMicrocyclesTime
() const
{
    return std::accumulate(microcycles.cbegin(), microcycles.cend(), 0, [](int total, const Microcycle &microcycle) {
        return total + microcycle.getTime();
    });
}


int
Mesocycle::getSlottedFrequency
() const
{
    return slots_.count();
}


int
Mesocycle::getSlottedFrequency
(const QString &sport) const
{
    return std::count_if(slots_.cbegin(), slots_.cend(), [sport](const Slot &slot) {
        return sport == slot.getSport();
    });
}


int
Mesocycle::getSlottedFrequency
(int microcycle) const
{
    return std::count_if(slots_.cbegin(), slots_.cend(), [this, microcycle](const Slot &slot) {
        return microcycle == toMicrocycle(slot.getDayOffset());
    });
}


int
Mesocycle::getSlottedFrequency
(int microcycle, const QString &sport) const
{
    return std::count_if(slots_.cbegin(), slots_.cend(), [this, microcycle, sport](const Slot &slot) {
        return microcycle == toMicrocycle(slot.getDayOffset()) && sport == slot.getSport();
    });
}


int
Mesocycle::getSlottedLoad
() const
{
    return std::accumulate(slots_.cbegin(), slots_.cend(), 0, [](int total, const Slot &slot) {
        return total + slot.getLoad();
    });
}


int
Mesocycle::getSlottedLoad
(const QString &sport) const
{
    return std::accumulate(slots_.cbegin(), slots_.cend(), 0, [sport](int total, const Slot &slot) {
        return sport == slot.getSport() ? total + slot.getLoad() : total;
    });
}


int
Mesocycle::getSlottedLoad
(int microcycle) const
{
    return std::accumulate(slots_.cbegin(), slots_.cend(), 0, [this, microcycle](int total, const Slot &slot) {
        return microcycle == toMicrocycle(slot.getDayOffset()) ? total + slot.getLoad() : total;
    });
}


int
Mesocycle::getSlottedLoad
(int microcycle, const QString &sport) const
{
    return std::accumulate(slots_.cbegin(), slots_.cend(), 0, [this, microcycle, sport](int total, const Slot &slot) {
        return microcycle == toMicrocycle(slot.getDayOffset()) && sport == slot.getSport() ? total + slot.getLoad() : total;
    });
}


float
Mesocycle::getSlottedIntensity
() const
{
    float weightedSum = 0.0f;
    float totalLoad = 0.0f;
    for (const Slot &slot : slots_) {
        weightedSum += slot.getIntensity() * slot.getLoad();
        totalLoad += slot.getLoad();
    }
    return totalLoad > 0.0f ? weightedSum / totalLoad : 0.0f;
}


float
Mesocycle::getSlottedIntensity
(const QString &sport) const
{
    float weightedSum = 0.0f;
    float totalLoad = 0.0f;
    for (const Slot &slot : slots_) {
        if (sport == slot.getSport()) {
            weightedSum += slot.getIntensity() * slot.getLoad();
            totalLoad += slot.getLoad();
        }
    }
    return totalLoad > 0.0f ? weightedSum / totalLoad : 0.0f;
}


float
Mesocycle::getSlottedIntensity
(int microcycle) const
{
    float weightedSum = 0.0f;
    float totalLoad = 0.0f;
    for (const Slot &slot : slots_) {
        if (microcycle == toMicrocycle(slot.getDayOffset())) {
            weightedSum += slot.getIntensity() * slot.getLoad();
            totalLoad += slot.getLoad();
        }
    }
    return totalLoad > 0.0f ? weightedSum / totalLoad : 0.0f;
}


float
Mesocycle::getSlottedIntensity
(int microcycle, const QString &sport) const
{
    float weightedSum = 0.0f;
    float totalLoad = 0.0f;
    for (const Slot &slot : slots_) {
        if (microcycle == toMicrocycle(slot.getDayOffset()) && sport == slot.getSport()) {
            weightedSum += slot.getIntensity() * slot.getLoad();
            totalLoad += slot.getLoad();
        }
    }
    return totalLoad > 0.0f ? weightedSum / totalLoad : 0.0f;
}


int
Mesocycle::getSlottedTime
() const
{
    return std::accumulate(slots_.cbegin(), slots_.cend(), 0, [](int total, const Slot &slot) {
        return total + slot.getTime();
    });
}


int
Mesocycle::getSlottedTime
(const QString &sport) const
{
    return std::accumulate(slots_.cbegin(), slots_.cend(), 0, [sport](int total, const Slot &slot) {
        return sport == slot.getSport() ? total + slot.getTime() : total;
    });
}


int
Mesocycle::getSlottedTime
(int microcycle) const
{
    return std::accumulate(slots_.cbegin(), slots_.cend(), 0, [this, microcycle](int total, const Slot &slot) {
        return microcycle == toMicrocycle(slot.getDayOffset()) ? total + slot.getTime() : total;
    });
}


int
Mesocycle::getSlottedTime
(int microcycle, const QString &sport) const
{
    return std::accumulate(slots_.cbegin(), slots_.cend(), 0, [this, microcycle, sport](int total, const Slot &slot) {
        return microcycle == toMicrocycle(slot.getDayOffset()) && sport == slot.getSport() ? total + slot.getTime() : total;
    });
}


bool
Mesocycle::hasSlot
(const QDate &when, const QString &sport) const
{
    return hasSlot(start.daysTo(when), sport);
}


bool
Mesocycle::hasSlot
(int offset, const QString &sport) const
{
    return std::any_of(slots_.cbegin(), slots_.cend(), [offset, &sport](const Slot &slot) {
        return slot.getDayOffset() == offset && slot.getSport() == sport;
    });
}


Slot*
Mesocycle::getSlot
(const QDate &when, const QString &sport)
{
    return getSlot(start.daysTo(when), sport);
}


Slot const *
Mesocycle::getSlot
(const QDate &when, const QString &sport) const
{
    return getSlot(start.daysTo(when), sport);
}


Slot*
Mesocycle::getSlot
(int offset, const QString &sport)
{
    QList<Slot>::iterator it = std::find_if(slots_.begin(), slots_.end(), [offset, &sport](const Slot &slot) {
        return slot.getDayOffset() == offset && slot.getSport() == sport;
    });
    return it != slots_.end() ? &(*it) : nullptr;
}


Slot const *
Mesocycle::getSlot
(int offset, const QString &sport) const
{
    QList<Slot>::const_iterator it = std::find_if(slots_.begin(), slots_.end(), [offset, &sport](const Slot &slot) {
        return slot.getDayOffset() == offset && slot.getSport() == sport;
    });
    return it != slots_.end() ? &(*it) : nullptr;
}


Slot*
Mesocycle::addSlot
(const QDate &when, const QString &sport)
{
    return addSlot(start.daysTo(when), sport);
}


Slot*
Mesocycle::addSlot
(int offset, const QString &sport)
{
    if (Slot *existing = getSlot(offset, sport)) {
        return existing;
    }
    if (offset < 0 || offset >= getMaxMicrocycles() * 7 || sport.isEmpty()) {
        return nullptr;
    }
    slots_.append(Slot(offset, sport));
    return &slots_.last();
}


void
Mesocycle::delSlot
(int offset, const QString &sport)
{
    slots_.removeIf([offset, &sport](const Slot &slot) {
        return slot.getDayOffset() == offset && slot.getSport() == sport;
    });
}


QMap<QString, QList<int>>
Mesocycle::getSlotSportOffsets
() const
{
    QMap<QString, QList<int>> ret;
    for (const Slot &slot : slots_) {
        ret[slot.getSport()].append(slot.getDayOffset());
    }
    for (QMap<QString, QList<int>>::iterator it = ret.begin(); it != ret.end(); ++it) {
        std::sort(it->begin(), it->end());
    }
    return ret;
}


QMap<QString, QList<QDate>>
Mesocycle::getSlotSportDates
() const
{
    QMap<QString, QList<QDate>> ret;
    const QMap<QString, QList<int>> offsets = getSlotSportOffsets();
    for (QMap<QString, QList<int>>::const_iterator it = offsets.cbegin(); it != offsets.cend(); ++it) {
        QList<QDate> dates;
        dates.reserve(it.value().size());
        for (int offset : it.value()) {
            dates.append(start.addDays(offset));
        }
        ret.insert(it.key(), dates);
    }
    return ret;
}


QMap<int, QList<QString>>
Mesocycle::getSlotOffsetSports
() const
{
    QMap<int, QList<QString>> ret;
    for (const Slot &slot : slots_) {
        ret[slot.getDayOffset()].append(slot.getSport());
    }
    return ret;
}


QMap<QDate, QList<QString>>
Mesocycle::getSlotDateSports
() const
{
    QMap<QDate, QList<QString>> ret;
    for (const Slot &slot : slots_) {
        ret[start.addDays(slot.getDayOffset())].append(slot.getSport());
    }
    return ret;
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


void
Phase::setType(int _type)
{
    if (_type != getType() && _type >= PhaseType::phase) {
        if (getType() != PhaseType::mesocycle) {
            Season::setType(_type);
        }
        if (getType() == PhaseType::mesocycle) {
            Mesocycle *meso = addMesocycle();
        }
    }
    if (getType() == PhaseType::mesocycle) {
        Mesocycle *meso = getMesocycle();
        meso->setMaxMicrocycles(numMicrocycles());
        meso->setStart(getStart());
    }
}


void
Phase::setAbsoluteStart(QDate _start)
{
    Season::setAbsoluteStart(_start);
    if (hasMesocycle()) {
        getMesocycle()->setMaxMicrocycles(numMicrocycles());
        getMesocycle()->setStart(getStart());
    }
}


void
Phase::setAbsoluteEnd(QDate _end)
{
    Season::setAbsoluteEnd(_end);
    if (hasMesocycle()) {
        getMesocycle()->setMaxMicrocycles(numMicrocycles());
    }
}


bool
Phase::hasMesocycle
() const
{
    return _mesocycle.has_value();
}


Mesocycle*
Phase::getMesocycle
()
{
    if (_mesocycle.has_value()) {
        return &_mesocycle.value();
    } else {
        return nullptr;
    }
}


Mesocycle const *
Phase::getMesocycle
() const
{
    if (_mesocycle.has_value()) {
        return &_mesocycle.value();
    } else {
        return nullptr;
    }
}


int
Phase::numMicrocycles
() const
{
    return getStart().daysTo(getEnd()) / 7 + 1;
}


Mesocycle*
Phase::addMesocycle
()
{
    _mesocycle.emplace();
    _mesocycle->setStart(getStart());
    _mesocycle->setMaxMicrocycles(numMicrocycles());
    return &_mesocycle.value();
}
