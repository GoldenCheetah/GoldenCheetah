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

#ifndef SEASON_H_
#define SEASON_H_

#include <QString>
#include <QDate>
#include <QCoreApplication>
#include <QUuid>

class Phase;
class Season;


class SeasonEvent
{
    Q_DECLARE_TR_FUNCTIONS(Season)

    public:
        static QStringList priorityList();

        SeasonEvent(QString name, QDate date, int priority=0, QString description="", QString id="", int targetLTS=0, int targetCP=0, int targetFTP=0);

        QString name;
        QDate date;
        int priority;
        QString description;
        QString id; // unique id
        int targetLTS = 0;
        int targetCP = 0;
        int targetFTP = 0;
};


class SeasonOffset
{
    public:
        enum SeasonOffsetType { invalid = -1, year = 0, month = 1, week = 2 };

        // by default, the offset is invalid
        SeasonOffset();

        // if different from the default, the offset represent the start
        // of the season with respect to the year/month/week that
        // contains the reference date (i.e. either the current date, or
        // the date of the selected activity)
        // When align is true start the season on the entities start
        // (i.e. weeks: monday, month: 1., year: 1.1.)
        SeasonOffset(int _years, int _months, int _weeks, bool _align = true);
        SeasonOffset(std::pair<SeasonOffsetType, int> item, bool _align = true);

        bool operator==(const SeasonOffset& offset) const;
        bool operator!=(const SeasonOffset& offset) const;

        // get the offest date, or QDate() if the offset is invalid
        QDate getDate(QDate reference) const;

        std::pair<SeasonOffsetType, int> getSignificantItem() const;

        bool isValid() const;

    private:
        // if <=0, offset in years of the season with respect to the
        // current year
        // if >0, unused: go to `months`
        int years = 1;

        // if <=0, offset in months of the season with respect to the
        // current month
        // if >0, unused: go to `weeks`
        int months = 1;

        // if <=0, offset in weeks of the season with respect to the
        // current week (Qt weeks start on Monday)
        // if >0, the whole offset is unused (either the season is
        // fixed, or it ends on the reference day)
        int weeks = 1;

        bool align = true;
};


class SeasonLength
{
    public:
        // by default, the length is invalid
        SeasonLength();

        // if different from the default, the length represent the
        // number of years, months and days from the start to the end,
        // included (i.e. if start==end, the length is (0,0,1))
        SeasonLength(int _years, int _months, int _days);

        bool operator==(const SeasonLength& length) const;
        bool operator!=(const SeasonLength& length) const;

        QDate addTo(QDate start) const;
        QDate substractFrom(QDate end) const;

        int getYears() const;
        int getMonths() const;
        int getDays() const;

        bool isValid() const;

    private:
        // if (-1,-1,-1), the length is invalid (the season is fixed)
        int years = -1;
        int months = -1;
        int days = -1;

};


class Season
{
    Q_DECLARE_TR_FUNCTIONS(Season)

    public:
        static QList<QString> types;
        enum SeasonType { season=0, cycle=1, adhoc=2, temporary=3 };

        enum class ModelType : int {
            none = -1,
            coggan,
            skiba
        };

        static QString modelToString(ModelType status) {
            switch (status) {
                case ModelType::coggan:
                    return QStringLiteral("coggan");
                case ModelType::skiba:
                    return QStringLiteral("skiba");
                default:
                    return QStringLiteral("none");
            }
        }

        static ModelType modelFromString(const QString &str) {
            if (str == QStringLiteral("coggan")) {
                return ModelType::coggan;
            } else if (str == QStringLiteral("skiba")) {
                return ModelType::skiba;
            } else {
                return ModelType::none;
            }
        }

        Season();

        void resetTimeRange();

        // get the date range (if relative, use today as reference)
        QDate getStart() const;
        QDate getEnd() const;

        // get the date range (if relative, use the specified date as reference)
        QDate getStart(QDate reference) const;
        QDate getEnd(QDate reference) const;

        void setName(QString _name);
        QString getName() const;

        void setId(QUuid x) { _id = x; }
        QUuid id() const { return _id; }

        virtual void setType(int _type);
        int getType() const;

        bool isMacrocycle() const;

        void setModelType(ModelType modeltype);
        ModelType getModelType() const;

