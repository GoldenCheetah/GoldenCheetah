/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _RideFileCommand_h
#define _RideFileCommand_h
#include "GoldenCheetah.h"

#include <QObject>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QList>
#include <QMap>
#include <QVector>
#include <QApplication>

#include "RideFile.h"

// for modifying ride data - implements the command pattern
//                           for undo/redo functionality
class RideCommand;
class LUWCommand;

class RideFileCommand : public QObject
{
    Q_OBJECT
    G_OBJECT


    friend class LUWCommand;

    public:
        RideFileCommand(RideFile *ride);
        virtual ~RideFileCommand();

        void setPointValue(int index, RideFile::SeriesType series, double value);
        void deletePoint(int index);
        void deletePoints(int index, int count);
        void insertPoint(int index, RideFilePoint *point);
        void appendPoints(QVector <struct RideFilePoint> newRows);
        void setDataPresent(RideFile::SeriesType, bool);

        // working with xdata
        void removeXData(QString name);
        void addXData(XDataSeries *series);
        void removeXDataSeries(QString xdata, QString name);
        void addXDataSeries(QString xdata, QString name, QString unit);
        void setXDataPointValue(QString xdata, int row, int column, double value);
        void deleteXDataPoints(QString xdata, int index, int count);
        void insertXDataPoint(QString xdata, int index, XDataPoint *point);
        void appendXDataPoints(QString xdata, QVector<XDataPoint*> rows);

        // execute atomic actions
        void doCommand(RideCommand*, bool noexec=false);
        void undoCommand();
        void redoCommand();

        // stack status
        void startLUW(QString name);
        void endLUW();

        // change status
        QString changeLog();
        int undoCount();
        int redoCount();

    public Q_SLOTS:
        void clearHistory();

    Q_SIGNALS:
        void beginCommand(bool undo, RideCommand *cmd);
        void endCommand(bool undo, RideCommand *cmd);

    protected:
        void emitBeginCommand(bool, RideCommand *cmd);
        void emitEndCommand(bool, RideCommand *cmd);

    private:
        RideFile *ride;
        QVector<RideCommand *> stack;
        int stackptr;
        bool inLUW;
        LUWCommand *luw;
};

// The Command itself, as a base class with
// subclasses for each type
class RideCommand
{
    public:
        // supported command types
        enum commandtype { NoOp, LUW, SetPointValue, DeletePoint, DeletePoints, InsertPoint, AppendPoints, SetDataPresent,
                           removeXData, addXData, RemoveXDataSeries, AddXDataSeries,
                           SetXDataPointValue, DeleteXDataPoints, InsertXDataPoint, AppendXDataPoints };
        typedef enum commandtype CommandType;


        RideCommand(RideFile *ride) : type(NoOp), ride(ride), docount(0) {}
        virtual ~RideCommand() {}
        virtual bool doCommand() { return true; }
        virtual bool undoCommand() { return true; }

        // state of selection model -- if passed at all
        CommandType type;
        QString description;
        RideFile *ride;
        int docount; // how many times has this been executed?
};

//----------------------------------------------------------------------
// The commands
//----------------------------------------------------------------------
class LUWCommand : public RideCommand
{
    public:
        LUWCommand(RideFileCommand *command, QString name, RideFile *ride);
        ~LUWCommand(); // needs to clear worklist entries

        void addCommand(RideCommand *cmd) { worklist.append(cmd); }
        bool doCommand();
        bool undoCommand();

        QVector<RideCommand*> worklist;
        RideFileCommand *commander;
};

class RemoveXDataCommand : public RideCommand
{
    Q_DECLARE_TR_FUNCTIONS(RemoveXDataCommand)

    public:
        RemoveXDataCommand(RideFile *ride, QString name);
        bool doCommand();
        bool undoCommand();

        // state
        QString name;
        XDataSeries *series;
};

class AddXDataCommand : public RideCommand
{
    Q_DECLARE_TR_FUNCTIONS(addXDataCommand)

    public:
        AddXDataCommand(RideFile *ride, XDataSeries *series);
        bool doCommand();
        bool undoCommand();

        // state
        XDataSeries *series;
};

class RemoveXDataSeriesCommand : public RideCommand
{
    Q_DECLARE_TR_FUNCTIONS(RemoveXDataSeriesCommand)

