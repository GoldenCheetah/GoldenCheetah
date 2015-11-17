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
#include "TrainDB.h"
#include "Colors.h"
#include "GcUpgrade.h"

#include <QApplication>
#include <QtGui>
#include <QFile>
#include <QWebSettings>
#include <QMessageBox>
#include "ChooseCyclistDialog.h"
#ifdef GC_WANT_HTTP
#include "httplistener.h"
#include "httprequesthandler.h"
#endif

#include <signal.h>

// redirect errors to `home'/goldencheetah.log
// sadly, no equivalent on Windows
#ifndef WIN32
#include "stdio.h"
#include "unistd.h"
void nostderr(QString dir)
{
    // redirect stderr to a file
    QFile *fp = new QFile(QString("%1/goldencheetah.log").arg(dir));
    if (fp->open(QIODevice::WriteOnly|QIODevice::Truncate) == true) {
        close(2);
        if(dup(fp->handle()) == -1) fprintf(stderr, "GoldenCheetah: cannot redirect stderr\n");
    } else {
        fprintf(stderr, "GoldenCheetah: cannot redirect stderr\n");
    }
}
#endif

#ifdef Q_OS_MAC
#include "QtMacSegmentedButton.h" // for cocoa initialiser
#endif

#ifdef Q_OS_X11
#include <X11/Xlib.h>
#endif

#if QT_VERSION > 0x050000
#include <QStandardPaths>
#endif

bool restarting = false;

// root directory shared by all
QString gcroot;

QApplication *application;

