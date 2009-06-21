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

#include <assert.h>
#include <QApplication>
#include <QtGui>
#include "ChooseCyclistDialog.h"
#include "MainWindow.h"
#include "Settings.h"

int 
main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QDir home = QDir::home();
    if (!home.exists("Library")) 
        if (!home.mkdir("Library"))
            assert(false);
    home.cd("Library");
    if (!home.exists("GoldenCheetah"))
        if (!home.mkdir("GoldenCheetah"))
            assert(false);
    home.cd("GoldenCheetah");
    QSettings settings(GC_SETTINGS_CO, GC_SETTINGS_APP);
    QVariant lastOpened = settings.value(GC_SETTINGS_LAST);
    QVariant unit = settings.value(GC_UNIT);
    double crankLength = settings.value(GC_CRANKLENGTH).toDouble();
    if(crankLength<=0) {
       settings.setValue(GC_CRANKLENGTH,172.5);
    }

    bool anyOpened = false;
    if (lastOpened != QVariant()) {
        QStringList list = lastOpened.toStringList();
        QStringListIterator i(list);
        while (i.hasNext()) {
            QString cyclist = i.next();
            if (home.cd(cyclist)) {
                MainWindow *main = new MainWindow(home);
                main->show();
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
            assert(false);
        MainWindow *main = new MainWindow(home);
        main->show();
    }
    return app.exec();
}