        void setFirstDayOfWeek(Qt::DayOfWeek dow);
        Qt::DayOfWeek getFirstDayOfWeek() const;

        // make the season fixed (by invalidating _offsetStart and _length)
        // and set the limits of a fixed season
        virtual void setAbsoluteStart(QDate _start);
        QDate getAbsoluteStart() const;

        virtual void setAbsoluteEnd(QDate _end);
        QDate getAbsoluteEnd() const;

        void setOffsetStart(int offsetYears, int offsetMonths, int offsetWeeks, bool align = true);
        void setOffsetStart(SeasonOffset offset);
        SeasonOffset getOffsetStart() const;

        void setOffsetEnd(int offsetYears, int offsetMonths, int offsetWeeks, bool align = true);
        void setOffsetEnd(SeasonOffset offset);
        SeasonOffset getOffsetEnd() const;

        // make the season relative (by setting _length; also
        // invalidates _start and _end) and set the offset of the start
        // with respect to the reference
        void setOffsetAndLength(int offetYears, int offsetMonths, int offsetWeeks, int years, int months, int days);

        // make the season relative (by setting _length; also
        // invalidates _start and _end) and invalidates the offset to
        // make the season end on the reference
        void setLengthOnly(int years, int months, int days);

        void setLength(int years, int months, int days);
        void setLength(SeasonLength length);

        // length of a relative season, SeasonLength() if fixed
        SeasonLength getLength() const { return _length; }

        void setYtd();
        bool isYtd() const;

        void setSeed(int x) { _seed = x; }
        int getSeed() const { return _seed; }

        void setLow(int x) { _low = x; }
        int getLow() const { return _low; }

        void setDescription(const QString &desc) { _description = desc; }
        QString getDescription() const { return _description; }

        bool isAbsolute() const;
        bool hasPhaseOrEvent() const;
        bool canHavePhasesOrEvents() const;

        static bool LessThanForStarts(const Season &a, const Season &b);

        QList<Phase> phases;
        QList<SeasonEvent> events;

    protected:
        QString name; // name, typically users name them by year e.g. "2011 Season"
        QUuid _id; // unique id
        int type = season;
        QDate _absoluteStart; // first day (fixed season)
        QDate _absoluteEnd; // last day (fixed season)
        SeasonOffset _offsetStart; // offset of the start (relative season)
        SeasonOffset _offsetEnd; // offset of the end (relative season)
        SeasonLength _length; // length (relative season)
        bool _ytd = false;
        int _seed = 0;
        int _low = -50; // low point for SB .. default to -50
        QString _description;

        ModelType _model = ModelType::none;
        Qt::DayOfWeek _firstDayOfWeek = Qt::Monday;
};


class Microcycle
{
public:
    Microcycle(int freq = 0, int load = 0, float intensity = 0.0f, int time = 0, const QString &description = "");

    bool setFrequency(int freq);
    int getFrequency() const;

    bool setLoad(int load);
    int getLoad() const;

    bool setIntensity(float intensity);
    float getIntensity() const;

    bool setTime(int time);
    int getTime() const;

    void setDescription(const QString &description);
    QString getDescription() const;

private:
    int freq = 0;
    int load = 0;
    float intensity = 0.0f;
    int time = 0;
    QString desc;
};


class Slot
{
public:
    enum class StatusType : int {
        generated = 0,
        adjusted
    };

    static QString statusToString(StatusType status);
    static StatusType stringToStatus(const QString &str);

    Slot(int dayOffset = 0, const QString &sport = "", const StatusType status = StatusType::generated, int load = 0, float intensity = 0.0f, int time = 0, const QString &description = "");

    bool setDayOffset(int offset);
    int getDayOffset() const;

    bool setSport(const QString &sport);
    QString getSport() const;

    bool setStatus(const StatusType &status);
    StatusType getStatus() const;

    bool setLoad(int load);
    int getLoad() const;
    bool hasLoad() const;

    bool setIntensity(float intensity);
    float getIntensity() const;
    bool hasIntensity() const;

    bool setTime(int time);
    int getTime() const;
    bool hasTime() const;

    bool setDescription(const QString &description);
    QString getDescription() const;
    bool hasDescription() const;

