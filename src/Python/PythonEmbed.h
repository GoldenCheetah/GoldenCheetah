/*
 * Copyright (c) 2017 Mark Liversedge (liversedge@gmail.com)
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

#ifndef GC_PYTHONEMBED_H
#define GC_PYTHONEMBED_H

#include <QWidget>
#include <QString>
#include <QMap>
#include <QStringList>

class Context;
class PythonChart;

class PythonEmbed;
extern PythonEmbed *python;

// a plain C++ class, no QObject stuff
class PythonEmbed {

    public:
    
    PythonEmbed(const bool verbose=false, const bool interactive=false);
    ~PythonEmbed();

    // scripts can set a result value
    double result;

    // catch and clear output - we use void* because we cannot
    // include the python headers here as they redefine the slots
    // mechanism that QT needs in header files. As a result they
    // are cast to PyObject* the CPP source code (which are in turn
    // typedefs so we can't even declare the class type here).
    void *catcher;
    void *clear;

    // run a single line from console
    void runline(Context *, QString);

    // stop current execution
    void cancel();

    // context for caller - can be called in a thread
    QMap<long,Context *> contexts;
    PythonChart *chart;
    QWidget *canvas;

    // the program being constructed/parsed
    QStringList program;
    QStringList messages;

    QString name;
    QString version;

    bool verbose;
    bool interactive;
    bool cancelled;

    bool loaded;
    long threadid;
};

#endif