    public:
        RemoveXDataSeriesCommand(RideFile *ride, QString xdata, QString name);
        bool doCommand();
        bool undoCommand();

        // state
        QString xdata, name;
        int index;
        QVector<double> values;
};

class AddXDataSeriesCommand : public RideCommand
{
    Q_DECLARE_TR_FUNCTIONS(AddXDataSeriesCommand)

    public:
        AddXDataSeriesCommand(RideFile *ride, QString xdata, QString name, QString unit);
        bool doCommand();
        bool undoCommand();

        // state
        QString xdata, name, unit;
};

class SetPointValueCommand : public RideCommand
{
    Q_DECLARE_TR_FUNCTIONS(SetPointValueCommand)

    public:
        SetPointValueCommand(RideFile *ride, int row, RideFile::SeriesType series, double oldvalue, double newvalue);
        bool doCommand();
        bool undoCommand();

        // state
        int row;
        RideFile::SeriesType series;
        double oldvalue, newvalue;
};

class SetXDataPointValueCommand : public RideCommand
{
    Q_DECLARE_TR_FUNCTIONS(SetXDataPointValueCommand)

    public:
        SetXDataPointValueCommand(RideFile *ride, QString xdata, int row, int col, double oldvalue, double newvalue);
        bool doCommand();
        bool undoCommand();

        // state
        int row, col;
        QString xdata;
        double oldvalue, newvalue;
};

class DeleteXDataPointsCommand : public RideCommand
{
    Q_DECLARE_TR_FUNCTIONS(DeletePointsCommand)

    public:
        DeleteXDataPointsCommand(RideFile *ride, QString xdata, int row, int count, QVector<XDataPoint*> current);
        bool doCommand();
        bool undoCommand();

        // state
        QString xdata;
        int row;
        int count;
        QVector<XDataPoint*> points;
};
class DeletePointCommand : public RideCommand
{
    Q_DECLARE_TR_FUNCTIONS(DeletePointCommand)

    public:
        DeletePointCommand(RideFile *ride, int row, RideFilePoint point);
        bool doCommand();
        bool undoCommand();

        // state
        int row;
        RideFilePoint point;
};

class DeletePointsCommand : public RideCommand
{
    Q_DECLARE_TR_FUNCTIONS(DeletePointsCommand)

    public:
        DeletePointsCommand(RideFile *ride, int row, int count,
            QVector<RideFilePoint> current);
        bool doCommand();
        bool undoCommand();

        // state
        int row;
        int count;
        QVector<RideFilePoint> points;
};

class InsertPointCommand : public RideCommand
{
    Q_DECLARE_TR_FUNCTIONS(InsertPointCommand)

    public:
        InsertPointCommand(RideFile *ride, int row, RideFilePoint *point);
        bool doCommand();
        bool undoCommand();

        // state
        int row;
        RideFilePoint point;
};
class InsertXDataPointCommand : public RideCommand
{
    Q_DECLARE_TR_FUNCTIONS(InsertXDataPointCommand)

    public:
        InsertXDataPointCommand(RideFile *ride, QString xdata, int row, XDataPoint *point);
        bool doCommand();
        bool undoCommand();

        // state
        QString xdata;
        int row;
        XDataPoint *point;
};
class AppendPointsCommand : public RideCommand
{
    Q_DECLARE_TR_FUNCTIONS(AppendPointsCommand)

    public:
        AppendPointsCommand(RideFile *ride, int row, QVector<RideFilePoint> points);
        bool doCommand();
        bool undoCommand();

        int row, count;
        QVector<RideFilePoint> points;
};
class AppendXDataPointsCommand : public RideCommand
{
    Q_DECLARE_TR_FUNCTIONS(AppendXDataPointsCommand)

    public:
        AppendXDataPointsCommand(RideFile *ride, QString xdata, int row, QVector<XDataPoint*> points);
        bool doCommand();
        bool undoCommand();

        QString xdata;
        int row, count;
        QVector<XDataPoint *> points;
};
class SetDataPresentCommand : public RideCommand
{
    Q_DECLARE_TR_FUNCTIONS(SetDataPresentCommand)

    public:
        SetDataPresentCommand(RideFile *ride, RideFile::SeriesType series,
                              bool newvalue, bool oldvalue);
        bool doCommand();
        bool undoCommand();
        RideFile::SeriesType series;
        bool oldvalue, newvalue;
};
#endif // _RideFileCommand_h
