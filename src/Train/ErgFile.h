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

// which section of the file are we in?
#define NOMANSLAND  0
#define SETTINGS 1
#define DATA     2
#define END      3

// is this in .erg or .mrc format?
#define ERG     1
#define MRC     2
#define CRS     3
#define CRS_LOC 4

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

class ErgFileText
{
    public:
        ErgFileText() : x(0), pos(0), text("") {}
        ErgFileText(double x, int pos, QString text) : x(x), pos(pos), text(text) {}

        double x;
        int pos;
        QString text;
};

class ErgFileLap
{
    public:
        long x;     // when does this LAP marker occur? (time in msecs or distance in meters
        int LapNum;     // from 1 - n
        bool selected; // used by the editor
        QString name;
};

class ErgFile
{
    public:
        ErgFile(QString, int, Context *context);       // constructor uses filename
        ErgFile(Context *context); // no filename, going to use a string

        ~ErgFile();             // delete the contents

        void setFrom(ErgFile *f); // clone an existing workout
        bool save(QStringList &errors); // save back, with changes

        static ErgFile *fromContent(QString, Context *); // read from memory
        static bool isWorkout(QString); // is this a supported workout?

        void reload();          // reload after messed about

        void parseComputrainer(QString p = ""); // its an erg,crs or mrc file
        void parseTacx();       // its a pgmf file
        void parseZwift();      // its a zwo file (zwift xml)
        void parseGpx();        // its a gpx...

        bool isValid();         // is the file valid or not?
        double Cp;
        int format;             // ERG, CRS, MRC and CRS_LOC currently supported
        int wattsAt(long, int&);      // return the watts value for the passed msec
        double gradientAt(long, int&);      // return the gradient value for the passed meter
        bool locationAt(long x, int& lapnum, geolocation &geoLoc); // location at meter

        int nextLap(long);      // return the start value (erg - time(ms) or slope - distance(m)) for the next lap
        int currentLap(long);   // return the start value (erg - time(ms) or slope - distance(m)) for the current lap

        // turn the ergfile into a series of sections rather
        // than a list of points
        QList<ErgFileSection> Sections();

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
        bool valid;             // did it parse ok?
        int mode;

        int leftPoint, rightPoint;     // current points we are between
        int interpolatorReadIndex;     // next point to be fed to interpolator

        QList<ErgFilePoint> Points;    // points in workout
        QList<ErgFileLap>   Laps;      // interval markers in the file
        QList<ErgFileText>  Texts;     // texts to display

        GeoPointInterpolator gpi; // Location interpolator

        void calculateMetrics(); // calculate IsoPower value for ErgFile

        // Metrics for this workout
        double maxY;                // maximum Y value
        double CP;
        double AP, IsoPower, IF, BikeStress, VI; // Coggan for erg / mrc
        double XP, RI, BS, SVI; // Skiba for erg / mrc
        double ELE, ELEDIST, GRADE;    // crs

        Context *context;

};

#endif
