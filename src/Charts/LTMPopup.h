/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_LTMPopup_h
#define _GC_LTMPopup_h 1
#include "GoldenCheetah.h"

#include "Context.h"
#include "Settings.h"
#include "LTMSettings.h"
#include "RideMetric.h"

#include <QtGui>
#include <QDir>
#include <QStyle>
#include <QStyleFactory>
#include <QtGui>
#include <QDialog>
#include <QTextEdit>
#include <QLabel>
#include <QHeaderView>
#include <QTableWidget>
#include <QTableWidgetItem>

class Specification;
class LTMPopup : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:

        LTMPopup(Context *context);
        void setTitle(QString);

        // when called from LTM chart (time only relevant for LTM_TOD)
        void setData(LTMSettings &settings, QDate start, QDate end, QTime time);

        // when called from a TreeMap chart
        void setData(Specification spec, const RideMetric *metric, QString title);

    signals:

    private slots:
        void rideSelected(); // scrolling up and down the popup ride list
        void rideOpen(); // double click an item on the popup ride list
        virtual void resizeEvent(QResizeEvent *);
        bool eventFilter(QObject *object, QEvent *e);

    private:

        Context *context;
        QLabel *title;
        QTableWidget *rides;
        QTextEdit *metrics;
        QTextEdit *notes;

        // builds HTML text for the selected ride(s)
        QString setSummaryHTML(RideItem*);
        QStringList selected; // filenames
};

// some geometry stuff to make Mac displays nice
// but sensible defaults for Win and Linux
#ifdef Q_OS_MAC
#define SMALLFONT 11
#define LISTWIDTH 180
#else
#define SMALLFONT 8
#define LISTWIDTH 200
#endif

#endif // _GC_LTMPopup_h
