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
#include <QStringList>

#ifdef slots // clashes with python headers
#undef slots
#endif
#include <Python.h>

// we only really support Python 3, so lets only work on that basis
#if PY_MAJOR_VERSION >= 3
#define PYTHON3_VERSION PY_MINOR_VERSION
#endif

#include <stdarg.h>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>

// Buffer for debug logs
static std::vector<QString> g_debugBuffer;

// Log helper — buffers messages; dumped via qDebug only on init failure
void debugLog(const char* fmt, ...) {
    char buf[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    g_debugBuffer.push_back(QString::fromLocal8Bit(buf).trimmed());
}

// Redefine printd to use our logger
#undef printd
#define printd(...) debugLog(__VA_ARGS__)

// global instance of embedded python
PythonEmbed *python;
PyThreadState *mainThreadState;

// SIP module with GoldenCheetah Bindings
extern "C" {
extern PyObject *PyInit_goldencheetah(void);
};

QString
PythonEmbed::buildVersion()
{
    return QString("%1.%2.%3").arg(PY_MAJOR_VERSION).arg(PY_MINOR_VERSION).arg(PY_MICRO_VERSION);
}

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

        printd("Binary found:%d\n", (int)installnames.count());
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
                pybin=pythonbinary;
                printd("Binary found\n");
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
    py.setProcessChannelMode(QProcess::ForwardedErrorChannel);

    // If checking a specific PYTHONHOME (e.g. bundled), ensure the process uses it
    // and doesn't get confused by local user environment variables.
    if (!PYTHONHOME.isEmpty()) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("PYTHONHOME", PYTHONHOME);
        env.remove("PYTHONPATH"); // Ensure isolation from user's python libs
        py.setProcessEnvironment(env);
    }

    py.start();

    // failed to start python
    if (py.waitForStarted(500) == false) {
        fprintf(stderr, "Failed to start: %s\n", pythonbinary.toStdString().c_str());
        py.terminate();
        return false;
    }

    // wait for output, should be rapid
    if (py.waitForReadyRead(4000)==false) {
        fprintf(stderr, "Didn't get output: %s\n", pythonbinary.toStdString().c_str());
        py.terminate();
        return false;
    }

    // get output
    QString output = py.readAll();

    // close if it didn't already
    if (py.waitForFinished(500)==false) {
        fprintf(stderr, "forced terminate of %s\n", pythonbinary.toStdString().c_str());
        py.terminate();
    }

    // scan output
    QRegExp contents("^ZZ(.*)ZZ.*ZZ(.*)ZZ.*ZZ(.*)ZZ.*$");
    printd("Output: %s\n", output.toStdString().c_str());
    if (contents.exactMatch(output)) {
        QString vmajor=contents.cap(1);
        QString vminor=contents.cap(2);
        QString path=contents.cap(3);

        // check its Python 3 matching the version used for build
        if (vmajor.toInt() != 3 || vminor.toInt() != PYTHON3_VERSION) {
            fprintf(stderr, "Python version mismatch: GoldenCheetah was built with Python 3.%d, but found Python %d.%d at %s\n",
                    PYTHON3_VERSION, vmajor.toInt(), vminor.toInt(), pythonbinary.toStdString().c_str());
            return false;
        }

        // now get python path
#ifdef WIN32
        pypath = path.replace("\\", "/");
#else
        pypath = path;
#endif
        printd("Python path: %s\n", pypath.toStdString().c_str());
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
    chart = NULL;
    perspective = NULL;
    threadid=-1;
    name = QString("GoldenCheetah");

    // register metatypes used to pass between threads
    qRegisterMetaType<QVector<double> >();
    qRegisterMetaType<QStringList>();


    // Deployed Python location
    QString deployedPython = QCoreApplication::applicationDirPath();
#if defined(Q_OS_MAC)
    deployedPython += "/../Frameworks/Python.framework/Versions/Current";
#elif defined(Q_OS_LINUX)
    deployedPython += QString("/opt/python3.%1").arg(PYTHON3_VERSION);
#endif

    // config, deployed or environment variable
    QString PYTHONHOME = appsettings->value(NULL, GC_PYTHON_HOME, "").toString().trimmed();
    if (PYTHONHOME == "") {
        if (pythonInstalled(pybin, pypath, deployedPython)) {
            PYTHONHOME = deployedPython;
            qputenv("PYTHONHOME",PYTHONHOME.toUtf8());
        } else {
            PYTHONHOME = QProcessEnvironment::systemEnvironment().value("PYTHONHOME", "");
        }
    } else {
        qputenv("PYTHONHOME",PYTHONHOME.toUtf8());
    }
    if (PYTHONHOME !="") printd("PYTHONHOME setting used: %s\n", PYTHONHOME.toStdString().c_str());

    // is python3 installed?
    if (pythonInstalled(pybin, pypath, PYTHONHOME)) {

        printd("Python is installed: %s\n", pybin.toStdString().c_str());

        // tell python our program name - pretend to be the usual interpreter
        printd("Py_SetProgramName: %s\n", pybin.toStdString().c_str()); // not wide char string as printd uses printf not wprintf
        Py_SetProgramName((wchar_t*) pybin.toStdWString().c_str());

        // our own module
        printd("PyImport_AppendInittab: goldencheetah\n");
        PyImport_AppendInittab("goldencheetah", PyInit_goldencheetah);

        // need to load the interpreter etc
        printd("PyInitializeEx(0)\n");
        Py_InitializeEx(0);

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
                                     "    def flush(self):\n"
                                     "        pass\n"
                                     "catchOutErr = CatchOutErr()\n"
                                     "sys.stdout = catchOutErr\n"
                                     "sys.stderr = catchOutErr\n"
                                     "import goldencheetah\n"
                                     "GC=goldencheetah.Bindings()\n");

            printd("Install stdio catcher\n");
            PyRun_SimpleString(stdOutErr.c_str()); //invoke code to redirect

            // Retrieve and prepare user library paths
            QStringList userPaths = appsettings->value(NULL, GC_PYTHON_USER_LIBRARY_PATHS).toStringList();

            // Ensure directories exist and add to sys.path
            for (const QString &path : userPaths) {
                QDir dir(path);
                if (!dir.exists()) {
                    dir.mkpath(".");
                }
                if (dir.exists()) {
                     QString code = QString("import sys; sys.path.append(r'%1')").arg(path);
                     PyRun_SimpleString(code.toStdString().c_str());
                     printd("Added to sys.path: %s\n", path.toStdString().c_str());
                }
            }
 #ifdef Q_OS_LINUX
            // ensure site-packages is in path when using deployed Python on Linux
            if (PYTHONHOME == deployedPython) {
                std::string ensureSitePackages = ("import sys\n"
                                                  "sys.path.append(sys.prefix+'/lib/python3.'+str(sys.version_info.minor)+'/site-packages')\n");
                PyRun_SimpleString(ensureSitePackages.c_str()); //invoke code
            }
 #endif
#ifdef Q_OS_MAC
            // ensure site-packages is in path when using deployed Python on macOS
            if (PYTHONHOME.contains("Python.framework")) {
                std::string ensureSitePackages = ("import sys, os\n"
                                                  "lib_ver = 'python%d.%d' % (sys.version_info.major, sys.version_info.minor)\n"
                                                  "site_pkg = os.path.join(sys.prefix, 'lib', lib_ver, 'site-packages')\n"
                                                  "if os.path.exists(site_pkg) and site_pkg not in sys.path:\n"
                                                  "    sys.path.append(site_pkg)\n");
                PyRun_SimpleString(ensureSitePackages.c_str()); //invoke code
            }
 #endif
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

            // Verify dependencies, something in Python works but maybe the packaging didn't.
            if (!checkDependencies()) {
                for (const auto &line : g_debugBuffer) {
                    qDebug() << line;
                }
            }
            return;
        } // sys != NULL
    } // pythonInstalled == true

    // if we get here loading failed
    fprintf(stderr, "Python embedding failed. GoldenCheetah requires Python 3.%d installed and in PATH.\n", PYTHON3_VERSION);
    // Notify user of the problem (they can disable Python in preferences if they don't want to see this)
    // Note: We don't permanently disable Python here - the user might fix the issue (install Python,
    // fix PYTHONHOME, etc.) and we should try again on next startup.
    // Dump debug log.
    for (const auto &line : g_debugBuffer) {
        qDebug() << line;
    }
    QMessageBox msg(QMessageBox::Warning, QObject::tr("Python not available"),
                    QObject::tr("GoldenCheetah was built with Python 3.%1 but could not initialize Python.\n\n"
                                "Please ensure Python 3.%1 is installed and in your PATH.\n"
                                "You can disable Python in Options > General if you don't need it.\n\n").arg(PYTHON3_VERSION));
    msg.exec();
    loaded=false;
    return;
}

