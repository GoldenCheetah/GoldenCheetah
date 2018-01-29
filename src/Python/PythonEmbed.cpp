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
#include <Python.h>

// global instance of embedded python
PythonEmbed *python;
PyThreadState *mainThreadState;

// SIP module with GoldenCheetah Bindings
extern "C" {
extern PyObject *PyInit_goldencheetah(void);
};

PythonEmbed::~PythonEmbed()
{
}

PythonEmbed::PythonEmbed(const bool verbose, const bool interactive) : verbose(verbose), interactive(interactive)
{
    loaded = false;
    threadid=-1;
    name = QString("GoldenCheetah");

    // tell python our program name
    Py_SetProgramName((wchar_t*) name.toStdString().c_str());

    // our own module
    PyImport_AppendInittab("goldencheetah", PyInit_goldencheetah);

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

    // our base code - traps stdout and loads goldencheetan module
    // mapping all the bindings to a GC object.
    std::string stdOutErr = ("import sys\n"
 #ifdef Q_OS_LINUX
                             "import os\n"
                             "sys.setdlopenflags(os.RTLD_NOW | os.RTLD_DEEPBIND)\n"
 #endif
                             "class CatchOutErr:\n"
                             "    def __init__(self):\n"
                             "        self.value = ''\n"
                             "    def write(self, txt):\n"
                             "        self.value += txt\n"
                             "catchOutErr = CatchOutErr()\n"
                             "sys.stdout = catchOutErr\n"
                             "sys.stderr = catchOutErr\n"
                             "import goldencheetah\n"
                             "GC=goldencheetah.Bindings()\n");

    PyRun_SimpleString(stdOutErr.c_str()); //invoke code to redirect

    // now load the library
    QFile lib(":python/library.py");
    if (lib.open(QFile::ReadOnly)) {
        QString libstring=lib.readAll();
        lib.close();
        PyRun_SimpleString(libstring.toLatin1().constData());
    }


    // setup trapping of output
    PyObject *pModule = PyImport_AddModule("__main__"); //create main module
    catcher = static_cast<void*>(PyObject_GetAttrString(pModule,"catchOutErr"));
    clear = static_cast<void*>(PyObject_GetAttrString(static_cast<PyObject*>(catcher), "__init__"));
    PyErr_Print(); //make python print any errors
    PyErr_Clear(); //and clear them !

    // prepare for threaded processing
    PyEval_InitThreads();
    mainThreadState = PyEval_SaveThread();
    loaded = true;
}

// run on called thread
void PythonEmbed::runline(Context *context, QString line)
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    // Get current thread ID via Python thread functions
    PyObject* thread = PyImport_ImportModule("_thread");
    PyObject* get_ident = PyObject_GetAttrString(thread, "get_ident");
    PyObject* ident = PyObject_CallObject(get_ident, 0);
    Py_DECREF(get_ident);
    threadid = PyLong_AsLong(ident);
    Py_DECREF(ident);

    // add to the thread/context map
    contexts.insert(threadid, context);

    // run and generate errors etc
    messages.clear();
    PyRun_SimpleString(line.toStdString().c_str());
    PyErr_Print();
    PyErr_Clear(); //and clear them !

    // capture results
    PyObject *output = PyObject_GetAttrString(static_cast<PyObject*>(catcher),"value"); //get the stdout and stderr from our catchOutErr object
    if (output) {
        // allocated as unicodeA
        Py_ssize_t size;
        wchar_t *string = PyUnicode_AsWideCharString(output, &size);
        if (string) {
            if (size) messages = QString::fromWCharArray(string).split("\n");
            PyMem_Free(string);
            if (messages.count()) messages << "\n"; // always add a newline after anything
        }

        // clear results
        PyObject_CallFunction(static_cast<PyObject*>(clear), NULL);
    }

    PyGILState_Release(gstate);
    threadid=-1;
}

void
PythonEmbed::cancel()
{
    if (chart!=NULL && threadid != -1) {
        PyGILState_STATE gstate;
        gstate = PyGILState_Ensure();

        // raise an exception to cancel the execution
        PyThreadState_SetAsyncExc(threadid, PyExc_KeyboardInterrupt);

        PyGILState_Release(gstate);
    }
}
