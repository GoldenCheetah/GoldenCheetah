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

#include "RideItem.h"
#include "Specification.h"

class Context;
class PythonChart;

class PythonEmbed;
extern PythonEmbed *python;

// Context for Python Scripts
class ScriptContext {
    public:
        // read-only ctor
        ScriptContext(Context *context, RideItem *item=NULL, const QHash<QString,RideMetric*> *metrics=NULL,
                      Specification spec=Specification(), bool interactiveShell=false)
            : context(context), item(item), rideFile(NULL), metrics(metrics), spec(spec),
              interactiveShell(interactiveShell), readOnly(true), editedRideFiles(NULL) {}

        // read/write ctor
        ScriptContext(Context *context, RideFile *rideFile, bool interactiveShell,
                      bool readOnly, QList<RideFile *> *editedRideFiles)
            : context(context), item(NULL), rideFile(rideFile), metrics(NULL), spec(),
              interactiveShell(interactiveShell), readOnly(readOnly), editedRideFiles(editedRideFiles) {}

        // default ctor
        ScriptContext() : context(NULL), item(NULL), rideFile(NULL), metrics(NULL), spec(),
            interactiveShell(false), readOnly(true), editedRideFiles(NULL) {}

        Context *context;
        RideItem *item;
        RideFile *rideFile;
        const QHash<QString,RideMetric*> *metrics;
        Specification spec;
        bool interactiveShell;

        bool readOnly;
        QList<RideFile *> *editedRideFiles;
};

// a plain C++ class, no QObject stuff
class PythonEmbed {

    public:
    
    PythonEmbed(const bool verbose=false, const bool interactive=false);
    ~PythonEmbed();

    static QString buildVersion(); // Python version used at build time
    // find installed binary and check version and module path
    static bool pythonInstalled(QString &pybin, QString &pypath, QString PYTHONHOME=QString(""));
    QString pybin, pypath;

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
    void runline(ScriptContext, QString);

    // stop current execution
    void cancel();

    // context for caller - can be called in a thread
    QMap<long, ScriptContext> contexts;
    Perspective *perspective;
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

// embed debugging via 'printd' and enable via PYTHON_DEBUG
#ifndef PYTHON_DEBUG
#define PYTHON_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (PYTHON_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (PYTHON_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif
#endif
