/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
 * Copyright (c) 2015 Vianney Boyer   (vlcvboyer@gmail.com)
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

#include "ErgFileBase.h"

#include <QTextDocument>


ErgFileBase::ErgFileBase
() : _format(ErgFileFormat::unknown), _Version(), _Units(), _originalFilename(), _filename(), _Name(), _Description(), _TrainerDayId(), _Source(), _Tags(), _Duration(0),
     _Ftp(0), _MinWatts(0), _MaxWatts(0), _mode(ErgFileFormat::unknown), _StrictGradient(false),
     _fHasGPS(false), _minY(0), _maxY(0), _CP(0), _AP(0), _IsoPower(0), _IF(0), _BikeStress(0), _VI(0), _XP(0), _RI(0), _BS(0), _SVI(0), _ele(0), _eleDist(0),
     _grade(0), _powerZonesPC{}, _dominantZone(ErgFilePowerZone::unknown), _numZones(0)
{
}


ErgFileBase::~ErgFileBase
()
{
}


const QList<ErgFilePowerZone>&
ErgFileBase::powerZoneList
()
{
    static QList<ErgFilePowerZone> _powerZoneList = QList<ErgFilePowerZone>({
        ErgFilePowerZone::z1,
        ErgFilePowerZone::z2,
        ErgFilePowerZone::z3,
        ErgFilePowerZone::z4,
        ErgFilePowerZone::z5,
        ErgFilePowerZone::z6,
        ErgFilePowerZone::z7,
        ErgFilePowerZone::z8,
        ErgFilePowerZone::z9,
        ErgFilePowerZone::z10
    });
    return _powerZoneList;
}


bool
ErgFileBase::hasGradient
() const
{
    return ErgFileFormat::crs == _format;
}


bool
ErgFileBase::hasWatts
() const
{
    return ErgFileFormat::erg == _format || ErgFileFormat::mrc == _format;
}


bool
ErgFileBase::hasAbsoluteWatts
() const
{
    return ErgFileFormat::erg == _format;
}


bool
ErgFileBase::hasRelativeWatts
() const
{
    return ErgFileFormat::mrc == _format;
}


ErgFileType
ErgFileBase::type
() const
{
    switch (_format) {
    case ErgFileFormat::crs:
        return ErgFileType::slp;
    case ErgFileFormat::erg:
    case ErgFileFormat::erg2:
    case ErgFileFormat::mrc:
        return ErgFileType::erg;
    case ErgFileFormat::code:
        return ErgFileType::code;
    default:
        return ErgFileType::unknown;
    }
}

QString
ErgFileBase::typeString
() const
{
    switch (type()) {
    case ErgFileType::slp:
        return "slp";
    case ErgFileType::erg:
        return "erg";
    case ErgFileType::code:
        return "code";
    default:
        return "unknown";
    }
}

QString
ErgFileBase::ergSubTypeString
() const
{
    if (hasAbsoluteWatts()) {
        return "abs";
    } else if (hasRelativeWatts()) {
        return "rel";
    }
    return QString();
}


bool
ErgFileBase::hasGPS
() const
{
    return _fHasGPS;
}


ErgFileFormat
ErgFileBase::format
() const
{
    return _format;
}

QString
ErgFileBase::formatString
() const
{
    switch (_format) {
    case ErgFileFormat::erg:
        return "ERG";
    case ErgFileFormat::mrc:
        return "MRC";
    case ErgFileFormat::crs:
        return "CRS";
    case ErgFileFormat::erg2:
        return "ERG";
    case ErgFileFormat::code:
        return "CODE";
    case ErgFileFormat::unknown:
        return "UNKNOWN";
    default:
        return "UNDEFINED";
    }
}

void
ErgFileBase::format
(ErgFileFormat format)
{
    _format = format;
}


QString
ErgFileBase::version
() const
{
    return _Version;
}

void
ErgFileBase::version
(QString version)
{
    _Version = version;
}


QString
ErgFileBase::units
() const
{
    return _Units;
}

