/*
 * Copyright (c) 2017 Mark Liversedge (liversedge@gmail.com)
 *
 * Additionally, for the original source used as a basis for this (RInside.cpp)
 * Released under the same GNU public license.
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

#include "PythonEmbed.h"
#include "Settings.h"
#include <stdexcept>

#include <QMessageBox>

// Python header - using a C preprocessor hack
// GC_PYTHON_INCDIR is from gcconfig.pri, typically python3.6
#ifdef slots // clashes with python headers
#undef slots
#endif
#include GC_PYTHONHEADER

// global instance of embedded python
PythonEmbed *python;

PythonEmbed::~PythonEmbed()
{
}

PythonEmbed::PythonEmbed(const bool verbose, const bool interactive) : verbose(verbose), interactive(interactive)
{
    loaded = false;
    name = QString("GoldenCheetah");

    // tell python our program name
    Py_SetProgramName((wchar_t*) name.toStdString().c_str());

    // need to load the interpreter etc
    Py_InitializeEx(0);

    // set the module path in the same way the interpreter would
    PyObject *sys = PyImport_ImportModule("sys");
    PyObject *path = PyObject_GetAttrString(sys, "path");
    PyList_Append(path, PyUnicode_FromString("."));

    // get version
    version = QString(Py_GetVersion());
    version.replace("\n", " ");

    fprintf(stderr, "Python loaded [%s]\n", version.toStdString().c_str());

    loaded = true;
}

void
PythonEmbed::cancel()
{
}
