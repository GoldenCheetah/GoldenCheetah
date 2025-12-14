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

#include "RideFile.h"
#include "RideFileCommand.h"
#include "RideEditor.h"
#include <cmath>
#include <float.h>

// comparing doubles is nasty
static bool doubles_equal(double a, double b)
{
    double errorB = b * DBL_EPSILON;
    return (a >= b - errorB) && (a <= b + errorB);
}
//----------------------------------------------------------------------
// The public interface to the commands
//----------------------------------------------------------------------
RideFileCommand::RideFileCommand(RideFile *ride) : ride(ride), stackptr(0), inLUW(false), luw(NULL)
{
    connect(ride, SIGNAL(saved()), this, SLOT(clearHistory()));
    connect(ride, SIGNAL(reverted()), this, SLOT(clearHistory()));
}

RideFileCommand::~RideFileCommand()
{
    clearHistory();
}

void
RideFileCommand::setPointValue(int index, RideFile::SeriesType series, double value)
{
    SetPointValueCommand *cmd = new SetPointValueCommand(ride, index, series,
                                    ride->getPointValue(index, series), value);
    doCommand(cmd);
}

void
RideFileCommand::deletePoint(int index)
{
    RideFilePoint current = *ride->dataPoints()[index];
    DeletePointCommand *cmd = new DeletePointCommand(ride, index, current);
    doCommand(cmd);
}

void
RideFileCommand::deletePoints(int index, int count)
{
    QVector<RideFilePoint> current;
    for(int i=0; i< count; i++) {
        RideFilePoint point = *ride->dataPoints()[index+i];
        current.append(point);
    }
    DeletePointsCommand *cmd = new DeletePointsCommand(ride, index, count, current);
    doCommand(cmd);
}

void
RideFileCommand::deleteXDataPoints(QString xdata, int index, int count)
{
    QVector<XDataPoint*> current;
    for(int i=0; i< count; i++) {
        XDataPoint *point = ride->xdata(xdata)->datapoints[index+i];
        current.append(point);
    }
    DeleteXDataPointsCommand *cmd = new DeleteXDataPointsCommand(ride, xdata , index, count, current);
    doCommand(cmd);
}

void
RideFileCommand::insertPoint(int index, RideFilePoint *points)
{
    InsertPointCommand *cmd = new InsertPointCommand(ride, index, points);
    doCommand(cmd);
}

void
RideFileCommand::insertXDataPoint(QString xdata, int index, XDataPoint *points)
{
    InsertXDataPointCommand *cmd = new InsertXDataPointCommand(ride, xdata, index, points);
    doCommand(cmd);
}

void
RideFileCommand::appendPoints(QVector <RideFilePoint> newRows)
{
    AppendPointsCommand *cmd = new AppendPointsCommand(ride, ride->dataPoints().count(), newRows);
    doCommand(cmd);
}

void
RideFileCommand::setDataPresent(RideFile::SeriesType type, bool newvalue)
{
    bool oldvalue = ride->isDataPresent(type);
    SetDataPresentCommand *cmd = new SetDataPresentCommand(ride, type, newvalue, oldvalue);
    doCommand(cmd);
}

QString
RideFileCommand::changeLog()
{
    QString log;
    for (int i=0; i<stackptr; i++) {
        if (stack[i]->type != RideCommand::NoOp)
            log += stack[i]->description + '\n';
    }
    return log;
}

void
RideFileCommand::removeXData(QString name)
{
    RemoveXDataCommand *cmd = new RemoveXDataCommand(ride, name);
    doCommand(cmd);
}

void
RideFileCommand::addXData(XDataSeries *series)
{
    AddXDataCommand *cmd = new AddXDataCommand(ride, series);
    doCommand(cmd);
}

void
RideFileCommand::addXDataSeries(QString xdata, QString name, QString unit)
{
    AddXDataSeriesCommand *cmd = new AddXDataSeriesCommand(ride, xdata, name, unit);
    doCommand(cmd);
}

