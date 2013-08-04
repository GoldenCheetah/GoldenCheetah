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
#include "ChooseCyclistDialog.h"
#include "MainWindow.h"
#include "Settings.h"
#include "TrainDB.h"

#ifdef Q_OS_X11
#include <X11/Xlib.h>
#endif

QApplication *application;

int
main(int argc, char *argv[])
{
#ifdef Q_OS_X11
    XInitThreads();
#endif

    application = new QApplication(argc, argv);

    QFont font;
    font.fromString(appsettings->value(NULL, GC_FONT_DEFAULT, QFont().toString()).toString());
    font.setPointSize(appsettings->value(NULL, GC_FONT_DEFAULT_SIZE, 12).toInt());
    application->setFont(font); // set default font

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
    QString libraryPath=QDesktopServices::storageLocation(QDesktopServices::DataLocation) + "/GoldenCheetah";
#else
    // Q_OS_LINUX et al
    QString libraryPath=".goldencheetah";
#endif

    //First check to see if the Library folder exists where the executable is (for USB sticks)
    QDir home = QDir();
    //if it does, create an ini file for settings and cd into the library
    if(home.exists(localLibraryPath))
    {
         home.cd(localLibraryPath);
    }
    //if it does not exist, let QSettings handle storing settings
    //also cd to the home directory and look for libraries
    else
    {
        home = QDir::home();
        //check if this users previously stored files in the old library path
        //if they did, use the existing library
        if (home.exists(oldLibraryPath))
        {
            home.cd(oldLibraryPath);
        }
        //otherwise use the new library path
        else
        {
            //first create the path if it does not exist
            if (!home.exists(libraryPath))
            {
                if (!home.mkpath(libraryPath))
                {
                    qDebug()<<"Failed to create library path\n";
                    exit(0);
                }
            }
            home.cd(libraryPath);
        }
    }

    // install QT Translator to enable QT Dialogs translation
    QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name(),
             QLibraryInfo::location(QLibraryInfo::TranslationsPath));
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

    QStringList args( application->arguments() );

    QVariant lastOpened;
    if( args.size() > 1 ){
        lastOpened = args.at(1);
    } else {
        lastOpened = appsettings->value(NULL, GC_SETTINGS_LAST);
    }

    double crankLength = appsettings->value(NULL, GC_CRANKLENGTH).toDouble();
    if(crankLength<=0) {
       appsettings->setValue(GC_CRANKLENGTH,172.5);
    }

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
    if (!anyOpened) {
        ChooseCyclistDialog d(home, true);
        d.setModal(true);
        if (d.exec() != QDialog::Accepted)
            return 0;
        home.cd(d.choice());
        if (!home.exists())
            exit(0);
        MainWindow *mainWindow = new MainWindow(home);
        mainWindow->show();
    }
    return application->exec();
}
