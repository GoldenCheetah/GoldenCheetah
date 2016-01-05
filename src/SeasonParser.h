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

#ifndef _seasonparser_h
#define _seasonparser_h
#include "GoldenCheetah.h"

#include <QXmlDefaultHandler>
#include "Season.h"

class SeasonParser : public QXmlDefaultHandler
{

public:
    // marshall
    static bool serialize(QString, QList<Season>);

    // unmarshall
    bool startDocument();
    bool endDocument();
    bool endElement( const QString&, const QString&, const QString &qName );
    bool startElement( const QString&, const QString&, const QString &name, const QXmlAttributes &attrs );
    bool characters( const QString& str );
    QList<Season> getSeasons();

protected:
    QString buffer;
    QDate seasonDateToDate(QString);
    Season season;
    QList<Season> seasons;

    bool isPhase;
    Phase phase;
    int loadcount;
    QString dateString;

};
#endif //SeasonParser
