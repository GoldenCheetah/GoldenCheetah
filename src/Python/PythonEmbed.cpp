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
#include "Utils.h"
#include "Settings.h"
#include <stdexcept>

#include <QtGlobal>
#include <QMessageBox>
#include <QProcess>

#ifdef slots // clashes with python headers
#undef slots
#endif
#include <Python.h>

// we only really support Python 3, so lets only work on that basis
#if PY_MAJOR_VERSION >= 3
#define PYTHON3_VERSION PY_MINOR_VERSION
#endif

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

bool PythonEmbed::pythonInstalled(QString &pybin, QString &pypath, QString PYTHONHOME)
{
    QStringList names; names << QString("python3.%1").arg(PYTHON3_VERSION) << QString("bin/python3.%1").arg(PYTHON3_VERSION) << "python3" << "bin/python3" << "python" << "bin/python";
    QString pythonbinary;

    if (PYTHONHOME=="") {

        // where to check
        QString path = QProcessEnvironment::systemEnvironment().value("PATH", "");
        printd("PATH=%s\n", path.toStdString().c_str());

        // what we found
        QStringList installnames;

        // lets search
        foreach(QString name, names) {
            installnames = Utils::searchPath(path, name, true);
            if (installnames.count() >0) break;
        }

        printd("Binary found:%d\n", installnames.count());
        // if we failed, its not installed
        if (installnames.count()==0) return false;

        // lets just use the first one we found
        pythonbinary = installnames[0];
        pybin=pythonbinary;

    } else {

        // look for python3 or python in PYTHONHOME
#ifdef WIN32
        QString ext= QString(".exe");
#else
        QString ext= QString("");
#endif
        foreach(QString name, names) {
            QString filename = PYTHONHOME + QDir::separator() + name + ext;
            if (QFileInfo(filename).exists() && QFileInfo(filename).isExecutable()) {
                pythonbinary=filename;
                break;
            }
        }
        // not found give up straight away
        if (pythonbinary == "") return false;
    }

#ifdef WIN32
        // ugh. QProcess doesn't like spaces or backslashes. POC.
        pythonbinary=pythonbinary.replace("\\", "/");
        pythonbinary="\"" + pythonbinary + "\"";
#endif

    // get the version and path via an interaction
    printd("Running: %s\n", pythonbinary.toStdString().c_str());
    QProcess py;
    py.setProgram(pythonbinary);

    // set the arguments
    QStringList args;
    args << "-c";
    args << QString("import sys\n"
                    "print('ZZ',sys.version_info.major,'ZZ')\n"
                    "print('ZZ',sys.version_info.minor,'ZZ')\n"
                    "print('ZZ', '%1'.join(sys.path), 'ZZ')\n"
                    "quit()\n").arg(PATHSEP);
    py.setArguments(args);
    py.start();

    // failed to start python
    if (py.waitForStarted(500) == false) {
        printd("Failed to start: %s\n", pythonbinary.toStdString().c_str());
        py.terminate();
        return false;
    }

    // wait for output, should be rapid
    if (py.waitForReadyRead(2000)==false) {
        printd("Didn't get output: %s\n", pythonbinary.toStdString().c_str());
        py.terminate();
        return false;
    }

    // get output
    QString output = py.readAll();

    // close if it didn't already
    if (py.waitForFinished(500)==false) {
        printd("forced terminate of %s\n", pythonbinary.toStdString().c_str());
        py.terminate();
    }

    // scan output
    QRegExp contents("^ZZ(.*)ZZ.*ZZ(.*)ZZ.*ZZ(.*)ZZ.*$");
    if (contents.exactMatch(output)) {
        QString vmajor=contents.cap(1);
        QString vminor=contents.cap(2);
        QString path=contents.cap(3);

        // check its Python 3 matching the version used for build
        if (vmajor.toInt() != 3 || vminor.toInt() != PYTHON3_VERSION) {
            printd( "%s is not version 3.%d, it's version %d.%d\n", pythonbinary.toStdString().c_str(), PYTHON3_VERSION, vmajor.toInt(), vminor.toInt());
            return false;
        }

        // now get python path
#ifdef WIN32
        pypath = path.replace("\\", "/");
#else
        pypath = path;
#endif
        return true;

    } else {

        // didn't understand !
        printd("Python output doesn't parse: %s\n", output.toStdString().c_str());
    }

    // by default we return false (pessimistic)
    return false;
}