    const QVarLengthArray<QString, 1> &getActivities() const;
    bool hasActivity(const QString &id) const;
    void addActivity(const QString &id);
    void setActivities(const QStringList &ids);
    bool removeActivity(const QString &id);
    void clearActivities();

private:
    int dayOffset = 0;
    QString sport;
    StatusType status = StatusType::generated;
    int load = 0;
    float intensity = 0;
    int time = 0;
    QString description;
    QVarLengthArray<QString, 1> activityIds;
};


class Mesocycle {
public:
    bool setStart(const QDate &date);

    ////////////////////////////////////////////////////////////
    // mesocycles targets
    void setTargetLTS(int targetLTS);
    int getTargetLTS() const;

    void setTargetCP(int targetCP);
    int getTargetCP() const;

    void setTargetFTP(int targetFTP);
    int getTargetFTP() const;

    ////////////////////////////////////////////////////////////
    // microcycles and their aggregates
    int toMicrocycle(const QDate &date) const;
    int toMicrocycle(int offset) const;
    bool setMaxMicrocycles(int maxMicrocycles);
    int getMaxMicrocycles() const;

    bool hasMicrocycle(int microcycle) const;
    Microcycle *getMicrocycle(int microcycle);
    Microcycle const *getMicrocycle(int microcycle) const;

    int getMicrocyclesFrequency() const;
    int getMicrocyclesLoad() const;
    float getMicrocyclesIntensity() const;
    int getMicrocyclesTime() const;

    ////////////////////////////////////////////////////////////
    // slots and their targets (aggregated or singular)
    int getSlottedFrequency() const;
    int getSlottedFrequency(const QString &sport) const;
    int getSlottedFrequency(int microcycle) const;
    int getSlottedFrequency(int microcycle, const QString &sport) const;

    int getSlottedLoad() const;
    int getSlottedLoad(const QString &sport) const;
    int getSlottedLoad(int microcycle) const;
    int getSlottedLoad(int microcycle, const QString &sport) const;

    float getSlottedIntensity() const;
    float getSlottedIntensity(const QString &sport) const;
    float getSlottedIntensity(int microcycle) const;
    float getSlottedIntensity(int microcycle, const QString &sport) const;

    int getSlottedTime() const;
    int getSlottedTime(const QString &sport) const;
    int getSlottedTime(int microcycle) const;
    int getSlottedTime(int microcycle, const QString &sport) const;

    bool hasSlot(const QDate &when, const QString &sport) const;
    bool hasSlot(int offset, const QString &sport) const;
    Slot *getSlot(const QDate &when, const QString &sport);
    Slot const *getSlot(const QDate &when, const QString &sport) const;
    Slot *getSlot(int offset, const QString &sport);
    Slot const *getSlot(int offset, const QString &sport) const;
    Slot *addSlot(const QDate &when, const QString &sport);
    Slot *addSlot(int offset, const QString &sport);
    void delSlot(int offset, const QString &sport);
    QMap<QString, QList<int>> getSlotSportOffsets() const;
    QMap<QString, QList<QDate>> getSlotSportDates() const;
    QMap<int, QList<QString>> getSlotOffsetSports() const;
    QMap<QDate, QList<QString>> getSlotDateSports() const;

private:
    QDate start;
    int targetLTS = 0;
    int targetCP = 0;
    int targetFTP = 0;
    int maxMicrocycles = 0;
    Microcycle dummyMicrocycle;
    QMap<int, Microcycle> microcycles;
    QList<Slot> slots_;
};


class Phase : public Season
{
    Q_DECLARE_TR_FUNCTIONS(Phase)

    public:
        static QList<QString> types;
        enum PhaseType { phase=100, prep=101, base=102, build=103, peak=104, camp=120, mesocycle=121 };

        Phase();
        Phase(QString _name, QDate _start, QDate _end);

        void setType(int _type) override;
        void setAbsoluteStart(QDate _start) override;
        void setAbsoluteEnd(QDate _end) override;

        bool hasMesocycle() const;
        Mesocycle *getMesocycle();
        Mesocycle const *getMesocycle() const;
        int numMicrocycles() const;

    private:
        std::optional<Mesocycle> _mesocycle;

        Mesocycle *addMesocycle();
};

#endif /* SEASON_H_ */
