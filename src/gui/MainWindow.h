/* 
 * $Id: MainWindow.h,v 1.6 2006/07/11 21:20:21 srhea Exp $
 *
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

#ifndef _GC_MainWindow_h
#define _GC_MainWindow_h 1

#include <QDir>
#include <QtGui>

class AllPlot;
class CpintPlot;

class MainWindow : public QMainWindow 
{
    Q_OBJECT

    public:
        MainWindow(const QDir &home);
        void addRide(QString name);

    protected:
        virtual void resizeEvent(QResizeEvent*);
        virtual void moveEvent(QMoveEvent*);
    
    private slots:
        void rideSelected();
        void splitterMoved();
        void newCyclist();
        void openCyclist();
        void downloadRide();
        void setSmoothingFromSlider();
        void setSmoothingFromLineEdit();
        void tabChanged(int index);

    private:

        QDir home;
        QSettings settings;

        QSplitter *splitter;
        QTreeWidget *treeWidget;
        QTabWidget *tabWidget;
        QTextEdit *rideSummary;
        AllPlot *allPlot;
        CpintPlot *cpintPlot;
        QSlider *smoothSlider;
        QLineEdit *smoothLineEdit;
        QTreeWidgetItem *allRides;
};

#endif // _GC_MainWindow_h

