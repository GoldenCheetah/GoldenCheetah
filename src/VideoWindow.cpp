/*
* Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#include "VideoWindow.h"
#include "MainWindow.h"
#include "RealtimeData.h"
#include "RaceDispatcher.h"
#include "math.h" // for round()
#include "Units.h" // for MILES_PER_KM

// Two current realtime device types supported are:
#include "ComputrainerController.h"
#include "ANTplusController.h"

#include "TrainTool.h"

#include <phonon/phonon>

VideoWindow::VideoWindow(MainWindow *parent, TrainTool *trainTool, const QDir &home)  :
QGraphicsView(parent), home(home), main(parent), trainTool(trainTool)
{
    setInstanceName("Video Window");

    setRenderHint(QPainter::Antialiasing, false);
    setRenderHint(QPainter::TextAntialiasing, false);
    setRenderHint(QPainter::SmoothPixmapTransform, false);

    QWidget *w = new QWidget;
    setViewport(new QGLWidget);
    QGraphicsProxyWidget* proxy = new QGraphicsProxyWidget();

    // black background for video
    setBackgroundBrush(QBrush(Qt::black));

    //setup media object
    Phonon::MediaObject *media = new Phonon::MediaObject(proxy);

    //setup audio
    Phonon::AudioOutput *audioOutput = new Phonon::AudioOutput(proxy);
    Phonon::createPath(media, audioOutput);

    // Setup video output in 16:9 (4:3 is so 80s)
    Phonon::VideoWidget *videoWidget = new Phonon::VideoWidget(w);
    videoWidget->setFixedSize(QSize(960,540));
    Phonon::createPath(media, videoWidget);

    //connect(videoPlayer, SIGNAL(finished()), videoPlayer, SLOT(deleteLater()));
    proxy->setWidget(w);
    scene = new QGraphicsScene;
    scene->addItem(proxy);
    setScene(scene);

    // Open the vid and play!
    media->setCurrentSource(Phonon::MediaSource("/home/markl/Desktop/My Documents/fightclub.mp4"));
    videoWidget->show();
    //media->play();

    // what we got?
    foreach (QString key, media->metaData().values()) qDebug()<<key<<media->metaData(key);
}

void VideoWindow::resizeEvent(QResizeEvent * )
{
    fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}