void
ErgFileBase::units
(QString units)
{
    _Units = units;
}


QString
ErgFileBase::filename
() const
{
    return _filename;
}

void
ErgFileBase::filename
(QString filename)
{
    _filename = filename;
}


QString
ErgFileBase::originalFilename
() const
{
    return _originalFilename;
}

void
ErgFileBase::originalFilename
(QString originalFilename)
{
    _originalFilename = originalFilename;
}


QString
ErgFileBase::name
() const
{
    return _Name;
}

void
ErgFileBase::name
(QString name)
{
    QTextDocument doc;
    doc.setHtml(name);
    _Name = doc.toPlainText();
}


QString
ErgFileBase::description
() const
{
    return _Description;
}

void
ErgFileBase::description
(QString description)
{
    QTextDocument doc;
    doc.setHtml(description);
    _Description = doc.toPlainText();
}


QString
ErgFileBase::trainerDayId
() const
{
    return _TrainerDayId;
}

void
ErgFileBase::trainerDayId
(QString trainerDayId)
{
    _TrainerDayId = trainerDayId;
}


QString
ErgFileBase::source
() const
{
    return _Source;
}

void
ErgFileBase::source
(QString source)
{
    _Source = source;
}


QStringList
ErgFileBase::tags
() const
{
    return _Tags;
}

void
ErgFileBase::tags
(QStringList tags)
{
    _Tags = tags;
}


long
ErgFileBase::duration
() const
{
    return _Duration;
}

void
ErgFileBase::duration
(long duration)
{
    _Duration = duration;
}


int
ErgFileBase::ftp
() const
{
    return _Ftp;
}

void
ErgFileBase::ftp
(int ftp)
{
    _Ftp = ftp;
}


int
ErgFileBase::minWatts
() const
{
    return _MinWatts;
}

void
ErgFileBase::minWatts
(int minWatts)
{
    _MinWatts = minWatts;
}


int
ErgFileBase::maxWatts
() const
{
    return _MaxWatts;
}

void
ErgFileBase::maxWatts
(int maxWatts)
{
    _MaxWatts = maxWatts;
}


ErgFileFormat
ErgFileBase::mode
() const
{
    return _mode;
}

void
ErgFileBase::mode
(ErgFileFormat mode)
{
    _mode = mode;
}


bool
ErgFileBase::strictGradient
() const
{
    return _StrictGradient;
}

void
ErgFileBase::strictGradient
(bool strictGradient)
{
    _StrictGradient = strictGradient;
}


bool
ErgFileBase::fHasGPS
() const
{
    return _fHasGPS;
}

void
ErgFileBase::fHasGPS
(bool hasGPS)
{
    _fHasGPS = hasGPS;
}


void
ErgFileBase::resetMetrics
()
{
    _XP = 0;
    _CP = 0;
    _AP = 0;
    _IsoPower = 0;
    _IF = 0;
    _RI = 0;
    _BikeStress = 0;
    _BS = 0;
    _SVI = 0;
    _VI = 0;
    _ele = 0;
    _eleDist = 0;
    _grade = 0;

    _minY = 0;
    _maxY = 0;
}


double
ErgFileBase::minY
() const
{
    return _minY;
}

void
ErgFileBase::minY
(double minY)
{
    _minY = minY;
}


double
ErgFileBase::maxY
() const
{
    return _maxY;
}

void
ErgFileBase::maxY
(double maxY)
{
    _maxY = maxY;
}


double
ErgFileBase::CP
() const
{
    return _CP;
}

void
ErgFileBase::CP
(double cp)
{
    _CP = cp;
}


double
ErgFileBase::AP
() const
{
    return _AP;
}

void
ErgFileBase::AP
(double ap)
{
    _AP = ap;
}


double
ErgFileBase::IsoPower
() const
{
    return _IsoPower;
}

void
ErgFileBase::IsoPower
(double isoPower)
{
    _IsoPower = isoPower;
}


