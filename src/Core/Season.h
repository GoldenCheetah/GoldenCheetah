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
#include "GoldenCheetah.h"

#include <QString>
#include <QDate>
#include <QDialog>
#include <QFile>
#include <QApplication>
#include <QTreeWidget>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>

#include "Context.h"

class Phase;

class SeasonEvent
{
    Q_DECLARE_TR_FUNCTIONS(Season)

    public:
        static QStringList priorityList();

        SeasonEvent(QString name, QDate date, int priority=0, QString description="", QString id="") : name(name), date(date), priority(priority), description(description), id(id) {}

        QString name;
        QDate date;
        int priority;
        QString description;
        QString id; // unique id
};

class SeasonOffset
{
    public:
        // by default, the offset is invalid
        SeasonOffset();

        // if different from the default, the offset represent the start
        // of the season with respect to the year/month/week that
        // contains the reference date (i.e. either the current date, or
        // the date of the selected activity)
        SeasonOffset(int _years, int _months, qint64 _weeks);

        // get the start of the season, or QDate() if the offset is
        // invalid
        QDate getStart(QDate reference) const;

    private:
        // if <=0, offset in years of the season with respect to the
        // current year
        // if >0, unused: go to `months`
        int years;

        // if <=0, offset in months of the season with respect to the
        // current month
        // if >0, unused: go to `weeks`
        int months;

        // if <=0, offset in weeks of the season with respect to the
        // current week (Qt weeks start on Monday)
        // if >0, the whole offset is unused (either the season is
        // fixed, or it ends on the reference day)
        qint64 weeks;

};

class SeasonLength
{

    public:
        // by default, the length is invalid
        SeasonLength();

        // if different from the default, the length represent the
        // number of years, months and days from the start to the end,
        // included (i.e. if start==end, the length is (0,0,1))
        SeasonLength(int _years, int _months, qint64 _days);

        bool operator==(const SeasonLength& length);

        QDate addTo(QDate start) const;
        QDate substractFrom(QDate end) const;

    private:
        // if (0,0,0), the length is invalid (the season is fixed)
        int years;
        int months;
        qint64 days;

};

class Season
{
    Q_DECLARE_TR_FUNCTIONS(Season)

    public:
        static QList<QString> types;
        enum SeasonType { season=0, cycle=1, adhoc=2, temporary=3, yearToDate=4 };
        //typedef enum seasontype SeasonType;

        Season();

        // get the date range (if relative, use today as reference)
        QDate getStart() const ;
        QDate getEnd() const ;
        // get the date range (if relative, use the specified date as reference)
        QDate getStart(QDate reference) const;
        QDate getEnd(QDate reference) const;

        int getSeed() { return _seed; }
        int getLow() { return _low; }
        int getMaxRamp() { return _ramp; }
        QString getName();

        // length of a relative season, SeasonLength() if fixed
        SeasonLength getLength() const { return _length; }

        int getType() const;
        static bool LessThanForStarts(const Season &a, const Season &b);

        // make the season fixed (by invalidating _offset and _length)
        // and set the limits of a fixed season
        void setStart(QDate _start);
        void setEnd(QDate _end);

        // make the season relative (by setting _length; also
        // invalidates _start and _end) and set the offset of the start
        // with respect to the reference
        void setOffsetAndLength(int offetYears, int offsetMonths, qint64 offsetWeeks, int years, int months, qint64 days);

        // make the season relative (by setting _length; also
        // invalidates _start and _end) and invalidates the offset to
        // make the season end on the reference
        void setLength(int years, int months, qint64 days);

        void setName(QString _name);
        void setType(int _type);
        void setSeed(int x) { _seed = x; }
        void setLow(int x) { _low = x; }
        void setMaxRamp(int x) { _ramp = x; }
        QUuid id() const { return _id; }
        void setId(QUuid x) { _id = x; }
        QVector<int> &load() { return _load; }

        int type;

        QString name; // name, typically users name them by year e.g. "2011 Season"

        QVector<int> _load; // array of daily planned load

        QList<Phase> phases;
        QList<SeasonEvent> events;

    protected:
        SeasonOffset _offset; // offset of the start (relative season)
        SeasonLength _length; // length (relative season)
        QDate _start; // first day (fixed season + year to date)
        QDate _end; // last day (fixed season)
        int _seed;
        int _low; // low point for SB .. default to -50
        int _ramp; // max ramp rate for CTL we want to see
        QUuid _id; // unique id

};

class EditSeasonDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:
        EditSeasonDialog(Context *, Season *);

    public slots:
        void applyClicked();
        void cancelClicked();
        void nameChanged();

    private:
        Context *context;
        Season *season;

        QLabel *to;
        QPushButton *applyButton, *cancelButton;
        QLineEdit *nameEdit;
        QComboBox *typeEdit;
        QDateEdit *fromEdit, *toEdit;
        QDoubleSpinBox *seedEdit;
        QDoubleSpinBox *lowEdit;

    public slots:
        void typeChanged();
};

class EditSeasonEventDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:
        EditSeasonEventDialog(Context *, SeasonEvent *, Season &);

    public slots:
        void applyClicked();
        void cancelClicked();
        void nameChanged();

    private:
        Context *context;
        SeasonEvent *event;

        QPushButton *applyButton, *cancelButton;
        QLineEdit *nameEdit;
        QDateEdit *dateEdit;
        QComboBox *priorityEdit;
        QTextEdit *descriptionEdit;
};

class Seasons : public QObject {

    Q_OBJECT;

    public:
        Seasons(QDir home) : home(home) { readSeasons(); }
        void readSeasons();
        int newSeason(QString name, QDate start, QDate end, int type);
        void updateSeason(int index, QString name, QDate start, QDate end, int type);
        void deleteSeason(int);
        void writeSeasons();
        QList<Season> seasons;

    signals:
        void seasonsChanged();


    private:
        QDir home;
};

class SeasonTreeView : public QTreeWidget
{
    Q_OBJECT

    public:
        SeasonTreeView(Context *);

        // for drag/drop
        QStringList mimeTypes () const;
        QMimeData * mimeData ( const QList<QTreeWidgetItem *> items ) const;

    signals:
        void itemMoved(QTreeWidgetItem* item, int previous, int actual);

    protected:
        void dragEnterEvent(QDragEnterEvent* event);
        void dropEvent(QDropEvent* event);
        Context *context;


};

class Phase : public Season
{
    Q_DECLARE_TR_FUNCTIONS(Phase)

    public:
        static QList<QString> types;
        enum PhaseType { phase=100, prep=101, base=102, build=103, peak=104, camp=120 };

        Phase();
        Phase(QString _name, QDate _start, QDate _end);

};

class EditPhaseDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:
        EditPhaseDialog(Context *, Phase *, Season &);

    public slots:
        void applyClicked();
        void cancelClicked();
        void nameChanged();

    private:
        Context *context;
        Phase *phase;

        QPushButton *applyButton, *cancelButton;
        QLineEdit *nameEdit;
        QComboBox *typeEdit;
        QDateEdit *fromEdit, *toEdit;
        QDoubleSpinBox *seedEdit;
        QDoubleSpinBox *lowEdit;
};

class SeasonEventTreeView : public QTreeWidget
{
    Q_OBJECT

    public:
        SeasonEventTreeView();

    signals:
        void itemMoved(QTreeWidgetItem* item, int previous, int actual);

    protected:
        void dropEvent(QDropEvent* event);
};
#endif /* SEASON_H_ */
