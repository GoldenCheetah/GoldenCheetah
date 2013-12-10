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

#include <QApplication>
#include <QtGui>
#include <QFile>
#include "ChooseCyclistDialog.h"
#include "MainWindow.h"
#include "Settings.h"
#include "TrainDB.h"

#include "GcUpgrade.h"

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
        dup(fp->handle());
    } else {
        fprintf(stderr, "GoldenCheetah: cannot redirect stderr\n");
    }
}
#endif


#ifdef Q_OS_X11
#include <X11/Xlib.h>
#endif

#if QT_VERSION > 0x050000
#include <QStandardPaths>
#endif

QApplication *application;
bool restarting = false;

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

    bool help = false;

    // honour command line switches
    foreach (QString arg, sargs) {

        // help or version requested
        if (arg == "--help" || arg == "--version") {

            help = true;
            fprintf(stderr, "GoldenCheetah %s (%d)\nusage: GoldenCheetah [[directory] athlete]\n\n", VERSION_STRING, VERSION_LATEST);
            fprintf(stderr, "--help or --version to print this message and exit\n");
#ifdef GC_DEBUG
            fprintf(stderr, "--debug             to turn on redirection of messages to goldencheetah.log [debug build]\n");
#else
            fprintf(stderr, "--debug             to direct diagnostic messages to the terminal instead of goldencheetah.log\n");
#endif
            fprintf (stderr, "\nSpecify the folder and/or athlete to open on startup\n");
            fprintf(stderr, "If no parameters are passed it will reopen the last athlete.\n\n");

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

    // set defaultfont
    QFont font;
    font.fromString(appsettings->value(NULL, GC_FONT_DEFAULT, QFont().toString()).toString());
    font.setPointSize(appsettings->value(NULL, GC_FONT_DEFAULT_SIZE, 12).toInt());
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
        QString oldLibraryPath=QDir::home().path()+"/Library/GoldenCheetah";

        //these are the new platform-dependent library paths
#if defined(Q_OS_MACX)
        QString libraryPath="Library/GoldenCheetah";
#elif defined(Q_OS_WIN)
#if QT_VERSION > 0x050000 // windows and qt5
        QStringList paths=QStandardPaths::standardLocations(QStandardPaths::DataLocation);
	    QString libraryPath = paths.at(0) + "/GoldenCheetah";
#else // windows not qt5
        QString libraryPath=QDesktopServices::storageLocation(QDesktopServices::DataLocation) + "/GoldenCheetah";
#endif // qt5
#else // not windows or osx (must be Linux or OpenBSD)
        // Q_OS_LINUX et al
        QString libraryPath=".goldencheetah";
#endif //

        // or did we override in settings?
        QString sh;
        if ((sh=appsettings->value(NULL, GC_HOMEDIR).toString()) != "") localLibraryPath = sh;

        // lets try the local library we've worked out...
        QDir home = QDir();
        if(home.exists(localLibraryPath)) {

            home.cd(localLibraryPath);

        } else {

            // YIKES !! The directory we should be using doesn't exist!
            home = QDir::home();
            if (home.exists(oldLibraryPath)) { // there is an old style path, lets fo there
                home.cd(oldLibraryPath);
            } else {

                if (!home.exists(libraryPath)) {
                    if (!home.mkpath(libraryPath)) {

                    qDebug()<<"Failed to create library path\n";
                    exit(0);

                    }
                }
                home.cd(libraryPath);
            }
        }

        // now redirect stderr
#ifndef WIN32
        if (!debug) nostderr(home.absolutePath());
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
        if(args.count() == 2) { // $ ./GoldenCheetah Mark

            // athlete
            lastOpened = args.at(1);

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
        }

        // lets attempt to open as asked/remembered
        bool anyOpened = false;
        if (lastOpened != QVariant()) {
            QStringList list = lastOpened.toStringList();
            QStringListIterator i(list);
            while (i.hasNext()) {
                QString cyclist = i.next();
                if (home.cd(cyclist)) {
                    MainWindow *mainWindow = new MainWindow(home);
                    mainWindow->show();
                    home.cdUp();
                    anyOpened = true;
                }
            }
        }

        // ack, didn't manage to open an athlete
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
            home.cd(d.choice());
            if (!home.exists()) {
                delete trainDB;
                exit(0);
            }

            // .. and open a mainwindow
            MainWindow *mainWindow = new MainWindow(home);
            mainWindow->show();
        }

        ret=application->exec();

        // close trainDB
        delete trainDB;

    } while (restarting);

    return ret;
}
