
/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GenerateHeatMapDialog_h
#define _GenerateHeatMapDialog_h
#include "GoldenCheetah.h"
#include "Context.h"
#include "Settings.h"
#include "Units.h"

#include "RideItem.h"
#include "RideFile.h"

#include <QtGui>
#include <QTableWidget>
#include <QProgressBar>
#include <QList>
#include <QFileDialog>
#include <QCheckBox>
#include <QLabel>
#include <QListIterator>
#include <QDebug>

// Dialog class to show filenames, import progress and to capture user input
// of ride date and time

class GenerateHeatMapDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


public:
    GenerateHeatMapDialog(Context *context);

    QTreeWidget *files; // choose files to export

    struct heatmapGridAndPoint {
        int xGrid;
        int yGrid;
        int xPixel;
        int yPixel;
        bool xNegative;
        bool yNegative;

        bool operator<(const heatmapGridAndPoint& a) const
        {
            return ((  xGrid+(  yGrid*100)+(  xNegative ? 1000 : 0)+(  yNegative ? 1000 : 0))
                  < (a.xGrid+(a.yGrid*100)+(a.xNegative ? 1000 : 0)+(a.yNegative ? 1000 : 0)));
        }
    };
signals:

private slots:
    void cancelClicked();
    void okClicked();
    void selectClicked();
    void generateNow();
    void allClicked();
    QString kmlOverlayLine(heatmapGridAndPoint point, double coverageLon, double coverageLat);

private:
    Context *context;
    bool aborted;

    QCheckBox *all;

    QComboBox *format;
    QLabel *formatLabel;

    QPushButton *selectDir;
    QLabel *dirLabel, *dirName;

    QCheckBox *overwrite;
    QPushButton *cancel, *ok;

    int exports, fails;
    QLabel *status;

};
#endif // _GenerateHeatMapDialog_h

