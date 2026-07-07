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

#ifndef _seasons_h
#define _seasons_h

#include <QList>
#include <QDir>
#include <QFile>
#include <QXmlStreamReader>
#include <QDate>

#include "Season.h"


class Seasons : public QObject
{
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


class SeasonParser
{
public:
    static QList<Season> readSeasons(QFile * const file);
    static bool serialize(QString filename, QList<Season> seasons);

private:
    static Season parseSeason(QXmlStreamReader &reader);
    static Phase parsePhase(QXmlStreamReader &reader);
    static QDate parseDate(QString);
    static SeasonOffset parseOffset(QString offsetStr);
    static SeasonLength parseLength(QString lengthStr);
};

#endif //Seasons
