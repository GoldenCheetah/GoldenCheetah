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
RideFileCommand::insertPoint(int index, RideFilePoint *points)
{
    InsertPointCommand *cmd = new InsertPointCommand(ride, index, points);
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
    ride->emitModified();
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