bool PythonEmbed::checkDependencies() const
{
    // Ensure we hold the GIL, as PyEval_SaveThread released it
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    printd("Checking python dependencies...\n");

    // List of modules to check from requirements.txt
    const QStringList modules = {
        "numpy", "pandas", "scipy", "lmfit", "plotly", "importlib_metadata"
    };

    QStringList missing;

    // Use a clean local scope to avoid polluting global namespace
    // We import, then check if it's there.
    for (const auto& module : modules) {
        // We try to import within a function/local scope if possible, but PyRun_SimpleString runs in __main__
        // A better way is to plain import and letting it fail usually prints to stderr which we capture.
        // To avoid pollution we can just 'del module' afterwards or wrap in a try block.
        // wrapping in try/except block that prints to stderr on failure
        QString code = QString("try:\n"
                               "    import %1\n"
                               "except ImportError as e:\n"
                               "    print(f'Dependency Check Failed: {e}')\n"
                               "    exit(1)\n").arg(module);

        // PyRun_SimpleString returns 0 on success, -1 on failure (if exit(1) is called it might kill the app? No, exit() in python raises SystemExit)
        // Actually exit(1) in embedded python might terminate the process if not handled?
        // Let's NOT use exit(). Let's set a variable.

        code = QString("try:\n"
                       "    import %1\n"
                       "except ImportError as e:\n"
                       "    print(f'Dependency Check Failed: {e}')\n"
                       "    raise e\n").arg(module);

        if (PyRun_SimpleString(code.toStdString().c_str()) != 0) {
            QString msg = QString("Failed to import module: %1").arg(module);
            debugLog(msg.toStdString().c_str());
            missing.push_back(module);
        }
    }

    if (!missing.empty()) {
        QString errorMsg = QObject::tr("GoldenCheetah Python support is enabled, but the following required packages could not be imported:\n\n%1\n\n"
                                       "Please check the log for details.").arg(missing.join(", "));

        // We should warn the user.
        // But we are in the constructor, potentially on the main thread? Yes.
        QMessageBox::warning(0, QObject::tr("Python Dependencies Missing"), errorMsg);
    } else {
        printd("All dependencies imported successfully.\n");
    }

    PyGILState_Release(gstate);
    return missing.empty();
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