bool nogui;
#ifdef GC_WANT_HTTP
#include "APIWebService.h"
#if QT_VERSION > 0x50000
void myMessageOutput(QtMsgType type, const QMessageLogContext &, const QString &string)
 {
    const char *msg = string.toLocal8Bit().constData();
#else
void myMessageOutput(QtMsgType type, const char *msg)
 {
#endif
     //in this function, you can write the message to any stream!
     switch (type) {
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

HttpListener *listener;
void sigabort(int)
{
    qDebug()<<""; // newline
    application->exit();
}
#endif

int
main(int argc, char *argv[])
{
    int ret=2; // return code from qapplication, default to error

    //
    // PROCESS COMMAND LINE SWITCHES
    //

    // snaffle arguments into a stringlist we can play with into sargs
    // and only keep non-switch args in the args string list
    QStringList sargs, args;
    for (int i=0; i<argc; i++) sargs << argv[i];

#ifdef GC_DEBUG
    bool debug = true;
#else
    bool debug = false;
#endif
    bool server = false;
    nogui = false;
    bool help = false;

    // honour command line switches
    foreach (QString arg, sargs) {

        // help or version requested
        if (arg == "--help" || arg == "--version") {

            help = true;
            fprintf(stderr, "GoldenCheetah %s (%d)\nusage: GoldenCheetah [[directory] athlete]\n\n", VERSION_STRING, VERSION_LATEST);
            fprintf(stderr, "--help or --version to print this message and exit\n");
#ifdef GC_WANT_HTTP
            fprintf(stderr, "--server to run as an API server\n");
#endif
#ifdef GC_DEBUG
            fprintf(stderr, "--debug             to turn on redirection of messages to goldencheetah.log [debug build]\n");
#else
            fprintf(stderr, "--debug             to direct diagnostic messages to the terminal instead of goldencheetah.log\n");
#endif
            fprintf (stderr, "\nSpecify the folder and/or athlete to open on startup\n");
            fprintf(stderr, "If no parameters are passed it will reopen the last athlete.\n\n");

        } else if (arg == "--server") {
#ifdef GC_WANT_HTTP
                nogui = server = true;
#else
                fprintf(stderr, "HTTP support not compiled in, exiting.\n");
                exit(1);
#endif

        } else if (arg == "--debug") {

#ifdef GC_DEBUG
            // debug, so don't redirect stderr!
            debug = false;
#else
            debug = true;
#endif

        } else {

            // not switches !
            args << arg;
        }
    }

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

#ifdef Q_OS_X11
    XInitThreads();
#endif

#ifdef Q_OS_MACX
    if ( QSysInfo::MacintoshVersion > QSysInfo::MV_10_8 )
    {
        // fix Mac OS X 10.9 (mavericks) font issue
        // https://bugreports.qt-project.org/browse/QTBUG-32789
        QFont::insertSubstitution("LucidaGrande", "Lucida Grande");
    }
#endif

    // create the application -- only ever ONE regardless of restarts
    application = new QApplication(argc, argv);

#ifdef Q_OS_MAC
    // get an autorelease pool setup
    static CocoaInitializer cocoaInitializer;
#endif

    // set default colors
    GCColor::setupColors();
    appsettings->migrateQSettingsSystem(); // colors must be setup before migration can take place, but reading has to be from the migrated ones
    GCColor::readConfig();

    // set defaultfont
    QFont font;
    font.fromString(appsettings->value(NULL, GC_FONT_DEFAULT, QFont().toString()).toString());
    font.setPointSize(appsettings->value(NULL, GC_FONT_DEFAULT_SIZE, 10).toInt());
    application->setFont(font); // set default font


    //
    // OPEN FIRST MAINWINDOW
    //
    do {

        // lets not restart endlessly
        restarting = false;

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
#if QT_VERSION > 0x050000 // windows and qt5
        QStringList paths=QStandardPaths::standardLocations(QStandardPaths::DataLocation);
        QString libraryPath = paths.at(0); 
#else // windows not qt5
        QString libraryPath=QDesktopServices::storageLocation(QDesktopServices::DataLocation) + "/GoldenCheetah";
#endif // qt5
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


        // now redirect stderr
#ifndef WIN32
        if (!debug) nostderr(home.canonicalPath());
#else
        Q_UNUSED(debug)
#endif

        // install QT Translator to enable QT Dialogs translation
        // we may have restarted JUST to get this!
        QTranslator qtTranslator;
        qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
        application->installTranslator(&qtTranslator);

        // Language setting (default to system locale)
        QVariant lang = appsettings->value(NULL, GC_LANG, QLocale::system().name());

        // Load specific translation
        QTranslator gcTranslator;
        gcTranslator.load(":translations/gc_" + lang.toString() + ".qm");
        application->installTranslator(&gcTranslator);

        // Initialize metrics once the translator is installed
        RideMetricFactory::instance().initialize();

        // Initialize global registry once the translator is installed
        GcWindowRegistry::initialize();

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

        } else {

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
        if (appsettings->value(NULL, GC_START_HTTP, true).toBool() || server) {

            // notifications etc
            if (nogui) {
                qDebug()<<"Starting GoldenCheetah API web-services... (hit ^C to close)";
                qDebug()<<"Athlete directory:"<<home.absolutePath();
            } else {
                // switch off warnings if in gui mode
#ifndef GC_WANT_ALLDEBUG
#if QT_VERSION > 0x50000
                qInstallMessageHandler(myMessageOutput);
#else
                qInstallMsgHandler(myMessageOutput);
#endif
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
                exit(0);
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
                        home.cdUp();
                        anyOpened = true;
                    } else {
                        delete trainDB;
                        return ret;
                    }
                }
            }
        }

        // ack, didn't manage to open an athlete
        // and the upgradeWarning was
        // lets ask the user which / create a new one
        if (!anyOpened) {
            ChooseCyclistDialog d(home, true);
            d.setModal(true);

            // choose cancel?
            if ((ret=d.exec()) != QDialog::Accepted) {
                delete trainDB;
                return ret;
            }

            // chosen, so lets get the choice..
            QString homeDir = home.canonicalPath();
            home.cd(d.choice());
            if (!home.exists()) {
                delete trainDB;
                exit(0);
            }

            appsettings->initializeQSettingsAthlete(homeDir, d.choice());
            // .. and open a mainwindow
            GcUpgrade v3;
            if (v3.upgradeConfirmedByUser(home)) {
                MainWindow *mainWindow = new MainWindow(home);
                mainWindow->show();
                mainWindow->ridesAutoImport();
            } else {
                delete trainDB;
                return ret;
            }
        }

        ret=application->exec();

        // close trainDB
        delete trainDB;

        // reset QSettings (global & Athlete)
        appsettings->clearGlobalAndAthletes();

        // clear web caches (stop warning of WebKit leaks)
        QWebSettings::clearMemoryCaches();

    } while (restarting);

    return ret;
}
