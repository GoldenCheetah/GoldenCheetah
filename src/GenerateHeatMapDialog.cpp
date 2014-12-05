
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

#include "GenerateHeatMapDialog.h"
#include "MainWindow.h"
#include "Context.h"
#include "Athlete.h"
#include "RideCache.h"

GenerateHeatMapDialog::GenerateHeatMapDialog(Context *context) : QDialog(context->mainWindow), context(context)
{
    setAttribute(Qt::WA_DeleteOnClose);
    //setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint); // must stop using this flag!
    setWindowTitle(tr("Ride Heat Map Generator"));

    // make the dialog a resonable size
    setMinimumWidth(550);
    setMinimumHeight(400);

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    files = new QTreeWidget;
    files->headerItem()->setText(0, tr(""));
    files->headerItem()->setText(1, tr("Filename"));
    files->headerItem()->setText(2, tr("Date"));
    files->headerItem()->setText(3, tr("Time"));
    files->headerItem()->setText(4, tr("Action"));

    files->setColumnCount(5);
    files->setColumnWidth(0, 30); // selector
    files->setColumnWidth(1, 190); // filename
    files->setColumnWidth(2, 95); // date
    files->setColumnWidth(3, 90); // time
    files->setSelectionMode(QAbstractItemView::SingleSelection);
    files->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    files->setUniformRowHeights(true);
    files->setIndentation(0);

    // populate with each ride in the ridelist
    foreach (RideItem *rideItem, context->athlete->rideCache->rides()) {

        QTreeWidgetItem *add = new QTreeWidgetItem(files->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        // selector
        QCheckBox *checkBox = new QCheckBox("", this);
        checkBox->setChecked(true);
        files->setItemWidget(add, 0, checkBox);

        // we will wipe the original file
        add->setText(1, rideItem->fileName);
        add->setText(2, rideItem->dateTime.toString(tr("dd MMM yyyy")));
        add->setText(3, rideItem->dateTime.toString("hh:mm:ss"));

        // interval action
        add->setText(4, tr("Export"));
    }

    // directory
    QGridLayout *grid = new QGridLayout;

    selectDir = new QPushButton(tr("Browse"), this);
    dirLabel = new QLabel (tr("Export to"), this);

    // default to last used
    QString dirDefault = appsettings->value(this, GC_BE_LASTDIR, QDir::home().absolutePath()).toString();
    if (!QDir(dirDefault).exists()) dirDefault = QDir::home().absolutePath();

    dirName = new QLabel(dirDefault, this);
    all = new QCheckBox(tr("check/uncheck all"), this);
    all->setChecked(true);

    grid->addWidget(dirLabel, 1,0, Qt::AlignLeft);
    grid->addWidget(dirName, 1,1, Qt::AlignLeft);
    grid->addWidget(selectDir, 1,2, Qt::AlignLeft);
    grid->addWidget(all, 2,0, Qt::AlignLeft);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 10);

    // buttons
    QHBoxLayout *buttons = new QHBoxLayout;
    status = new QLabel("", this);
    status->hide();
    cancel = new QPushButton(tr("Cancel"), this);
    ok = new QPushButton(tr("Generate Heat Map"), this);
    buttons->addWidget(status);
    buttons->addStretch();
    buttons->addWidget(cancel);
    buttons->addWidget(ok);

    layout->addLayout(grid);
    layout->addWidget(files);
    layout->addLayout(buttons);

    exports = fails = 0;

    // connect signals and slots up..
    connect(selectDir, SIGNAL(clicked()), this, SLOT(selectClicked()));
    connect(ok, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(all, SIGNAL(stateChanged(int)), this, SLOT(allClicked()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
}

void
GenerateHeatMapDialog::selectClicked()
{
    QString before = dirName->text();
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Target Directory"),
                                                 before,
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);
    if (dir!="") {
        dirName->setText(dir);
    }
    return;
}

void
GenerateHeatMapDialog::allClicked()
{
    // set/uncheck all rides according to the "all"
    bool checked = all->isChecked();

    for(int i=0; i<files->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *current = files->invisibleRootItem()->child(i);
        static_cast<QCheckBox*>(files->itemWidget(current,0))->setChecked(checked);
    }
}

void
GenerateHeatMapDialog::okClicked()
{
    if (ok->text() == "Generate Heat Map" || ok->text() == tr("Generate Heat Map")) {
        aborted = false;

        status->setText(tr("Generating Heat Map..."));
        status->show();
        cancel->hide();
        ok->setText(tr("Abort"));
        appsettings->setValue(GC_BE_LASTDIR, dirName->text());
        generateNow();
        status->setText(QString(tr("%1 rides exported, %2 failed or skipped.")).arg(exports).arg(fails));
        ok->setText(tr("Finish"));

    } else if (ok->text() == "Abort" || ok->text() == tr("Abort")) {
        aborted = true;
    } else if (ok->text() == "Finish" || ok->text() == tr("Finish")) {
        accept(); // our work is done!
    }
}

void
GenerateHeatMapDialog::cancelClicked()
{
    reject();
}

void
GenerateHeatMapDialog::generateNow()
{

    double minLat = 999;
    double maxLat = -999;
    double minLon = 999;
    double maxLon = -999;
    QHash<QString, int> hash;

    // loop through the table and export all selected
    for(int i=0; i<files->invisibleRootItem()->childCount(); i++) {

        // give user a chance to abort..
        QApplication::processEvents();

        // did they?
        if (aborted == true) return; // user aborted!

        QTreeWidgetItem *current = files->invisibleRootItem()->child(i);

        // is it selected
        if (static_cast<QCheckBox*>(files->itemWidget(current,0))->isChecked()) {

            files->setCurrentItem(current); QApplication::processEvents();

            // this one then
            current->setText(4, tr("Reading...")); QApplication::processEvents();

            // open it..
            QStringList errors;
            QList<RideFile*> rides;
            QFile thisfile(QString(context->athlete->home->activities().absolutePath()+"/"+current->text(1)));
            RideFile *ride = RideFileFactory::instance().openRideFile(context, thisfile, errors, &rides);

            // open success?
            if (ride) {
                current->setText(4, tr("Writing...")); QApplication::processEvents();

                if (ride->areDataPresent()->lat == true && ride->areDataPresent()->lon == true) {
                    int lastDistance = 0;
                    foreach(const RideFilePoint *point, ride->dataPoints()) {

                        if (lastDistance < (int) (point->km * 1000) &&
                           (point->lon!=0 || point->lat!=0)) {

                            // Pick up a point max every 15m
                            lastDistance = (int) (point->km * 1000) + 15;
                            //outhtml << "  new google.maps.LatLng("<<point->lat<<", "<<point->lon<<"),\n";
                            QString lonlat = QString("%1,%2").arg(floorf(point->lat*100000)/100000).arg(floorf(point->lon*100000)/100000);
                            if (hash.contains(lonlat)) {
                                hash[lonlat] = hash[lonlat] + 1;
                            } else {
                                hash[lonlat] = 1;
                            }

                            if (minLon > point->lon) minLon = point->lon;
                            if (minLat > point->lat) minLat = point->lat;
                            if (maxLon < point->lon) maxLon = point->lon;
                            if (maxLat < point->lat) maxLat = point->lat;

                        }

                    }
                }

                delete ride; // free memory!
            // open failed
            } else {
                current->setText(4, tr("Read error")); QApplication::processEvents();
            }
        }
    }

    QHashIterator<QString, int> i(hash);
    QString datapoints = "";
    while (i.hasNext()) {
         i.next();
         datapoints += QString("[%1,%2],")
                    .arg(i.key())
                    .arg(i.value());
    }
    QFile filehtml(dirName->text() + "/HeatMap.htm");
    filehtml.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream outhtml(&filehtml);
    outhtml << "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Heatmaps</title>\n";
    outhtml << "<style>\n";
    outhtml << "html,body,#map-canvas {height: 100%;margin: 0px;padding: 0px;}</style>\n";
    outhtml << "<script src=\"https://maps.googleapis.com/maps/api/js?v=3.exp&libraries=visualization\"></script>\n";
    outhtml << "<script>\n";
    outhtml << "var map,pointarray,heatmap;\n";
    outhtml << "var dataarray = [\n";
    outhtml << datapoints;
    outhtml << "];\n";
    outhtml << "var hmData = [];\n";
    outhtml << "function initialize() {\n";
    outhtml << "dataarray.forEach(function(entry) {hmData.push({location: new google.maps.LatLng(entry[0], entry[1]), weight: entry[2]});});\n";
    outhtml << "dataarray = [];\n";
    outhtml << "var mapOptions = { mapTypeId: google.maps.MapTypeId.SATELLITE};\n";
    outhtml << "map = new google.maps.Map(document.getElementById('map-canvas'),mapOptions);\n";
    outhtml << "var bounds = new google.maps.LatLngBounds();\n";
    outhtml << "bounds.extend(new google.maps.LatLng(" << minLat <<"," << minLon << "));\n";
    outhtml << "bounds.extend(new google.maps.LatLng(" << maxLat <<"," << maxLon << "));\n";
    outhtml << "map.fitBounds(bounds);\n";
    outhtml << "var pointArray = new google.maps.MVCArray(hmData);\n";
    outhtml << "heatmap = new google.maps.visualization.HeatmapLayer({data: pointArray, dissipating:true, maxIntensity:30, opacity:0.8});\n";
    outhtml << "google.maps.event.addListener(map,'zoom_changed',function() {\n";
    outhtml << "var zoomLevel = map.getZoom();\n";
    outhtml << "heatmap.set('radius',Math.ceil((Math.pow(zoomLevel,3))/200));\n";
    outhtml << "});\n";
    outhtml << "heatmap.setMap(map);\n";
    outhtml << "}\n";
    outhtml << "google.maps.event.addDomListener(window,'load',initialize);\n";
    outhtml << "</script></head><body><div id=\"map-canvas\"></div></body></html>\n";
    filehtml.close();
}
