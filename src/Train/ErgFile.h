/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _ErgFile_h
#define _ErgFile_h
#include "GoldenCheetah.h"
#include "Context.h"

#include <QtGui>
#include <QObject>
#include <QDebug>
#include <QString>
#include <QDate>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QTextEdit>
#include <QRegExp>
#include "Zones.h"      // For zones ... see below vvvv
#include "LocationInterpolation.h"
#include "Settings.h"

// which section of the file are we in?
#define NOMANSLAND  0
#define SETTINGS 1
#define DATA     2
#define END      3
#define TEXTS    4

// is this in .erg or .mrc format?
#define ERG     1
#define MRC     2
#define CRS     3
#define ERG2    4

class ErgFilePoint
{
    public:

        ErgFilePoint() : x(0), y(0), val(0), lat(0), lon(0) {}
        ErgFilePoint(double x_, double y_, double val_) : x(x_), y(y_), val(val_), lat(0), lon(0) {}
        ErgFilePoint(double x_, double y_, double val_, double lat_, double lon_) : x(x_), y(y_), val(val_), lat(lat_), lon(lon_) {}

        double x;     // x axis - time in msecs or distance in meters
        double y;     // y axis - load in watts or altitude

        double val;   // the value to send to the device (watts/gradient)

        double lat, lon; // location (alt is y, above)
};

class ErgFileSection
{
    public:
        ErgFileSection() : duration(0), start(0), end(0) {}
        ErgFileSection(int duration, int start, int end) : duration(duration), start(start), end(end) {}

        int duration;
        double start, end;
};

class ErgFileZoneSection
: public ErgFileSection
{
    public:
        ErgFileZoneSection() : ErgFileSection(), startValue(0), endValue(0), zone(0) {}
        ErgFileZoneSection(int startMSecs, int startValue, int endMSecs, int endValue, int zone)
         : ErgFileSection(endMSecs - startMSecs, startMSecs, endMSecs), startValue(startValue), endValue(endValue), zone(zone)
        {}

        int startValue;
        int endValue;
        int zone;
};

class ErgFileText
{
    public:
        ErgFileText() : x(0), duration(0), text("") {}
        ErgFileText(double x, int duration, const QString &text) : x(x), duration(duration), text(text) {}

        double x;
        int duration;
        QString text;
};

class ErgFileLap
{
    public:
        ErgFileLap() : name(""), x(0), LapNum(0), lapRangeId(0), selected(false) {}
        ErgFileLap(double x, int LapNum, const QString& name) : name(name), x(x), LapNum(LapNum), lapRangeId(0), selected(false) {}
        ErgFileLap(double x, int LapNum, int lapRangeId, const QString& name) : name(name), x(x), LapNum(LapNum), lapRangeId(lapRangeId), selected(false) {}

        QString name;
        double x;      // when does this LAP marker occur? (time in msecs or distance in meters
        int LapNum;    // from 1 - n
        int lapRangeId;// for grouping lap markers into ranges. Value of zero is considered 'ungrouped'.
        bool selected; // used by the editor
};

class ErgFile
{
    public:
        ErgFile(QString, int, Context *context);       // constructor uses filename
        ErgFile(Context *context); // no filename, going to use a string

        ~ErgFile();             // delete the contents

        void finalize();        // finish up ergfile creation

        void setFrom(ErgFile *f); // clone an existing workout
        bool save(QStringList &errors); // save back, with changes

        static ErgFile *fromContent(QString, Context *); // read from memory *.erg
        static ErgFile *fromContent2(QString, Context *); // read from memory *.erg2

        static bool isWorkout(QString);  // is this a supported workout?

        void reload();                   // reload after messed about

        void parseComputrainer(QString p = ""); // its an erg,crs or mrc file
        void parseTacx();                // its a pgmf file
        void parseZwift();               // its a zwo file (zwift xml)
        void parseFromRideFileFactory(); // its something we can parse using ridefilefactory...
        void parseErg2(QString p = "");  // ergdb
        void parseTTS();                 // its ahh tts

        bool isValid() const;            // is the file valid or not?

        double Cp;
        int format;             // ERG, CRS, MRC, ERG2 currently supported

        bool hasGradient() const { return CRS == format; } // Has Gradient and Altitude
        bool hasWatts()    const { return ERG == format || MRC == format; }
        bool hasGPS()      const { return fHasGPS; } // Has Lat/Lon

private:
        void sortLaps() const;
        void sortTexts() const;

        bool coalescedSections = false;

public:
        void coalesceSections();
        bool hasCoalescedSections() const;

