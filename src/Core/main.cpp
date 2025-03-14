/*
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
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

#include "Context.h"
#include "Athlete.h"
#include "MainWindow.h"
#include "Settings.h"
#include "CloudService.h"
#include "TrainDB.h"
#include "Colors.h"
#include "GcUpgrade.h"
#include "IdleTimer.h"
#include "PowerProfile.h"
#include "GcCrashDialog.h" // for versionHTML
#include "OverviewItems.h"

#include <QApplication>
#include <QtGui>
#include <QFile>
#include <QMessageBox>
#include "ChooseCyclistDialog.h"
#ifdef GC_WANT_HTTP
#include "httplistener.h"
#include "httprequesthandler.h"
#endif
#ifdef GC_HAS_CLOUD_DB
#include "CloudDBCommon.h"
#endif
#ifdef GC_WANT_PYTHON
#include "PythonEmbed.h"
#include "FixPySettings.h"
#endif
#include <signal.h>


#ifdef GC_WANT_X11
#include <X11/Xlib.h>
#endif

#include <QStandardPaths>

#include <gsl/gsl_errno.h>

#ifdef Q_OS_WIN
#if QT_VERSION > 0x060500
#include <QStyleFactory>
#endif
#endif

//
// bootstrap state
//
bool restarting = false;
static bool nogui;
static int gc_opened=0;

//
// global application
//
QString gcroot;
QApplication *application;

#ifdef GC_WANT_HTTP
#include "APIWebService.h"
HttpListener *listener = NULL;
#endif

// R is not multithreaded, has a single instance that we setup at startup.
#ifdef GC_WANT_R
#include <RTool.h>
// All R Runtime elements encapsulated in RTool
RTool *rtool = NULL;
#endif

//
// Trap signals / termination
//
void terminate(int code)
{
#ifdef GC_WANT_HTTP
    if (listener) listener->close();
#endif

    // tidy up static stuff (our globals) that are not tied
    // to a mainwindow instance (which will be deleted on close)
#ifdef GC_WANT_PYTHON
    delete fixPySettings;
#endif
    delete appsettings;
    application->exit();

    // because QT starts a bunch of threads (e.g. reading XcbEvents)
    // calling exit() during startup is a no-no. So we go nuclear and
    // exit without calling the static destructors via _Exit(), unless we did
    // actually open an athlete and start-up proper with app->exec().
    if (gc_opened) exit(code);
    else _Exit(code);
}

//
// redirect logging
//
#ifdef GC_WANT_HTTP
void myMessageOutput(QtMsgType type, const QMessageLogContext &, const QString &string)
 {
    QByteArray ba = string.toLocal8Bit();
    const char *msg = ba.constData();
     //in this function, you can write the message to any stream!
     switch (type) {
     default: // QtInfoMsg from 5.5 would arrive here
     case QtDebugMsg:
         fprintf(stderr, "Debug: %s\n", msg);
         break;
     case QtWarningMsg: // supress warnings unless server mode
         if (nogui) fprintf(stderr, "Warning: %s\n", msg);
         break;
     case QtCriticalMsg:
         fprintf(stderr, "Critical: %s\n", msg);
         break;
     case QtFatalMsg:
         fprintf(stderr, "Fatal: %s\n", msg);
         abort();
     }
 }

void sigabort(int x)
{
    terminate(x);
}
#endif

//
// redirect stderr to `home'/goldencheetah.log or to the file specified with --debug-file
//
#include <stdio.h>
#include <cstdio>
#include <iostream>

#ifdef WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

void nostderr(QString file)
{
    int fd;
    int fd_stderr = 2;
    FILE *fp;

    // On Windows, stderr is not connected to fd_stderr = 2
    // freopen seems the only function available to redirect stderr
    qDebug() << "GoldenCheetah: redirecting log messages (stderr) to file " << file;
    fp = freopen(file.toLocal8Bit(), "w", stderr);
    if (fp == NULL) {
        qDebug() << "GoldenCheetah: cannot redirect stderr, unable to open file " << file;
        return;
    }

    fd = fileno(stderr); 
    if (fd < 0) {
        qDebug() << "GoldenCheetah: invalid handle obtained from stderr " << fd;
        return;
    }

    // We redirect fd_stderr to this new file with dup2(), in case some libraries write fd_stderr.
    // Normally we would close fd right after it is duplicated, but not in this case.
    // Indeed stderr still uses fd. Both fd and fd_stderr may be used interchangeably after dup2().
    int ret = dup2(fd, fd_stderr);
    if (ret < 0) {
        qDebug() << "Goldencheetah: cannot redirect STDERR_FILENO, dup2 failed with return value " << ret;
        return;
    }

    // Synchronize cerr with the new stderr
    std::ios::sync_with_stdio();

#ifdef WIN32
    // Redirect STD_ERROR_HANDLE to the new file   
    HANDLE fileHandle = (HANDLE)_get_osfhandle(fd_stderr);
    if(fileHandle == INVALID_HANDLE_VALUE) qDebug() << "GoldenCheetah: cannot get Win32 HANDLE for stderr";
    
    bool res = SetStdHandle(STD_ERROR_HANDLE, fileHandle);
    if (!res) qDebug() << "GoldenCheetah: cannot redirect STD_ERROR_HANDLE";
#endif
}

//
// By default will open last athlete, but will also provide
// a dialog to select an athlete if not found, and then upgrade
// the athlete one selected before opening a mainwindow
//
// It will also respawn mainwindows when restarting for changes
// to application settings (athlete folder, language)
//
// Also creates singleton instances prior to application launching
//
int
main(int argc, char *argv[])
{
    int ret=2; // return code from qapplication, default to error

#ifdef Q_OS_WIN
    // On Windows without console, we try to attach to the parent's console
    // and redirect stderr and stdout on success, to have a more Unix-like
    // behavior when launched from cmd or PowerShell.
    if (_fileno(stderr) == -2 && AttachConsole(ATTACH_PARENT_PROCESS )) {
        freopen("CONOUT$", "w", stderr);
        freopen("CONOUT$", "w", stdout);
    }
#if QT_VERSION < 0x060000
    bool angle=true;
#endif
#endif

    //
    // PROCESS COMMAND LINE SWITCHES
    //

    // snaffle arguments into a stringlist we can play with into sargs
    // and only keep non-switch args in the args string list
    QStringList sargs, args;
    for (int i=0; i<argc; i++) sargs << argv[i];
#ifdef GC_WANT_PYTHON
    bool noPy=false;
#endif
#ifdef GC_WANT_R
    bool noR=false;
#endif
#ifdef GC_DEBUG
    bool debug = true;
    QString debugFormat = QString("[%{time h:mm:ss.zzz}] %{type}: %{message} (%{file}:%{line})");
#else
    bool debug = false;
    QString debugFormat = QString("[%{time h:mm:ss.zzz}] %{type}: %{message}");
#endif

    // By default, print messages from all categories, but not those from
    // specialised categories like qt.* or gc.* which can be quite verbose
    QString debugRules = QString("*.debug=true;gc.*.debug=false;qt.*.debug=false");
    QString debugFile = QString();

    bool server = false;
    nogui = false;
    bool help = false;

    // honour command line switches
    QString arg;
    for(int i = 0; i < sargs.length();) {
        arg = sargs[i];
        i++;

        // help, usage or version requested, basic information
        if (arg == "--help" || arg == "--usage" || arg == "--version") {

            help = true;
            fprintf(stderr, "GoldenCheetah %s (%d)\n", VERSION_STRING, VERSION_LATEST);

        }

        // help or usage requrested, additional information
        if (arg == "--help" || arg == "--usage") {

            fprintf(stderr, "usage: GoldenCheetah [[directory] athlete]\n\n");
            fprintf(stderr, "--help or --usage   to print this message and exit\n");
            fprintf(stderr, "--version           to print detailed version information and exit\n");
#ifdef GC_WANT_HTTP
            fprintf(stderr, "--server            to run as an API server\n");
#endif
#ifdef GC_DEBUG
            fprintf(stderr, "--debug             to turn on redirection of messages to goldencheetah.log [debug build]\n");
#else
            fprintf(stderr, "--debug             to direct diagnostic messages to the terminal instead of goldencheetah.log\n");
#endif
            fprintf(stderr, "--debug-file file   to direct diagnostic messages to file\n");
            fprintf(stderr, "--debug-rules \"rules\" to specify which diagnostic messages to output, using the same syntax as QT_LOGGING_RULES\n");
            fprintf(stderr, "--debug-format \"format\" to specify the format of diagnostic messages, using the same syntax as QT_MESSAGE_PATTERN\n");

#ifdef GC_HAS_CLOUD_DB
            fprintf(stderr, "--clouddbcurator    to add CloudDB curator specific functions to the menus\n");
#endif
#ifdef GC_WANT_PYTHON
            fprintf(stderr, "--no-python         to disable Python startup\n");
#endif
#ifdef GC_WANT_R
            fprintf(stderr, "--no-r              to disable R startup\n");
#endif
#ifdef Q_OS_WIN
#if QT_VERSION < 0x060000
            fprintf(stderr, "--no-angle          to disable ANGLE rendering\n");
#endif
#endif
            fprintf (stderr, "\nSpecify the folder and/or athlete to open on startup\n");
            fprintf(stderr, "If no parameters are passed it will reopen the last athlete.\n\n");

        // version requested, additional information
        } else if (arg == "--version") {

            QString html = GcCrashDialog::versionHTML();
            html.replace("</td><td>", ": "); // to maintain colums in one line
            QString text = QTextDocumentFragment::fromHtml(html).toPlainText();
            QByteArray ba = text.toLocal8Bit();
            const char *c_str = ba.data();
            fprintf(stderr, "\n%s\n\n", c_str);

        } else if (arg == "--server") {
#ifdef GC_WANT_HTTP
            nogui = server = true;
#else
            fprintf(stderr, "HTTP support not compiled in, exiting.\n");
            exit(1);
#endif

#ifdef GC_WANT_PYTHON
        } else if (arg == "--no-python") {

            noPy = true;
#endif
#ifdef GC_WANT_R
        } else if (arg == "--no-r") {

            noR = true;
#endif
        } else if (arg == "--debug") {

#ifdef GC_DEBUG
            // debug, so don't redirect stderr!
            debug = false;
#else
            debug = true;
#endif
        } else if (arg == "--debug-file" && i < sargs.length()) {
            debugFile = QString(sargs[i]);
            i++;
        } else if (arg == "--debug-format" && i < sargs.length()) {
            debugFormat = QString(sargs[i]);
            i++;
        } else if (arg == "--debug-rules" && i < sargs.length()) {
            debugRules = QString(sargs[i]);
            i++;
        } else if (arg == "--clouddbcurator") {
#ifdef GC_HAS_CLOUD_DB
            CloudDBCommon::addCuratorFeatures = true;
#else
            fprintf(stderr, "CloudDB support not compiled in, exiting.\n");
            exit(1);
#endif
#ifdef Q_OS_WIN
#if QT_VERSION < 0x060000
        } else if (arg == "--no-angle") {
            angle = false;
#endif
#endif
        } else {

            // not switches !
            args << arg;
        }
    }

    #if 0 // quick hack to get list of metrics and descriptions
    RideMetricFactory::instance().initialize();

    const RideMetricFactory &factory = RideMetricFactory::instance();
    QHashIterator<QString,RideMetric*> it(factory.metricHash());
    it.toFront();
    while(it.hasNext()) {
        it.next();
        fprintf(stderr, "%s|%s\n",
                it.value()->name().toUtf8().data(),
                it.value()->description().toUtf8().data());
    }
    exit(0);
    #endif

    // help or version printed so just exit now
    if (help) {
        exit(0);
    }

    //
    // INITIALISE ONE TIME OBJECTS
    //
#ifdef GC_WANT_HTTP
    listener = NULL;
#endif

#ifdef GC_WANT_X11
    XInitThreads();
#endif

#ifdef GC_WANT_R
    rtool = NULL;
#endif
#ifdef GC_WANT_PYTHON
    python = NULL;
#endif

    // numerous bugs related to autoscaling and opengl that have persisted since Qt5.6 on and off
    // now we support hidpi natively we will unset scaling factors and use our own scaling ratios
#ifdef Q_OS_LINUX
    unsetenv("QT_SCALE_FACTOR");
#endif

    // we don't want program aborts when maths routines don't know
    // what to do. We may add our own error handler later.
    gsl_set_error_handler_off();

#ifdef Q_OS_WIN
#if QT_VERSION < 0x060000
    if (angle) {
        // windows we use ANGLE for opengl on top of DirectX/Direct3D
        // it avoids issues with bad graphics drivers
        QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
    }
#endif
#endif

    // create the application -- only ever ONE regardless of restarts
    application = new QApplication(argc, argv);

#ifdef Q_OS_WIN
#if QT_VERSION > 0x060500
    if (application->style()->name() == "windows11") {
        application->setStyle(QStyleFactory::create("Windows"));
        qDebug()<<"Replacing windows11 by Windows style to avoid a Qt bug";
    }
#endif
#endif

    //XXXIdleEventFilter idleFilter;
    //XXXapplication->installEventFilter(&idleFilter);

    // read defaults
    initPowerProfile();

    // output colors as configured so we can cut and paste into Colors.cpp
    // uncomment when developers working on theme colors
    //GCColor::dumpColors();

    // get defaults in case need them later
    AppearanceSettings defaults = GSettings::defaultAppearanceSettings();

    // set font family and size
    QFont font;
    font.fromString(appsettings->value(NULL, GC_FONT_DEFAULT, defaults.fontfamily).toString());
    font.setPointSize(defaults.fontpointsize);

    // use the default before taking into account screen size
    baseFont = font;

    // hidpi ratios
    dpiXFactor = defaults.xfactor;
    dpiYFactor = defaults.yfactor;

#ifndef Q_OS_MAC // not needed on a Mac

    application->setStyleSheet(QString("QPushButton { padding-left: %1px; padding-right: %1px; "
                                       "              padding-top: %2px; padding-bottom: %2px; }"
                                       "QComboBox   { padding-left: %1px; padding-right: %1px; }")
                                       .arg(15*dpiXFactor)
                                       .arg(3*dpiYFactor));

#endif

    // scale up to user scale factor
    double fontscale = appsettings->value(NULL, GC_FONT_SCALE, defaults.fontscale).toDouble();
    font.setPointSizeF(baseFont.pointSizeF() * fontscale);

    // now apply !
    application->setFont(font); // set default font

    // set application wide
    appsettings->setValue(GC_FONT_DEFAULT, font.toString());
    appsettings->setValue(GC_FONT_CHARTLABELS, font.toString());
    appsettings->setValue(GC_FONT_DEFAULT_SIZE, font.pointSizeF());
    appsettings->setValue(GC_FONT_CHARTLABELS_SIZE, font.pointSizeF() * 0.8);


    // what filestores are registered (whilst we refactor)
    //qDebug()<<"Cloud services registered:"<<CloudServiceFactory::instance().serviceNames();

    //
    // OPEN FIRST MAINWINDOW
    //
    do {

        // lets not restart endlessly
        restarting = false;

        // we reload R if we are restarting to get that, but
        // only if it failed
#ifdef GC_WANT_R
        // create the singleton in the main thread
        // will be shared by all athletes and all charts (!!)
        if (noR) {
            rtool = NULL;
        } else if (rtool == NULL && appsettings->value(NULL, GC_EMBED_R, true).toBool()) {
            rtool = new RTool();
            if (rtool->failed == true) rtool=NULL;
        }
#endif

#ifdef GC_WANT_PYTHON
        bool embed = appsettings->value(NULL, GC_EMBED_PYTHON, true).toBool();
        if (embed && noPy == false && python == NULL) {
            python = new PythonEmbed(); // initialise python in this thread ?
            if (python->loaded == false) python=NULL;
        }
#endif

        //this is the path within the current directory where GC will look for
        //files to allow USB stick support
        QString localLibraryPath="Library/GoldenCheetah";

        //this is the path that used to be used for all platforms
        //now different platforms will use their own path
        //this path is checked first to make things easier for long-time users
        QString oldLibraryPath=QDir::home().canonicalPath()+"/Library/GoldenCheetah";

        //these are the new platform-dependent library paths
#if defined(Q_OS_MACX)
        QString libraryPath="Library/GoldenCheetah";
#elif defined(Q_OS_WIN)
        QStringList paths=QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation);
        QString libraryPath = paths.at(0); 
#else // not windows or osx (must be Linux or OpenBSD)
        // Q_OS_LINUX et al
        QString libraryPath=".goldencheetah";
#endif //

        // or did we override in settings?
        QString sh;
        if ((sh=appsettings->value(NULL, GC_HOMEDIR, "").toString()) != QString("")) localLibraryPath = sh;

        // lets try the local library we've worked out...
        QDir home = QDir();
        if(QDir(localLibraryPath).exists() || home.exists(localLibraryPath)) {

            home.cd(localLibraryPath);

        } else {

            // YIKES !! The directory we should be using doesn't exist!
            home = QDir::home();
            if (home.exists(oldLibraryPath)) { // there is an old style path, lets fo there
                home.cd(oldLibraryPath);
            } else {

                if (!home.exists(libraryPath)) {
                    if (!home.mkpath(libraryPath)) {

                        // tell user why we aborted !
                        QMessageBox::critical(NULL, "Exiting", QString("Cannot create library directory (%1)").arg(libraryPath));
                        exit(0);
                    }
                }
                home.cd(libraryPath);
            }
        }

        // set global root directory
        gcroot = home.canonicalPath();
        appsettings->initializeQSettingsGlobal(gcroot);


        // now redirect stderr and set the log filter and format
        if (debugFile != QString()) nostderr(debugFile);
        else if (!debug) nostderr(QString("%1/%2").arg(home.canonicalPath()).arg("goldencheetah.log"));
        qSetMessagePattern(debugFormat);
        QLoggingCategory::setFilterRules(debugRules.replace(";", "\n")); // accept ; as separator like QT_LOGGING_RULES

        // Language setting (default to system locale)
        QVariant lang = appsettings->value(NULL, GC_LANG, QLocale::system().name());

        // install QT Translator to enable QT Dialogs translation
        QTranslator qtTranslator;
        if (!qtTranslator.load("qt_" + lang.toString(), QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
            qDebug()<<"Failed to load Qt translator for "<<lang.toString();
        application->installTranslator(&qtTranslator);

        // Load specific translation, try from GCROOT otherwise from binary
        QTranslator gcTranslator;
        QString translation_file = "/gc_" + lang.toString() + ".qm";
        if (gcTranslator.load(gcroot + translation_file))
            qDebug() << "Loaded translation from"+gcroot+translation_file;
        else if (!gcTranslator.load(":translations" + translation_file))
            qDebug()<<"Failed to load GC translator for "<<lang.toString();
        application->installTranslator(&gcTranslator);

        // Now the translator is installed, set default colors with translated names
        GCColor::setupColors();

        // has a default theme been applied (first run) ?
        QString powercolor = appsettings->value(NULL, "COLORPOWER", "").toString();
        if (powercolor == "")  GCColor::applyTheme(defaults.theme);

        // migration
        appsettings->migrateQSettingsSystem(); // colors must be setup before migration can take place, but reading has to be from the migrated ones
        GCColor::readConfig();

        // Initialize metrics once the translator is installed
        RideMetricFactory::instance().initialize();

        // Initialize global registry once the translator is installed
        GcWindowRegistry::initialize();

        // initialize Overview Items once the translator is installed
        OverviewItemConfig::registerItems();

        // initialise the trainDB
        trainDB = new TrainDB(home);

        // lets do what the command line says ...
        QVariant lastOpened;
        if(args.count() == 2) { // $ ./GoldenCheetah Mark -or- ./GoldenCheetah --server ~/athletedir

            // athlete
            if (!server) lastOpened = args.at(1);
            else home.cd(args.at(1));

        } else if (args.count() == 3) { // $ ./GoldenCheetah ~/Athletes Mark

            // first parameter is a folder that exists?
            if (QFileInfo(args.at(1)).isDir()) {
                home.cd(args.at(1));
            }

            // folder and athlete
            lastOpened = args.at(2);

        } else if (appsettings->value(NULL, GC_OPENLASTATHLETE, true).toBool()) {

            // no parameters passed lets open the last athlete we worked with
            lastOpened = appsettings->value(NULL, GC_SETTINGS_LAST);

            // does lastopened Directory exists at all
            QDir lastOpenedDir(gcroot+"/"+lastOpened.toString());
            if (lastOpenedDir.exists()) {
                // but hang on, did they crash? if so we need to open with a menu
                appsettings->initializeQSettingsAthlete(gcroot, lastOpened.toString());
                if(appsettings->cvalue(lastOpened.toString(), GC_SAFEEXIT, true).toBool() != true)
                    lastOpened = QVariant();
            } else {
                lastOpened = QVariant();
            }
            
        }

#ifdef GC_WANT_HTTP

        // The API server offers webservices (default port 12021, see httpserver.ini)
        // This is to enable integration with R and similar
        if (appsettings->value(NULL, GC_START_HTTP, false).toBool() || server) {

            // notifications etc
            if (nogui) {
                qDebug()<<"Starting GoldenCheetah API web-services... (hit ^C to close)";
                qDebug()<<"Athlete directory:"<<home.absolutePath();
            } else {
                // switch off warnings if in gui mode
#ifndef GC_WANT_ALLDEBUG
                qInstallMessageHandler(myMessageOutput);
#endif
            }

            QString httpini = home.absolutePath() + "/httpserver.ini";
            if (!QFile(httpini).exists()) {

                // read default ini file
                QFile file(":webservice/httpserver.ini");
                QString content;
                if (file.open(QIODevice::ReadOnly)) {
                    content = file.readAll();
                    file.close();
                }

                // write default ini file
                QFile out(httpini);
                if (out.open(QIODevice::WriteOnly)) {
                
                    out.resize(0);
                    QTextStream stream(&out);
                    stream << content;
                    out.close();
                }
            }

            // use the default handler (just get an error page)
            QSettings* settings=new QSettings(httpini,QSettings::IniFormat,application);

            if (listener) {
                // when changing the Athlete Directory, there is already a listener running
                // close first to avoid errors
                listener->close();
            }
            listener=new HttpListener(settings,new APIWebService(home, application),application);

            // if not going on to launch a gui...
            if (nogui) {
                // catch ^C exit
                signal(SIGINT, sigabort);

                ret = application->exec();

                // stop web server if running
                qDebug()<<"Stopping GoldenCheetah API web-services...";
                listener->close();

                // and done
                terminate(0);
            }
        }
#endif

        // lets attempt to open as asked/remembered
        bool anyOpened = false;
        if (lastOpened != QVariant()) {
            QStringList list = lastOpened.toStringList();
            QStringListIterator i(list);
            while (i.hasNext()) {
                QString cyclist = i.next();
                QString homeDir = home.canonicalPath();
                if (home.cd(cyclist)) {
                    appsettings->initializeQSettingsAthlete(homeDir, cyclist);
                    GcUpgrade v3;
                    if (v3.upgradeConfirmedByUser(home)) {
                        MainWindow *mainWindow = new MainWindow(home);
                        mainWindow->show();
                        mainWindow->ridesAutoImport();
                        gc_opened++;
                        home.cdUp();
                        anyOpened = true;
                    } else {
                        delete trainDB;
                        terminate(0);
                    }
                }
            }
        }

        // ack, didn't manage to open an athlete
        // and the upgradeWarning was
        // lets ask the user which / create a new one
        if (!anyOpened) {
            ChooseCyclistDialog d(home);
            d.setModal(true);

            // choose cancel?
            if ((ret=d.exec()) != QDialog::Accepted) {
                delete trainDB;
                terminate(0);
            }

            // chosen, so lets get the choice..
            QString homeDir = home.canonicalPath();
            home.cd(d.choice());
            if (!home.exists()) {
                delete trainDB;
                terminate(0);
            }

            appsettings->initializeQSettingsAthlete(homeDir, d.choice());
            // .. and open a mainwindow
            GcUpgrade v3;
            if (v3.upgradeConfirmedByUser(home)) {
                MainWindow *mainWindow = new MainWindow(home);
                mainWindow->show();
                mainWindow->ridesAutoImport();
                gc_opened++;
            } else {
                delete trainDB;
                terminate(0);
            }
        }

        ret=application->exec();

        // close trainDB
        delete trainDB;

        // reset QSettings (global & Athlete)
        appsettings->clearGlobalAndAthletes();

    } while (restarting);

    delete application;

    return ret;
}