void
RideFileCommand::removeXDataSeries(QString xdata, QString name)
{
    RemoveXDataSeriesCommand *cmd = new RemoveXDataSeriesCommand(ride, xdata, name);
    doCommand(cmd);
}

void
RideFileCommand::setXDataPointValue(QString xdata, int row, int column, double value)
{
    // get current value
    XDataSeries *series = ride->xdata(xdata);
    if (!series) return;

    double ovalue = 0;
    switch(column) {
        case 0: ovalue = series->datapoints[row]->secs; break;
        case 1: ovalue = series->datapoints[row]->km; break;
        default: ovalue = series->datapoints[row]->number[column-2]; break;
    }

    SetXDataPointValueCommand *cmd = new  SetXDataPointValueCommand(ride, xdata, row, column, ovalue, value);
    doCommand(cmd);
}

void
RideFileCommand::appendXDataPoints(QString _xdata, QVector <XDataPoint *> newRows)
{
    XDataSeries *series = ride->xdata(_xdata);
    if (!series) return;

    AppendXDataPointsCommand *cmd = new AppendXDataPointsCommand(ride, _xdata, series->datapoints.count(), newRows);
    doCommand(cmd);
}

//----------------------------------------------------------------------
// Manage the Command Stack
//----------------------------------------------------------------------
void
RideFileCommand::clearHistory()
{
    foreach (RideCommand *cmd, stack) delete cmd;
    stack.clear();
    stackptr = 0;
}

void
RideFileCommand::startLUW(QString name)
{
    luw = new LUWCommand(this, name, ride);
    inLUW = true;
    beginCommand(false, luw);
}

void
RideFileCommand::endLUW()
{
    if (inLUW == false) return; // huh?
    inLUW = false;

    // add to the stack if it isn't empty
    if (luw->worklist.count()) doCommand(luw, true);
}

void
RideFileCommand::doCommand(RideCommand *cmd, bool noexec)
{

    // we must add to the LUW, but also must
    // execute immediately since state data
    // is collected by each command as it is
    // created.
    if (inLUW) {
        luw->addCommand(cmd);
        beginCommand(false, cmd);
        cmd->doCommand(); // luw must be executed as added!!!
        cmd->docount++;
        endCommand(false, cmd);
        return;
    }

    // place onto stack
    if (stack.count()) {
        // wipe away commands we can no longer redo
        for (int i=stackptr; i<stack.count(); i++) delete stack.at(i);
        stack.remove(stackptr, stack.count() - stackptr);
    }
    stack.append(cmd);
    stackptr++;

    if (noexec == false) {
        beginCommand(false, cmd); // signal
        cmd->doCommand(); // execute
    }
    cmd->docount++;
    endCommand(false, cmd); // signal - even if LUW

    // we changed it!
    ride->notifyModified();
}

void
RideFileCommand::redoCommand()
{
    if (stackptr < stack.count()) {
        beginCommand(false, stack[stackptr]); // signal
        stack[stackptr]->doCommand();
        stack[stackptr]->docount++;
        stackptr++; // increment before end to keep in sync in case
                    // it is queried 'after' the command is executed
                    // i.e. within a slot connected to this signal
        endCommand(false, stack[stackptr-1]); // signal
    }
}

void
RideFileCommand::undoCommand()
{
    if (stackptr > 0) {
        stackptr--;

        beginCommand(true, stack[stackptr]); // signal
        stack[stackptr]->undoCommand();
        endCommand(true, stack[stackptr]); // signal
    }
}

int
RideFileCommand::undoCount()
{
    return stackptr;
}

int
RideFileCommand::redoCount()
{
    return stack.count() - stackptr;
}

void
RideFileCommand::emitBeginCommand(bool undo, RideCommand *cmd)
{
    emit beginCommand(undo, cmd);
}