        double nextLap(double) const;    // return the start value (erg - time(ms) or slope - distance(m)) for the next lap
        double prevLap(double) const;    // return the start value (erg - time(ms) or slope - distance(m)) for the prev lap
        double currentLap(double) const; // return the start value (erg - time(ms) or slope - distance(m)) for the current lap

        int    addNewLap(double loc) const; // creates new lap at location, returns index of new lap.

        bool textsInRange(double searchStart, double searchRange, int& rangeStart, int& rangeEnd) const;

        // turn the ergfile into a series of sections rather
        // than a list of points
        QList<ErgFileSection> Sections();
        QList<ErgFileZoneSection> ZoneSections();

        QString Version,        // version number / identifer
                Units,          // units used
                Filename,       // filename from inside file
                filename,       // filename on disk
                Name,           // description in file
                Description,    // long narrative for workout
                ErgDBId,        // if downloaded from ergdb
                Source;         // where did this come from
        QStringList Tags;       // tagged strings

        long    Duration;       // Duration of this workout in msecs
        int     Ftp;            // FTP this file was targetted at
        int     MaxWatts;       // maxWatts in this ergfile (scaling)
        bool    valid;          // did it parse ok?
        int     mode;
        bool    StrictGradient; // should gradient be strict or smoothed?
        bool    fHasGPS;        // has Lat/Lon?

        QList<ErgFilePoint>         Points; // points in workout
        mutable QList<ErgFileLap>   Laps;   // interval markers in the file
        mutable QList<ErgFileText>  Texts;  // texts to display

        GeoPointInterpolator gpi;      // Location interpolator

        void calculateMetrics();       // calculate IsoPower value for ErgFile

        // Metrics for this workout
        double minY, maxY;             // minimum and maximum Y value
        double CP;
        double AP, IsoPower, IF, BikeStress, VI; // Coggan for erg / mrc
        double XP, RI, BS, SVI;        // Skiba for erg / mrc
        double ELE, ELEDIST, GRADE;    // crs

        Context *context;
};

// Store state used for location query external to ergfile. This permits sharing
// to simulataniusly query multiple locations.
class ErgFileQueryAdapter {

    mutable struct ErgFileLocationQueryState
    {
        int leftPoint, rightPoint;     // current points we are between
        int interpolatorReadIndex;     // next point to be fed to interpolator
        GeoPointInterpolator gpi;      // Location interpolator

        ErgFileLocationQueryState() {
            Reset();
        }

        void Reset() {
            leftPoint = 0;
            rightPoint = 1;
            interpolatorReadIndex = 0;
            gpi.Reset();
        }
    } qs;

    const ErgFile* ergFile;

public:

    ErgFileQueryAdapter(ErgFile* ef = NULL) : ergFile(ef) {}

    const ErgFile* getErgFile() const     { return ergFile; }
    void     setErgFile(const ErgFile* p) { ergFile = p; }
    void     resetQueryState()            { qs.Reset(); }
    int      addNewLap(double loc) const;

private:
    const QList<ErgFilePoint>& Points() const { return ergFile->Points; }
    const QList<ErgFileLap>  & Laps()   const { return ergFile->Laps; }
    const QList<ErgFileText> & Texts()  const { return ergFile->Texts; }


    // Common helper to setup query state for query. Returns false if bracket cannot be established.
    bool   updateQueryStateFromDistance(double x, int& lapnum) const;

public:
    // Const getters
    bool   hasGradient() const { return ergFile && ergFile->hasGradient(); }
    bool   hasWatts()    const { return ergFile && ergFile->hasWatts();    }
    bool   hasGPS()      const { return ergFile && ergFile->hasGPS();      }

    double nextLap   (double x) const { return !ergFile ? -1 : ergFile->nextLap(x);    }
    double prevLap   (double x) const { return !ergFile ? -1 : ergFile->prevLap(x);    }
    double currentLap(double x) const { return !ergFile ? -1 : ergFile->currentLap(x); }

    bool   textsInRange(double searchStart, double searchRange, int& rangeStart, int& rangeEnd) const {
        return !ergFile ? false : ergFile->textsInRange(searchStart, searchRange, rangeStart, rangeEnd);
    }

    double currentTime() const { return !ergFile ? 0. : ergFile->Points.at(qs.rightPoint).x; }

    double Duration(void) const { return !ergFile ? 0. : ergFile->Duration; }

    // State queries (maintain mutable state.)
    double wattsAt(double msec, int& lapnum) const;
    double gradientAt(double meters, int& lapnum) const;
    double altitudeAt(double meters, int& lapnum) const;
    bool   locationAt(double meters, int& lapnum, geolocation& geoLoc, double& slope100) const;
};

#endif