PythonEmbed::PythonEmbed(const bool verbose, const bool interactive) : verbose(verbose), interactive(interactive)
{
    loaded = false;
    threadid=-1;
    name = QString("GoldenCheetah");

    // config or environment variable
    QString PYTHONHOME = appsettings->value(NULL, GC_PYTHON_HOME, "").toString();
    if (PYTHONHOME == "") PYTHONHOME = QProcessEnvironment::systemEnvironment().value("PYTHONHOME", "");
    else {
        qputenv("PYTHONHOME",PYTHONHOME.toUtf8());
    }
    if (PYTHONHOME !="") printd("PYTHONHOME setting used: %s\n", PYTHONHOME.toStdString().c_str());

    // is python3 installed?
    if (pythonInstalled(pybin, pypath, PYTHONHOME)) {

        printd("Python is installed: %s\n", pybin.toStdString().c_str());

        // tell python our program name - pretend to be the usual interpreter
        printd("Py_SetProgramName: %s\n", pybin.toStdString().c_str());
        Py_SetProgramName((wchar_t*) pybin.toStdString().c_str());

        // our own module
        printd("PyImport_AppendInittab: goldencheetah\n");
        PyImport_AppendInittab("goldencheetah", PyInit_goldencheetah);

        // need to load the interpreter etc
        printd("PyInitializeEx(0)\n");
        Py_InitializeEx(0);

        // set path - allocate storage for it...
        //printd("set path=%s\n", pypath.toStdString().c_str());
        //wchar_t *here = new wchar_t(pypath.length()+1);
        //pypath.toWCharArray(here);
        //here[pypath.length()]=0;
        //PySys_SetPath(here);

        // set the module path in the same way the interpreter would
        printd("PyImportModule('sys')\n");
        PyObject *sys = PyImport_ImportModule("sys");

        // did module import fail (python not installed properly?)
        if (sys != NULL)  {

            printd("Add '.' to Path\n");
            PyObject *path = PyObject_GetAttrString(sys, "path");
            PyList_Append(path, PyUnicode_FromString("."));

            // get version
            printd("Py_GetVersion()\n");
            version = QString(Py_GetVersion());
            version.replace("\n", " ");

            fprintf(stderr, "Python loaded [%s]\n", version.toStdString().c_str()); fflush(stderr);

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

            printd("Install stdio catcher\n");
            PyRun_SimpleString(stdOutErr.c_str()); //invoke code to redirect

            // now load the library
            printd("Load library.py\n");
            QFile lib(":python/library.py");
            if (lib.open(QFile::ReadOnly)) {
                QString libstring=lib.readAll();
                lib.close();
                PyRun_SimpleString(libstring.toLatin1().constData());
            }


            // setup trapping of output
            printd("Get catcher refs\n");
            PyObject *pModule = PyImport_AddModule("__main__"); //create main module
            catcher = static_cast<void*>(PyObject_GetAttrString(pModule,"catchOutErr"));
            clear = static_cast<void*>(PyObject_GetAttrString(static_cast<PyObject*>(catcher), "__init__"));
            PyErr_Print(); //make python print any errors
            PyErr_Clear(); //and clear them !

            // prepare for threaded processing
            printd("PyEval_InitThreads\n");
            PyEval_InitThreads();
            mainThreadState = PyEval_SaveThread();
            loaded = true;

            printd("Embedding completes\n");
            return;
        } // sys != NULL
    } // pythonInstalled == true

    // if we get here loading failed
    printd("Embedding failed\n");
    // Only inform the user if Python embedding is enabled
    if (appsettings->value(NULL, GC_EMBED_PYTHON, false).toBool()) {
        QMessageBox msg(QMessageBox::Information, QObject::tr("Python not installed or in path"), QObject::tr("Python v3.%1 is required for Python embedding.\nPython disabled in preferences.").arg(PYTHON3_VERSION));
        msg.exec();
    }
    appsettings->setValue(GC_EMBED_PYTHON, false);
    loaded=false;
    return;
}

// run on called thread
void PythonEmbed::runline(ScriptContext scriptContext, QString line)
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
    contexts.insert(threadid, scriptContext);

    // run and generate errors etc
    messages.clear();

    if (scriptContext.interactiveShell) {
        PyObject *m, *d, *v;
        m = PyImport_AddModule("__main__");
        d = PyModule_GetDict(m);
        v = PyRun_StringFlags(line.toStdString().c_str(), Py_single_input, d, d, 0);
        if (v) Py_DECREF(v);
    } else {
        PyRun_SimpleString(line.toStdString().c_str());
    }

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