double
ErgFileBase::IF
() const
{
    return _IF;
}

void
ErgFileBase::IF
(double IF)
{
    _IF = IF;
}


double
ErgFileBase::bikeStress
() const
{
    return _BikeStress;
}

void
ErgFileBase::bikeStress
(double bikeStress)
{
    _BikeStress = bikeStress;
}


double
ErgFileBase::VI
() const
{
    return _VI;
}

void
ErgFileBase::VI
(double vi)
{
    _VI = vi;
}


double
ErgFileBase::XP
() const
{
    return _XP;
}

void
ErgFileBase::XP
(double xp)
{
    _XP = xp;
}


double
ErgFileBase::RI
() const
{
    return _RI;
}

void
ErgFileBase::RI
(double ri)
{
    _RI = ri;
}


double
ErgFileBase::BS
() const
{
    return _BS;
}

void
ErgFileBase::BS
(double bs)
{
    _BS = bs;
}


double
ErgFileBase::SVI
() const
{
    return _SVI;
}

void
ErgFileBase::SVI
(double svi)
{
    _SVI = svi;
}


long
ErgFileBase::distance
() const
{
    if (hasGradient()) {
        return duration();
    }
    return 0;
}


double
ErgFileBase::minElevation
() const
{
    if (hasGradient()) {
        return minY();
    }
    return 0;
}


double
ErgFileBase::maxElevation
() const
{
    if (hasGradient()) {
        return maxY();
    }
    return 0;
}


double
ErgFileBase::ele
() const
{
    return _ele;
}

void
ErgFileBase::ele
(double ele)
{
    _ele = ele;
}


double
ErgFileBase::eleDist
() const
{
    return _eleDist;
}

void
ErgFileBase::eleDist
(double eleDist)
{
    _eleDist = eleDist;
}


double
ErgFileBase::grade
() const
{
    return _grade;
}

void
ErgFileBase::grade
(double grade)
{
    _grade = grade;
}


double
ErgFileBase::powerZonePC
(ErgFilePowerZone zone) const
{
    return _powerZonesPC[static_cast<int>(zone)];
}

QList<double>
ErgFileBase::powerZonesPC
() const
{
    QList<double> ret;
    for (int i = 1; i <= numZones(); ++i) {
        ret << _powerZonesPC[i];
    }
    return ret;
}

void
ErgFileBase::powerZonePC
(ErgFilePowerZone zone, double pc)
{
    _powerZonesPC[static_cast<int>(zone)] = pc;
}


ErgFilePowerZone
ErgFileBase::dominantZone
() const
{
    return _dominantZone;
}

int
ErgFileBase::dominantZoneInt
() const
{
    return static_cast<int>(_dominantZone);
}

void
ErgFileBase::dominantZone
(ErgFilePowerZone zone)
{
    _dominantZone = zone;
}

void
ErgFileBase::dominantZone
(int zone)
{
    switch(zone) {
    case 1:
        _dominantZone = ErgFilePowerZone::z1;
        break;
    case 2:
        _dominantZone = ErgFilePowerZone::z2;
        break;
    case 3:
        _dominantZone = ErgFilePowerZone::z3;
        break;
    case 4:
        _dominantZone = ErgFilePowerZone::z4;
        break;
    case 5:
        _dominantZone = ErgFilePowerZone::z5;
        break;
    case 6:
        _dominantZone = ErgFilePowerZone::z6;
        break;
    case 7:
        _dominantZone = ErgFilePowerZone::z7;
        break;
    case 8:
        _dominantZone = ErgFilePowerZone::z8;
        break;
    case 9:
        _dominantZone = ErgFilePowerZone::z9;
        break;
    case 10:
        _dominantZone = ErgFilePowerZone::z10;
        break;
    default:
        _dominantZone = ErgFilePowerZone::unknown;
        break;
    }
}


int
ErgFileBase::numZones
() const
{
    return _numZones;
}

void
ErgFileBase::numZones
(int nz)
{
    _numZones = nz;
}
