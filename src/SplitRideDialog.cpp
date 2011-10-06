/*
 * Copyright (c) 2009 Ned Harding (ned@hardinggroup.com)
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

#include "SplitRideDialog.h"
#include "GcRideFile.h"
#include "MainWindow.h"
#include "RideFile.h"
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <QtGui>
#include <boost/scoped_ptr.hpp>

namespace
{
    QString MakeTimeString(double secs)
    {
        QString strTime;
        double mins = ((int) secs) / 60;
        secs = secs - mins * 60.0;
        double hrs = ((int) mins) / 60;
        mins = mins - hrs * 60.0;
        strTime.sprintf("%02.0f:%02.0f:%02.0f", hrs, mins, secs);
        return strTime;
    }

    class ListItem_Split : public QListWidgetItem
    {
        public:
        int m_nRecNum;
        explicit ListItem_Split(int nRecNum, const QString &text, QListWidget *view)
                : QListWidgetItem(text, view)
                , m_nRecNum(nRecNum)
        {
        }
    };
}

SplitRideDialog::SplitRideDialog(MainWindow *mainWindow)
    : mainWindow(mainWindow)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("Split Ride");

    listWidget = new QListWidget(this);
    listWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    QLabel *splitLabel = new QLabel(tr("Split at the following positions:"), this);
    QLabel *labelSpacer = new QLabel(this);
    labelSpacer->setIndent(10);

    okButton = new QPushButton(tr("&OK"), this);
    cancelButton = new QPushButton(tr("&Cancel"), this);

    connect(okButton, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(listWidget,
            SIGNAL(itemSelectionChanged()), this, SLOT(selectionChanged()));

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(splitLabel);
    mainLayout->addWidget(listWidget);
    mainLayout->addWidget(labelSpacer);
    mainLayout->addLayout(buttonLayout);

    const RideFile *ride = mainWindow->currentRide();
    QMap<int,QString> splitPoints;

    QList<RideFileInterval> intervals = ride->intervals();
    if (!intervals.empty()) {
        intervals.removeFirst();
        foreach (RideFileInterval interval, intervals) {
            int nDataPoint = ride->intervalBegin(interval);
            QString strDesc = QString("Interval %1").arg(interval.name);
            splitPoints.insert(nDataPoint, strDesc);
        }
    }

    for (int nDataPoint = 1; nDataPoint < ride->dataPoints().size(); ++nDataPoint) {
        const RideFilePoint *point = ride->dataPoints()[nDataPoint];
        double dLastSeconds = ride->dataPoints()[nDataPoint - 1]->secs;
        if ((point->secs-dLastSeconds)>=30 && !splitPoints.contains(nDataPoint)) {
            QString strDesc = "Time Gap: " +  MakeTimeString(point->secs-dLastSeconds);
            splitPoints.insert(nDataPoint, strDesc);
        }
    }

    QMapIterator<int,QString> i(splitPoints);
    while (i.hasNext()) {
        i.next();
        int nDataPoint = i.key();
        const RideFilePoint *point = ride->dataPoints()[nDataPoint];
        QString strDesc = MakeTimeString(point->secs) + " - " + i.value();
        ListItem_Split *pItem = new ListItem_Split(nDataPoint, strDesc, listWidget);

        if (nDataPoint > 0) {
            double dLastSeconds = ride->dataPoints()[nDataPoint - 1]->secs;
            if ((point->secs-dLastSeconds)>20*60)
                pItem->setCheckState(Qt::Checked);
            else
                pItem->setCheckState(Qt::Unchecked);
        }
    }
}


void
SplitRideDialog::cancelClicked()
{
    reject();
}

void
SplitRideDialog::CreateNewRideFile(const RideFile *ride, int nRecStart, int nRecEnd)
{
    // set the start time, taking care to include the offset for the interval
    QDateTime newStart =  ride->startTime();
    RideFilePoint *pointStart = ride->dataPoints().at(nRecStart);
    newStart = newStart.addMSecs(static_cast<qint64>(pointStart->secs*1000 + 0.5));

    QString fileName;
    QString filePath;

    // adjust start time of file to avoid conflicts with existing files
    int offset = 0;
    QDateTime noConflicts;
    do {
        noConflicts = newStart.addSecs(offset);

        fileName.sprintf("%04d_%02d_%02d_%02d_%02d_%02d.gc",
                            noConflicts.date().year(), noConflicts.date().month(),
                            noConflicts.date().day(), noConflicts.time().hour(), noConflicts.time().minute(),
                            noConflicts.time().second());
        filePath = mainWindow->home.absolutePath() + "/" + fileName;

        offset++;

    } while (QFile(filePath).exists());
    newStart = newStart.addSecs(offset);

    // create the ridefile in memory
    boost::scoped_ptr<RideFile> newRideFile(new RideFile(newStart, ride->recIntSecs()));
    for (int nItem = nRecStart; nItem<nRecEnd; ++nItem)
    {
        RideFilePoint *pPoint = ride->dataPoints().at(nItem);
        newRideFile->appendPoint(pPoint->secs-pointStart->secs, pPoint->cad, pPoint->hr, pPoint->km - pointStart->km,
                 pPoint->kph, pPoint->nm, pPoint->watts, pPoint->alt, pPoint->lon, pPoint->lat, pPoint->headwind, pPoint->interval-pointStart->interval);
    }
    newRideFile->setDeviceType(ride->deviceType());

    double endSecs = ride->dataPoints().at(nRecEnd - 1)->secs + ride->recIntSecs();
    foreach (RideFileInterval interval, ride->intervals()) {
        if ((interval.start >= pointStart->secs) && (interval.stop <= endSecs)) {
            newRideFile->addInterval(interval.start - pointStart->secs,
                                     interval.stop - pointStart->secs,
                                     interval.name);
        }
    }

    // should never, ever complain.. could wipe out, but lets check anyway
    // it costs nothing and might save a file one day
    QFile file(filePath);
    if (file.exists())
    {
        QMessageBox::critical(this, tr("Split Ride"), tr("The file %1 already exists and will not be overwritten").arg(filePath));
        return;
    }

    // write to disk
    GcFileReader f;
    f.writeRideFile(newRideFile.get(), file);

    // add to the ride list
    mainWindow->addRide(fileName, false);
}
void
SplitRideDialog::okClicked()
{
    const RideFile *ride = mainWindow->currentRide();

    int nStartPoint = 0;
    for (int nRow=0; nRow<listWidget->count(); ++nRow)
    {
        const ListItem_Split *pItem = static_cast<const ListItem_Split *>(listWidget->item(nRow));
        if (pItem->checkState()==Qt::Checked)
        {
            CreateNewRideFile(ride, nStartPoint, pItem->m_nRecNum);
            nStartPoint = pItem->m_nRecNum;
        }
    }

    if (nStartPoint!=0)
    {
        CreateNewRideFile(ride, nStartPoint, mainWindow->currentRide()->dataPoints().count());
        mainWindow->removeCurrentRide();
    }

    done(0);
}

void
SplitRideDialog::selectionChanged()
{
}
