
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
    const QTreeWidgetItem *allRides = context->athlete->allRideItems();

    for (int i=0; i<allRides->childCount(); i++) {

        RideItem *rideItem = static_cast<RideItem*>(allRides->child(i));

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
    // setup vector to store the processed lon lat data into global grid
    std::vector<heatmapGridAndPoint> globalGrid;

    // Tile size setup coverage is the amount of lon lat covered by one grid
    // x and y size are the pixel size of the grid squares
    double coverageLon = 0.05;
    double coverageLat = 0.05;
    int xSize = 300;
    int ySize = 300;
    int dm = 10; // Diameter of the heat paint brush - thickness of the line

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
            QFile thisfile(QString(context->athlete->home.absolutePath()+"/"+current->text(1)));
            RideFile *ride = RideFileFactory::instance().openRideFile(context, thisfile, errors, &rides);

            // open success?
            if (ride) {
                current->setText(4, tr("Writing...")); QApplication::processEvents();

                if (ride->areDataPresent()->lat == true && ride->areDataPresent()->lon == true) {
                    int lastDistance = 0;
                    foreach(const RideFilePoint *point, ride->dataPoints()) {

                        if (lastDistance < (int) (point->km * 1000) &&
                            (point->lon!=0 || point->lat!=0)) {

                            heatmapGridAndPoint gridAndPoint;

                            // Store the grid position
                            gridAndPoint.xGrid = point->lon/coverageLon;
                            gridAndPoint.yGrid = point->lat/coverageLat;

                            // Caculate the x axis pixel positions relative to
                            // the grid square either side of zero longitude
                            if (point->lon<0) {
                                gridAndPoint.xPixel = xSize + ((point->lon-((double) gridAndPoint.xGrid*coverageLon)) * (xSize/coverageLon));
                                gridAndPoint.xNegative = true;
                            } else {
                                gridAndPoint.xPixel = ((point->lon-((double) gridAndPoint.xGrid*coverageLon)) * (xSize/coverageLon));
                                gridAndPoint.xNegative = false;
                            }

                            // Caculate the y axis pixel positions relative to
                            // the grid square either side of zero latitude
                            if (point->lat<0) {
                                gridAndPoint.yPixel = ((point->lat-((double) gridAndPoint.yGrid*coverageLat)) * (ySize/coverageLat))*-1;
                                gridAndPoint.yNegative = true;
                            } else {
                                gridAndPoint.yPixel = ySize - ((point->lat-((double) gridAndPoint.yGrid*coverageLat)) * (ySize/coverageLat));
                                gridAndPoint.yNegative = false;
                            }

                            // Push into the vector for images to be process in the next loop
                            globalGrid.push_back(gridAndPoint);

                            // Pick up a point max every 20m
                            lastDistance = (int) (point->km * 1000) + 40;
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

    // Sort the vector so that all poins in each grid square are
    // together so we can just process one image at a time
    std::sort(globalGrid.begin(), globalGrid.end());

    // Set up image prosessing variables
    // map is the actual image being drawn
    QImage *map = new QImage(xSize, ySize, QImage::Format_ARGB32);

    // paint provides the functions for doing the drawing
    QPainter paint(map);

    // Antialiasing on
    paint.setRenderHint(QPainter::HighQualityAntialiasing);

    // Load heatmap colour image for later mapping
    // from gray scale to colours
    // Could add the option to use different colour scales
    // by changing this imgage
    QImage *grad_map = new QImage(":images/heatmapColors.png");

    // pen is the outline - set it off with width = 0
    QPen g_pen(QColor(0, 0, 0, 0));
    g_pen.setWidth(0);
    paint.setPen(g_pen);

    heatmapGridAndPoint lastPoint;
    lastPoint.xGrid = 999;
    lastPoint.yGrid = 999;

    // kml file output stream
    QFile file(dirName->text() + "/HeatMap.kml");
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&file);

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    out << "<kml xmlns=\"http://earth.google.com/kml/2.2\"><Document>";

    // itterate through the vector and save the image to
    // disk when we hit each new grid cooridinate
    for( std::vector<heatmapGridAndPoint>::iterator point = globalGrid.begin() ; point != globalGrid.end() ; ++point ) {

        // Has the grid changed since the last itteration
        if (lastPoint.xGrid!=point->xGrid || lastPoint.yGrid!=point->yGrid
         || lastPoint.xNegative!=point->xNegative || lastPoint.yNegative!=point->yNegative) {

            // not the first loop
            if (lastPoint.xGrid!=999) {
                // Convert to heat map
                for(int i = 0; i < xSize; ++i) {
                    for(int j = 0; j < ySize; ++j) {
                        int b = qGray(map->pixel(i, j));
                        if (b >= 251) {
                            //alpha fade around the edges
                            map->setPixel(i, j, qRgba(0,0,0,(255 - b)*51));
                        } else {
                            // use grayscale value to map to colour in the heatmap
                            map->setPixel(i, j, grad_map->pixel(255 - b, 0));
                        }
                    }
                };

                // Save the image to disk
                map->save(dirName->text() + QString("/map_%1%2_%3%4.png")
                          .arg((lastPoint.xNegative && lastPoint.xGrid == 0) ? "-" : "")
                          .arg(lastPoint.xGrid)
                          .arg((lastPoint.yNegative && lastPoint.yGrid == 0) ? "-" : "")
                          .arg(lastPoint.yGrid));

                // add the tile to the kml
                out << kmlOverlayLine(lastPoint, coverageLon, coverageLat);
            }

            // Blank out the image ready to start a new one
            map->fill(QColor(255, 255, 255, 255));
            lastPoint = *point;
        }

        // setup the radial gradient with its center at the lon lat pixel location
        QRadialGradient grad(point->xPixel, point->yPixel, dm/2); // Create Gradient
        grad.setColorAt(0, QColor(0, 0, 0, 40)); // Black, varying alpha
        grad.setColorAt(1, QColor(0, 0, 0, 0)); // Black, completely transparent
        QBrush g_brush(grad); // Gradient QBrush
        paint.setBrush(g_brush);

        // Draw a circle
        paint.drawEllipse(point->xPixel-dm/2, point->yPixel-dm/2, dm, dm); // Draw circle
    }

    // Pick up the last grid square
    if (lastPoint.xGrid!=999) {
        // Convert to heat map
        for(int i = 0; i < xSize; ++i) {
            for(int j = 0; j < ySize; ++j) {
                int b = qGray(map->pixel(i, j));
                if (b >= 251) {
                    map->setPixel(i, j, qRgba(0,0,0,(255 - b)*51));
                } else {
                    map->setPixel(i, j, grad_map->pixel(255 - b, 0)); //grad_map is a QImage gradient map
                }
            }
        };
        map->save(dirName->text() + QString("/map_%1%2_%3%4.png")
                  .arg((lastPoint.xNegative && lastPoint.xGrid == 0) ? "-" : "")
                  .arg(lastPoint.xGrid)
                  .arg((lastPoint.yNegative && lastPoint.yGrid == 0) ? "-" : "")
                  .arg(lastPoint.yGrid));
        out << kmlOverlayLine(lastPoint, coverageLon, coverageLat);
    }

    // finish off the kml and close the file
    out << "</Document></kml>";
    file.close();
}

QString GenerateHeatMapDialog::kmlOverlayLine(heatmapGridAndPoint point, double coverageLon, double coverageLat)
{
    QString kmlContent = "<GroundOverlay>";
    kmlContent += QString("<name>%1%2 %3%4</name>")
                            .arg((point.xNegative && point.xGrid == 0) ? "-" : "")
                            .arg(point.xGrid)
                            .arg((point.yNegative && point.yGrid == 0) ? "-" : "")
                            .arg(point.yGrid);
    kmlContent += "<Icon>";
    kmlContent += QString("<href>map_%1%2_%3%4.png</href>")
                            .arg((point.xNegative && point.xGrid == 0) ? "-" : "")
                            .arg(point.xGrid)
                            .arg((point.yNegative && point.yGrid == 0) ? "-" : "")
                            .arg(point.yGrid);
    kmlContent += "<viewBoundScale>0.75</viewBoundScale>";
    kmlContent += "</Icon>";
    kmlContent += "<LatLonBox>";
    kmlContent += QString("<north>%1</north>").arg((double) point.yGrid*coverageLon+(point.yNegative ? 0 : coverageLon));
    kmlContent += QString("<south>%1</south>").arg((double) point.yGrid*coverageLon-(point.yNegative ? coverageLon : 0));
    kmlContent += QString("<east>%1</east>").arg((double) point.xGrid*coverageLat+(point.xNegative ? 0 : coverageLat));
    kmlContent += QString("<west>%1</west>").arg((double) point.xGrid*coverageLat-(point.xNegative ? coverageLat : 0));
    kmlContent += "</LatLonBox>";
    kmlContent += "</GroundOverlay>";
    return kmlContent;
}
