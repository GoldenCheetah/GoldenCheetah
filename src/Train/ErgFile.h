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
#include "ErgFileEnums.h"

class ErgFilePoint
{
    friend class ErgFile;
    friend class ErgFileData;
    friend class ErgFilePlot;
    friend class TrainSidebar;
    friend class WorkoutPlotWindow;
    friend class WorkoutWidget;

public:

        ErgFilePoint() : x(0), y(0), val(0) {}
        ErgFilePoint(double x, double y, double val) : x(x), y(y), val(val) {}

    protected:
        double x;     // x axis - time in msecs or distance in meters
        double y;     // y axis - load in watts or altitude
        double val;   // the value to send to the device (watts/gradient)
};

class ErgFileSection
{
    friend class ErgFile;
    friend class ErgFileData;
    friend class ErgFilePlot;
    public:
        ErgFileSection() : duration(0), start(0), end(0) {}
        ErgFileSection(int duration, int start, int end) : duration(duration), start(start), end(end) {}

    protected:
        int duration;
        double start, end;
};

class ErgFileText
{
    friend class ErgFile;
    friend class ErgFileData;
    friend class ErgFilePlot;
    public:
        ErgFileText() : x(0), pos(0), text("") {}
        ErgFileText(double x, int pos, QString text) : x(x), pos(pos), text(text) {}

    protected:
        double x;
        int pos;
        QString text;
};

class ErgFileLap
{
    friend class ErgFile;
    friend class ErgFileData;
    friend class ErgFilePlot;
    friend class WorkoutWidget;
    friend class WWLap;

    protected:
        long x;     // when does this LAP marker occur? (time in msecs or distance in meters
        int LapNum;     // from 1 - n
        bool selected; // used by the editor
        QString name;
};

class ErgFile
{

    /** these are offenders to not using public accessors and should change */
    friend class ErgFilePlot;
    friend class ErgFileData;
    friend class NowData;
    friend class TrainDB;
    friend class TrainSidebar;
    friend class WorkoutPlotWindow;
    friend class WorkoutWidget;
    friend class WorkoutWindow;
    friend class WorkoutWidgetItems;

    public:
    
        ErgFile(QString filenameOnDisk, ErgFileMode mode, Context *context); // constructor uses filename
        ErgFile(Context *context); // no filename, going to use a string

        virtual ~ErgFile();             // delete the contents

        void setFrom(ErgFile *f); // clone an existing workout
        bool save(QStringList &errors); // save back, with changes

        static ErgFile *fromContent(QString, Context *); // read from memory
        static bool isWorkout(QString); // is this a supported workout?

        void reload();          // reload after messed about

        void parseComputrainer(QString p = ""); // its an erg,crs or mrc file
        void parseTacx();         // its a pgmf file
        void parseZwift();         // its a zwo file (zwift xml)

        bool isValid();         // is the file valid or not?
        int wattsAt(long, int&);      // return the watts value for the passed msec
        double gradientAt(long, int&);      // return the gradient value for the passed meter
        int nextLap(long);      // return the msecs value for the next Lap marker

        // turn the ergfile into a series of sections rather
        // than a list of points
        QList<ErgFileSection> Sections();

        void calculateMetrics(); // calculate NP value for ErgFile
        long GetDuration() const;
        ErgFileFormat GetFormat();
        Context* GetContext();
        
    private:
        double Cp;
        ErgFileFormat format;             // ERG, CRS or MRC currently supported
        ErgFileMode mode;
        long    Duration;       // Duration of this workout in msecs
        int     Ftp;            // FTP this file was targetted at
        int     MaxWatts;       // maxWatts in this ergfile (scaling)
        bool valid;             // did it parse ok?
        
        
        int leftPoint, rightPoint;            // current points we are between
        
        QList<ErgFilePoint> Points;    // points in workout
        QList<ErgFileLap>   Laps;      // interval markers in the file
        QList<ErgFileText>  Texts;     // texts to display
        QString Version,        // version number / identifer
            Units,          // units used
            filenameInsideFile,       // filename from inside file
            filenameOnDisk,       // filename on disk
            Name,           // description in file
            Description,    // long narrative for workout
            ErgDBId,        // if downloaded from ergdb
            Source;         // where did this come from
        QStringList Tags;       // tagged strings

        // Metrics for this workout
        double maxY;                // maximum Y value
        double CP;
        double AP, NP, IF, TSS, VI; // Coggan for erg / mrc
        double XP, RI, BS, SVI; // Skiba for erg / mrc
        double ELE, ELEDIST, GRADE;    // crs
    
    protected:
        Context *context;
};

#endif