void
RideFileCommand::emitEndCommand(bool undo, RideCommand *cmd)
{
    emit endCommand(undo, cmd);
}

//----------------------------------------------------------------------
// Commands...
//----------------------------------------------------------------------

// Logical Unit of work
LUWCommand::LUWCommand(RideFileCommand *commander, QString name, RideFile *ride) :
        RideCommand(ride), commander(commander)
{
    type = RideCommand::LUW;
    description = name;
}

LUWCommand::~LUWCommand()
{
    foreach(RideCommand *cmd, worklist) delete cmd;
}

bool
LUWCommand::doCommand()
{
    foreach(RideCommand *cmd, worklist) {
        commander->emitBeginCommand(false, cmd);
        cmd->doCommand();
        commander->emitEndCommand(false, cmd);
    }
    return true;
}

bool
LUWCommand::undoCommand()
{
    for (int i=worklist.count(); i > 0; i--) {
        RideCommand *cmd = worklist[i-1];
        commander->emitBeginCommand(true, cmd);
        cmd->undoCommand();
        commander->emitEndCommand(true, cmd);
    }
    return true;
}

// Set point value
SetPointValueCommand::SetPointValueCommand(RideFile *ride, int row,
            RideFile::SeriesType series, double oldvalue, double newvalue) :
            RideCommand(ride), // base class looks after these
            row(row), series(series), oldvalue(oldvalue), newvalue(newvalue)
{
    type = RideCommand::SetPointValue;
    description = tr("Set Value");
}

bool
SetPointValueCommand::doCommand()
{
    // check it has changed first!
    if (!doubles_equal(oldvalue, newvalue)) {
        ride->setPointValue(row,series,newvalue);
    }
    return true;
}

bool
SetPointValueCommand::undoCommand()
{
    if (!doubles_equal(oldvalue, newvalue)) {
        ride->setPointValue(row,series,oldvalue);
    }
    return true;
}

// Remove a point
DeletePointCommand::DeletePointCommand(RideFile *ride, int row, RideFilePoint point) :
        RideCommand(ride), // base class looks after these
        row(row), point(point)
{
    type = RideCommand::DeletePoint;
    description = tr("Remove Point");
}

bool
DeletePointCommand::doCommand()
{
    ride->deletePoint(row);
    return true;
}

bool
DeletePointCommand::undoCommand()
{
    ride->insertPoint(row, new RideFilePoint(point));
    return true;
}

// Remove points
DeletePointsCommand::DeletePointsCommand(RideFile *ride, int row, int count,
                     QVector<RideFilePoint> current) :
        RideCommand(ride), // base class looks after these
        row(row), count(count), points(current)
{
    type = RideCommand::DeletePoints;
    description = tr("Remove Points");
}

bool
DeletePointsCommand::doCommand()
{
    ride->deletePoints(row, count);
    return true;
}

bool
DeletePointsCommand::undoCommand()
{
    for (int i=(count-1); i>=0; i--) ride->insertPoint(row, new RideFilePoint(points[i]));
    return true;
}

// Insert a point
InsertPointCommand::InsertPointCommand(RideFile *ride, int row, RideFilePoint *point) :
        RideCommand(ride), // base class looks after these
        row(row), point(*point)
{
    type = RideCommand::InsertPoint;
    description = tr("Insert Point");
}

bool
InsertPointCommand::doCommand()
{
    ride->insertPoint(row, new RideFilePoint(point));
    return true;
}

bool
InsertPointCommand::undoCommand()
{
    ride->deletePoint(row);
    return true;
}

// Append points
AppendPointsCommand::AppendPointsCommand(RideFile *ride, int row, QVector<RideFilePoint> points) :
        RideCommand(ride), // base class looks after these
        row(row), count (points.count()), points(points)
{
    type = RideCommand::AppendPoints;
    description = tr("Append Points");
}

