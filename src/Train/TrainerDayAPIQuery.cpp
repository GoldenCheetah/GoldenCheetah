/*
 * Copyright (c) 2024 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#include "TrainerDayAPIQuery.h"

#include <QString>
#include <QList>
#include <QMap>
#include <QSet>
#include <QPair>
#include <QDebug>


extern int TrainerDayAPIQuerylex();
extern char *TrainerDayAPIQuerytext;

extern void TrainerDayAPIQuery_setString(QString);
extern void TrainerDayAPIQuery_clearString();

extern int TrainerDayAPIQueryparse();

QUrlQuery trainerDayAPIQuery;
QString trainerDayAPIQueryErrorMsg;


extern QUrlQuery
parseTrainerDayAPIQuery
(QString text, bool &ok, QString &msg)
{
    trainerDayAPIQuery.clear();
    trainerDayAPIQueryErrorMsg.clear();
    TrainerDayAPIQuery_setString(text);
    msg = trainerDayAPIQueryErrorMsg;
    ok = (TrainerDayAPIQueryparse() == 0);
    if (ok) {
        QList<QPair<QString, QString>> items = trainerDayAPIQuery.queryItems();
        QMap<QString, int> um;
        for (const QPair<QString, QString> &p : items) {
            if (um.contains(p.first)) {
                ok = false;
                ++um[p.first];
            } else {
                um[p.first] = 1;
            }
        }
        if (! ok) {
            QSet<QString> reverseCollector;
            for (auto it = um.keyValueBegin(); it != um.keyValueEnd(); ++it) {
                if (it->second > 1) {
                    if (it->first.endsWith("Minutes")) {
                        reverseCollector.insert("duration");
                    } else {
                        reverseCollector.insert(it->first);
                    }
                }
            }
            if (msg.length() > 0) {
                msg += "; ";
            }
            msg += "Duplicate queries " + reverseCollector.values().join(", ");
        }
    }
    if (ok) {
        return trainerDayAPIQuery;
    } else {
        return QUrlQuery();
    }
}


extern QStringList
trainerDayAPIQueryCommands
()
{
    QStringList queryCommands;
    queryCommands << "duration"
                   << "dominantzone";
    queryCommands.sort(Qt::CaseInsensitive);
    return queryCommands;
}
