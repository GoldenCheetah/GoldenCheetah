/*
 * Copyright (c) 2015 Joern Rischmueller (joern.rm@gmail.com)
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

#ifndef _Gc_BodyMeasures_h
#define _Gc_BodyMeasures_h

#include "Context.h"

#include <QString>
#include <QStringList>
#include <QDateTime>

#define BODY_WEIGHT_KG           0
#define BODY_WEIGHT_FAT_KG       1
#define BODY_WEIGHT_MUSCLE_KG    2
#define BODY_WEIGHT_BONES_KG     3
#define BODY_WEIGHT_LEAN_KG      4
#define BODY_WEIGHT_FAT_PERCENT  5

class BodyMeasure {

public:
    BodyMeasure() : when(QDateTime()), comment(""), weightkg(0), fatkg(0), musclekg(0), boneskg(0), leankg(0), fatpercent(0) {}

    // depending on datasource not all fields may be filled with actual values

    QDateTime when;         // when was this reading taken
    QString comment;        // user commentary regarding this measurement

    double  weightkg,       // weight in Kilograms
            fatkg,          // fat in Kilograms
			musclekg,       // muscles in Kilograms
			boneskg,        // bones in Kilograms
            leankg,         // lean mass in Kilograms
            fatpercent;     // body fat as a percentage of weight

    // used by qSort()
    bool operator< (BodyMeasure right) const {
        return (when < right.when);
    }
};

class BodyMeasureParser  {

public:
    static bool serialize(QString, QList<BodyMeasure> &);
    static bool unserialize(QFile &, QList<BodyMeasure> &);
};


#endif