bool
AppendPointsCommand::doCommand()
{
    QVector<RideFilePoint *> newPoints;
    foreach (RideFilePoint point, points) {
        RideFilePoint *p = new RideFilePoint(point);
        newPoints.append(p);
    }
    ride->appendPoints(newPoints);
    return true;
}

bool
AppendPointsCommand::undoCommand()
{
    for (int i=0; i<count; i++) ride->deletePoint(row);
    return true;
}

SetDataPresentCommand::SetDataPresentCommand(RideFile *ride,
                RideFile::SeriesType series, bool newvalue, bool oldvalue) :
        RideCommand(ride), // base class looks after these
        series(series), oldvalue(oldvalue), newvalue(newvalue)
{
    type = RideCommand::SetDataPresent;
    description = tr("Set Data Present");
}

bool
SetDataPresentCommand::doCommand()
{
    ride->setDataPresent(series, newvalue);
    return true;
}

bool
SetDataPresentCommand::undoCommand()
{
    ride->setDataPresent(series, oldvalue);
    return true;
}

RemoveXDataCommand::RemoveXDataCommand(RideFile *ride, QString name) : RideCommand(ride), name(name) {
    type = RideCommand::removeXData;
    description = tr("Remove XData");
}

bool
RemoveXDataCommand::doCommand()
{
    series = ride->xdata(name);
    ride->xdata().remove(name);
    return true;
}

bool
RemoveXDataCommand::undoCommand()
{
    ride->xdata().insert(name, series);
    return true;
}


AddXDataCommand::AddXDataCommand(RideFile *ride, XDataSeries *series) : RideCommand(ride), series(series) {
    type = RideCommand::addXData;
    description = tr("Add XData");
}

bool
AddXDataCommand::doCommand()
{
    ride->xdata().insert(series->name, series);
    return true;
}

bool
AddXDataCommand::undoCommand()
{
    ride->xdata().remove(series->name);
    return true;
}

RemoveXDataSeriesCommand::RemoveXDataSeriesCommand(RideFile *ride, QString xdata, QString name) : RideCommand(ride), xdata(xdata), name(name) {
    type = RideCommand::RemoveXDataSeries;
    description = tr("Remove XData Series");
}

bool
RemoveXDataSeriesCommand::doCommand()
{
    index = -1;

    // whats the index?
    XDataSeries *series = ride->xdata(xdata);
    if (series == NULL)  return false;

    index = series->valuename.indexOf(name);
    if (index == -1) return false;

    // snaffle away the data and clear
    values.resize(series->datapoints.count());
    for(int i=0; i<series->datapoints.count(); i++) {
        values[i] = series->datapoints[i]->number[index];
        series->datapoints[i]->number[index] = 0;

        // shift the values down
        for(int j=index+1; j<8; j++) {
            series->datapoints[i]->number[j-1] =
            series->datapoints[i]->number[j];
        }
    }

    // remove the name
    series->valuename.removeAt(index);
    return true;
}

bool
RemoveXDataSeriesCommand::undoCommand()
{

    if (index == -1 || name == "") return false;

    // add the series back
    XDataSeries *series = ride->xdata(xdata);
    if (series == NULL) return false;

    series->valuename.insert(index, name);

    // put data back
    for(int i=0; i<series->datapoints.count(); i++) {
        // shift the values right
        for(int j=index; j<7; j++) {
            series->datapoints[i]->number[j+1] =
            series->datapoints[i]->number[j];
        }
        series->datapoints[i]->number[index] = values[i];
    }
    return true;
}


AddXDataSeriesCommand::AddXDataSeriesCommand(RideFile *ride, QString xdata, QString name, QString unit) : RideCommand(ride), xdata(xdata), name(name), unit(unit) {
    type = RideCommand::AddXDataSeries;
    description = tr("Add XData Series");
}

