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
#include <QFile>
#include <QApplication>

#include "Context.h"

class SeasonEvent
{
    public:
        SeasonEvent(QString name, QDate date) : name(name), date(date) {}

        QString name;
        QDate date;
};

class Season
{
    Q_DECLARE_TR_FUNCTIONS(Season)

	public:
        static QList<QString> types;
        enum SeasonType { season=0, cycle=1, adhoc=2, temporary=3 };
        //typedef enum seasontype SeasonType;

		Season();
        QDate getStart();
        QDate getEnd();
        int getSeed() { return _seed; }
        int getLow() { return _low; }
        QString getName();
        int days() { return _days; } // how many days in the season, -1 if never ending
        int getType();


        void setStart(QDate _start);
        void setEnd(QDate _end);
        void setName(QString _name);
        void setType(int _type);
        void setSeed(int x) { _seed = x; }
        void setLow(int x) { _low = x; }
        QUuid id() const { return _id; }
        void setId(QUuid x) { _id = x; }
        QVector<int> &load() { return _load; }

        QDate start; // first day of the season
        QDate end; // last day of the season
        int _days; // how many days in this season?
        int _seed;
        int _low; // low point for SB .. default to -50
        QUuid _id; // unique id

        QString name; // name, typically users name them by year e.g. "2011 Season"
        int type;

        QVector<int> _load; // array of daily planned load
        QList<SeasonEvent> events;
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

    private:
        Context *context;
        Season *season;

        QPushButton *applyButton, *cancelButton;
        QLineEdit *nameEdit;
        QComboBox *typeEdit;
        QDateEdit *fromEdit, *toEdit;
        QDoubleSpinBox *seedEdit;
        QDoubleSpinBox *lowEdit;
};

class EditSeasonEventDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:
        EditSeasonEventDialog(Context *, SeasonEvent *);

    public slots:
        void applyClicked();
        void cancelClicked();

    private:
        Context *context;
        SeasonEvent *event;

        QPushButton *applyButton, *cancelButton;
        QLineEdit *nameEdit;
        QDateEdit *dateEdit;
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
        SeasonTreeView();

    signals:
        void itemMoved(QTreeWidgetItem* item, int previous, int actual);

    protected:
        void dropEvent(QDropEvent* event);


};


#endif /* SEASON_H_ */