bool
AddXDataSeriesCommand::doCommand()
{
    // whats the index?
    XDataSeries *series = ride->xdata(xdata);
    if (series == NULL)  return false;

    series->valuename.append(name);
    series->unitname.append(unit);

    int index = series->valuename.indexOf(name);
    if (index == -1) return false;

    // Clear the value
    for(int i=0; i<series->datapoints.count(); i++) {
        series->datapoints[i]->number[index] = 0;
    }

    return true;
}

bool
AddXDataSeriesCommand::undoCommand()
{
    // add the series back
    XDataSeries *series = ride->xdata(xdata);
    if (series == NULL) return false;

    int index = series->valuename.count()-1;
    if (index == -1) return false;
    series->valuename.removeAt(index);

    return true;
}

SetXDataPointValueCommand::SetXDataPointValueCommand(RideFile *ride, QString xdata, int row, int col, double oldvalue, double newvalue)
    : RideCommand(ride), row(row), col(col), xdata(xdata), oldvalue(oldvalue), newvalue(newvalue)
{
    type = RideCommand::SetXDataPointValue;
    description = tr("Set XData point value");
}

bool
SetXDataPointValueCommand::doCommand()
{
    XDataSeries *series = ride->xdata(xdata);
    if (series && !doubles_equal(oldvalue, newvalue)) {
        switch(col){
        case 0:
            series->datapoints[row]->secs = newvalue;
            break;
        case 1:
            series->datapoints[row]->km = newvalue;
            break;
        default:
            series->datapoints[row]->number[col-2] = newvalue;
        }
    }
    return true;
}
bool
SetXDataPointValueCommand::undoCommand()
{
    XDataSeries *series = ride->xdata(xdata);
    if (series && !doubles_equal(oldvalue, newvalue)) {
        switch(col){
        case 0:
            series->datapoints[row]->secs = oldvalue;
            break;
        case 1:
            series->datapoints[row]->km = oldvalue;
            break;
        default:
            series->datapoints[row]->number[col-2] = oldvalue;
        }
    }
    return true;
}

// Remove points
DeleteXDataPointsCommand::DeleteXDataPointsCommand(RideFile *ride, QString xdata, int row, int count,
                     QVector<XDataPoint *> current) :
        RideCommand(ride), // base class looks after these
        xdata(xdata), row(row), count(count), points(current)
{
    type = RideCommand::DeleteXDataPoints;
    description = tr("Remove XDATA Points");
}

bool
DeleteXDataPointsCommand::doCommand()
{
    ride->deleteXDataPoints(xdata, row, count);
    return true;
}

bool
DeleteXDataPointsCommand::undoCommand()
{
    for (int i=(count-1); i>=0; i--) ride->insertXDataPoint(xdata, row, points[i]);
    return true;
}

// Insert a point
InsertXDataPointCommand::InsertXDataPointCommand(RideFile *ride, QString xdata, int row, XDataPoint *point) :
        RideCommand(ride), // base class looks after these
        xdata(xdata), row(row), point(point)
{
    type = RideCommand::InsertXDataPoint;
    description = tr("Insert XData Point");
}

bool
InsertXDataPointCommand::doCommand()
{
    ride->insertXDataPoint(xdata, row, point);
    return true;
}

bool
InsertXDataPointCommand::undoCommand()
{
    ride->deleteXDataPoints(xdata, row, 1);
    return true;
}

// Append points
AppendXDataPointsCommand::AppendXDataPointsCommand(RideFile *ride, QString xdata, int row, QVector<XDataPoint*> points) :
        RideCommand(ride), // base class looks after these
        xdata(xdata), row(row), count(points.count()), points(points)
{
    type = RideCommand::AppendXDataPoints;
    description = tr("Append XData Points");
}

bool
AppendXDataPointsCommand::doCommand()
{
    ride->appendXDataPoints(xdata, points);
    return true;
}

bool
AppendXDataPointsCommand::undoCommand()
{
    ride->deleteXDataPoints(xdata, row, count);
    return true;
}
