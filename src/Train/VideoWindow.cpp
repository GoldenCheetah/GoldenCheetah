/*
* Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
*               2015 Vianney Boyer   (vlcvboyer@gmail.com)
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

#include <QGraphicsPathItem>
#include "VideoWindow.h"
#include "Context.h"
#include "RideItem.h"
#include "RideFile.h"
#include "MeterWidget.h"
#include "VideoLayoutParser.h"


VideoWindow::VideoWindow(Context *context)  :
    GcChartWindow(context), context(context), m_MediaChanged(false)
{
    setControls(NULL);
    setProperty("color", QColor(Qt::black));

    QHBoxLayout *layout = new QHBoxLayout();
    setChartLayout(layout);

    curPosition = 1;

    init = true; // assume initialisation was ok ...

#ifdef GC_VIDEO_VLC
    //
    // USE VLC VIDEOPLAYER
    //
#ifndef Q_CC_MSVC
#if QT_VERSION >= 0x050000
// we no longer warn here since it is a runtime issue, on some Ubuntu platforms
// the VLC plugin cache is out of date and needs refreshing with the command:
// $ sudo /usr/lib/vlc/vlc-cache-gen -f /usr/lib/vlc/plugins/
// #warning "WARNING: Please ensure the VLC QT4 plugin (gui/libqt4_plugin) is NOT available as it will cause GC to crash."
#endif
#endif

    // config parameters to libvlc
    const char * const vlc_args[] = {
                    "-I", "dummy", /* Don't use any interface */
                    "--ignore-config", /* Don't use VLC's config */
                    "--disable-screensaver", /* disable screensaver during playback */
#ifdef Q_OS_LINUX
                    "--no-xlib", // avoid xlib thread error messages
#endif
                    //"--verbose=-1", // -1 = no output at all
                    //"--quiet"
                };

    /* create an exception handler */
    //libvlc_exception_init(&exceptions);
    //vlc_exceptions(&exceptions);

    /* Load the VLC engine */
    inst = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);
    //vlc_exceptions(&exceptions);

    /* Create a new item */

    if (inst) { // if vlc doesn't initialise don't even try!

        m = NULL;
        //vlc_exceptions(&exceptions);

        /* Create a media player playing environement */
        mp = libvlc_media_player_new (inst);
        //vlc_exceptions(&exceptions);

        //vlc_exceptions(&exceptions);


    /* This is a non working code that show how to hooks into a window,
     * if we have a window around */
#ifdef Q_OS_LINUX
#if QT_VERSION > 0x50000
        x11Container = new QWidget(this); //XXX PORT TO 5.1 BROKEN CODE XXX
#else
        x11Container = new QX11EmbedContainer(this);
#endif
        layout->addWidget(x11Container);
        libvlc_media_player_set_xwindow (mp, x11Container->winId());
#endif

#ifdef WIN32
        container = new QWidget(this);
        layout->addWidget(container);
        libvlc_media_player_set_hwnd (mp, (HWND)(container->winId()));

        QString filename = context->athlete->home->config().canonicalPath() + "/" + "video-layout.xml";
        QFileInfo finfo(filename);

        if (finfo.exists())
        {
            QFile file(filename);

            // clean previous layout
            foreach(MeterWidget* p_meterWidget, m_metersWidget)
            {
                m_metersWidget.removeAll(p_meterWidget);
                delete p_meterWidget;
            }

            VideoLayoutParser handler(&m_metersWidget, container);

            QXmlInputSource source (&file);
            QXmlSimpleReader reader;
            reader.setContentHandler (&handler);

            reader.parse (source);
        }
        else
        {
            qDebug() << qPrintable(QString("file" + filename + " (video layout XML file) not found"));
        }
#endif
    } else {

        // something went wrong !
        init = false;
    }
#endif

#ifdef GC_VIDEO_QT5
    // USE QT VIDEO PLAYER
    wd = new QVideoWidget(this);
    wd->show();

    mp = new QMediaPlayer(this);
    mp->setVideoOutput(wd);

    layout->addWidget(wd);
#endif

    if (init) {
        // get updates..
        connect(context, SIGNAL(telemetryUpdate(RealtimeData)), this, SLOT(telemetryUpdate(RealtimeData)));
        connect(context, SIGNAL(stop()), this, SLOT(stopPlayback()));
        connect(context, SIGNAL(start()), this, SLOT(startPlayback()));
        connect(context, SIGNAL(pause()), this, SLOT(pausePlayback()));
        connect(context, SIGNAL(seek(long)), this, SLOT(seekPlayback(long)));
        connect(context, SIGNAL(unpause()), this, SLOT(resumePlayback()));
        connect(context, SIGNAL(mediaSelected(QString)), this, SLOT(mediaSelected(QString)));
    }
}

VideoWindow::~VideoWindow()
{
    if (!init) return; // we didn't initialise properly so all bets are off

#if (defined Q_OS_LINUX) && (QT_VERSION < 0x050000) && (defined GC_VIDEO_VLC)
    // unembed vlc backend first
    x11Container->discardClient();
#endif

    stopPlayback();

#ifdef GC_VIDEO_VLC
    // VLC

    /* No need to keep the media now */
    if (m) libvlc_media_release (m);

    /* nor the player */
    libvlc_media_player_release (mp);

    // unload vlc
    libvlc_release (inst);
#endif

#ifdef GC_VIDEO_QT5
    // QT MEDIA
    delete mp;
    delete wd;
#endif
}

void VideoWindow::resizeEvent(QResizeEvent * )
{
    foreach(MeterWidget* p_meterWidget , m_metersWidget)
        p_meterWidget->AdjustSizePos();
}

void VideoWindow::startPlayback()
{
    if (context->currentVideoSyncFile())
        context->currentVideoSyncFile()->manualOffset = 0.0;

#ifdef GC_VIDEO_VLC
    if (!m) return; // ignore if no media selected

    // stop playback & wipe player
    libvlc_media_player_stop (mp);

    /* set the media to playback */
    libvlc_media_player_set_media (mp, m);

    /* set the playback rate to the media default - since there may be a different one set from RLV */
    libvlc_media_player_set_rate(mp, libvlc_media_player_get_fps(mp));

    /* play the media_player */
    libvlc_media_player_play (mp);

    m_MediaChanged = false;
#endif

#ifdef GC_VIDEO_QT5
    // open the media object
    mp->play();
#endif

    foreach(MeterWidget* p_meterWidget , m_metersWidget)
    {
        p_meterWidget->setWindowOpacity(1); // Show the widget
        p_meterWidget->AdjustSizePos();
        p_meterWidget->update();

        p_meterWidget->raise();
        p_meterWidget->show();
    }
}

void VideoWindow::stopPlayback()
{
    if (context->currentVideoSyncFile())
        context->currentVideoSyncFile()->manualOffset = 0.0;

#ifdef GC_VIDEO_VLC
    if (!m) return; // ignore if no media selected

    // stop playback & wipe player
    libvlc_media_player_stop (mp);
#endif

#ifdef GC_VIDEO_QT5
    mp->stop();
#endif
    foreach(MeterWidget* p_meterWidget , m_metersWidget)
        p_meterWidget->hide();

}

void VideoWindow::pausePlayback()
{
#ifdef GC_VIDEO_VLC
    if (!m) return; // ignore if no media selected

    // stop playback & wipe player
    libvlc_media_player_set_pause(mp, true);
#endif

#ifdef GC_VIDEO_QT5
    mp->pause();
#endif
}

void VideoWindow::resumePlayback()
{
#ifdef GC_VIDEO_VLC
    if (!m) return; // ignore if no media selected

    // stop playback & wipe player
    if(m_MediaChanged)
        startPlayback();
    else
        libvlc_media_player_set_pause(mp, false);
#endif

#ifdef GC_VIDEO_QT5
    mp->play();
#endif
}

void VideoWindow::telemetryUpdate(RealtimeData rtd)
{
    foreach(MeterWidget* p_meterWidget , m_metersWidget)
    {
        if (p_meterWidget->Source() == QString("None"))
        {
            //Nothing
        }
        else if (p_meterWidget->Source() == QString("Speed"))
        {
            p_meterWidget->Value = rtd.getSpeed();
            p_meterWidget->Text = QString::number((int) rtd.getSpeed());
            p_meterWidget->AltText = QString(".") +QString::number((int)(p_meterWidget->Value * 10.0) - (((int) p_meterWidget->Value) * 10)) + tr(" kph");
        }
        else if (p_meterWidget->Source() == QString("Cadence"))
        {
            p_meterWidget->Value = rtd.getCadence();
            p_meterWidget->Text = QString::number((int)p_meterWidget->Value);
        }
        else if (p_meterWidget->Source() == QString("Watt"))
        {
            p_meterWidget->Value =  rtd.getWatts();
            p_meterWidget->Text = QString::number((int)p_meterWidget->Value);
        }
        else if (p_meterWidget->Source() == QString("HRM"))
        {
            p_meterWidget->Value =  rtd.getHr();
            p_meterWidget->Text = QString::number((int)p_meterWidget->Value) + tr(" bpm");
        }
        else if (p_meterWidget->Source() == QString("TrainerStatus"))
        {
            if (!rtd.getTrainerStatusAvailable())
            {  // we don't have status from trainer thus we cannot indicate anything on screen
                p_meterWidget->Text = tr("");
            }
            else if (rtd.getTrainerCalibRequired())
            {
                p_meterWidget->setColor(QColor(255,0,0,180));
                p_meterWidget->Text = tr("Calibration required");
            }
            else if (rtd.getTrainerConfigRequired())
            {
                p_meterWidget->setColor(QColor(255,0,0,180));
                p_meterWidget->Text = tr("Configuration required");
            }
            else if (rtd.getTrainerBrakeFault())
            {
                p_meterWidget->setColor(QColor(255,0,0,180));
                p_meterWidget->Text = tr("brake fault");
            }
            else if (rtd.getTrainerReady())
            {
                p_meterWidget->setColor(QColor(0,255,0,180));
                p_meterWidget->Text = tr("Ready");
            }
            else
            {
                p_meterWidget->Text = tr("");
            }
        }
    }

    foreach(MeterWidget* p_meterWidget , m_metersWidget)
        p_meterWidget->update();

#ifdef GC_VIDEO_NONE
    Q_UNUSED(rtd)
#endif

#ifdef GC_VIDEO_VLC
    if (!m) return;

    QList<ErgFilePoint> *ErgFilePoints = NULL;
    if (context->currentErgFile()) {
        if (context->currentErgFile()->format == CRS || context->currentErgFile()->format == CRS_LOC) {
            ErgFilePoints = &(context->currentErgFile()->Points);
        }
    }

    // find the curPosition
    if (context->currentVideoSyncFile())
    {
        // when we selected a videosync file in training mode (rlv...):

        QVector<VideoSyncFilePoint> VideoSyncFiledataPoints = context->currentVideoSyncFile()->Points;
        if (VideoSyncFiledataPoints.count()<2) return;

        if (curPosition > VideoSyncFiledataPoints.count() - 1 || curPosition < 1)
            curPosition = 1; // minimum curPosition is 1 as we will use [curPosition-1]

        double CurrentDistance = qBound(0.0,  rtd.getDistance() + context->currentVideoSyncFile()->manualOffset, context->currentVideoSyncFile()->Distance);
        context->currentVideoSyncFile()->km = CurrentDistance;

        // make sure the current position is less than the new distance
        while ((VideoSyncFiledataPoints[curPosition].km > CurrentDistance) && (curPosition > 1))
            curPosition--;
        while ((VideoSyncFiledataPoints[curPosition].km <= CurrentDistance) && (curPosition < VideoSyncFiledataPoints.count()-1))
            curPosition++;

        // update the rfp
        float weighted_average = (VideoSyncFiledataPoints[curPosition].km - VideoSyncFiledataPoints[curPosition-1].km != 0.0)?(CurrentDistance-VideoSyncFiledataPoints[curPosition-1].km) / (VideoSyncFiledataPoints[curPosition].km - VideoSyncFiledataPoints[curPosition-1].km):0.0;
        rfp.km = CurrentDistance;
        rfp.secs = VideoSyncFiledataPoints[curPosition-1].secs + weighted_average * (VideoSyncFiledataPoints[curPosition].secs - VideoSyncFiledataPoints[curPosition-1].secs);
        rfp.kph = VideoSyncFiledataPoints[curPosition-1].kph + weighted_average * (VideoSyncFiledataPoints[curPosition].kph - VideoSyncFiledataPoints[curPosition-1].kph);

        /*
        //TODO : GPX file format
            // otherwise we use the gpx from selected ride in analysis view:
            QVector<RideFilePoint*> dataPoints =  myRideItem->ride()->dataPoints();
            if (dataPoints.count()<2) return;

            if(curPosition > dataPoints.count()-1 || curPosition < 1)
                curPosition = 1; // minimum curPosition is 1 as we will use [curPosition-1]

            // make sure the current position is less than the new distance
            while ((dataPoints[curPosition]->km > rtd.getDistance()) && (curPosition > 1))
                curPosition--;
            while ((dataPoints[curPosition]->km <= rtd.getDistance()) && (curPosition < dataPoints.count()-1))
                curPosition++;

            // update the rfp
            rfp = *dataPoints[curPosition];

        }
        */
        // set video rate ( theoretical : video rate = training speed / ghost speed)
        float rate;
        float video_time_shift_ms;
        video_time_shift_ms = (rfp.secs*1000.0 - (double) libvlc_media_player_get_time(mp));
        if (rfp.kph == 0.0)
            rate = 1.0;
        else
            rate = rtd.getSpeed() / rfp.kph;

        //if video is far (empiric) from ghost:
        if (fabs(video_time_shift_ms) > 5000)
        {
            libvlc_media_player_set_time(mp, (libvlc_time_t) (rfp.secs*1000.0));
        }
        else
        // otherwise add "small" empiric corrective parameter to get video back to ghost position:
            rate *= 1.0 + (video_time_shift_ms / 10000.0);

        libvlc_media_player_set_pause(mp, (rate < 0.01));

        // change video rate but only if there is a significant change
        if ((rate != 0.0) && (fabs((rate - currentVideoRate) / rate) > 0.05))
        {
            libvlc_media_player_set_rate(mp, rate );
            currentVideoRate = rate;
        }
    }
#endif

#ifdef GC_VIDEO_QT5
//TODO
//    // seek to ms position in current file
//    mp->setPosition(ms);
#endif
}

void VideoWindow::seekPlayback(long ms)
{
#ifdef GC_VIDEO_NONE
    Q_UNUSED(ms)
#endif

#ifdef GC_VIDEO_VLC
    if (!m) return;

    // when we selected a videosync file in training mode (rlv...)
    if (context->currentVideoSyncFile())
    {
        context->currentVideoSyncFile()->manualOffset += (double) ms; //we consider +/- 1km
    }
    else
    {
        // seek to ms position in current file
        libvlc_media_player_set_time(mp, (libvlc_time_t) ms);
    }
#endif

#ifdef GC_VIDEO_QT5
    mp->setPosition(ms);
#endif
}

void VideoWindow::mediaSelected(QString filename)
{
#ifdef GC_VIDEO_NONE
    Q_UNUSED(filename);
#endif

#ifdef GC_VIDEO_VLC

    // VLC

    // stop any current playback
    stopPlayback();

    // release whatever is already loaded
    if (m) libvlc_media_release(m);
    m = NULL;

    if (filename.endsWith("/DVD") || (filename != "" && QFile(filename).exists())) {

        // properly encode the filename as URL (with all special characters)
        filename =  QUrl::toPercentEncoding(filename, "/\\", "");
#ifdef Q_OS_LINUX
        QString fileURL = "file://" + filename.replace("\\", "/");
#else
        // A Windows "c:\xyz\abc def.avi" filename should become file:///c:/xyz/abc%20def.avi
        QString fileURL = "file:///" + filename.replace("\\", "/");
#endif
        //qDebug()<<"file url="<<fileURL;
        /* open media */
        m = libvlc_media_new_location(inst, filename.endsWith("/DVD") ? "dvd://" : fileURL.toLocal8Bit());

        /* set the media to playback */
        if (m) libvlc_media_player_set_media (mp, m);

        m_MediaChanged = true;
    }
#endif

#ifdef GC_VIDEO_QT5
    // QT MEDIA
    mc = QMediaContent(QUrl::fromLocalFile(filename));
    mp->setMedia(mc);
#endif
}

MediaHelper::MediaHelper()
{
    // construct a list of supported types
    // Using the basic list from the VLC
    // Wiki here: http://www.videolan.org/vlc/features.html and then looked for
    // the common extensions used from here: http://www.fileinfo.com/filetypes/video
    supported << ".3GP";
    supported << ".ASF";
    supported << ".AVI";
    supported << ".DIVX";
    supported << ".FLV";
    supported << ".M4V";
    supported << ".MKV";
    supported << ".MOV";
    supported << ".MP4";
    supported << ".MPEG";
    supported << ".MPG";
    supported << ".MXF";
    supported << ".VOB";
    supported << ".WMV";

}

MediaHelper::~MediaHelper()
{
}

QStringList
MediaHelper::listMedia(QDir dir)
{
    QStringList returning;

    // go through the sub directories
    QDirIterator directory_walker(dir, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);

    while(directory_walker.hasNext()){
        directory_walker.next();

        // whizz through every file in the directory
        // if it has the right extension then we are happy
        QString name = directory_walker.filePath();
        foreach(QString extension, supported) {
            if (name.endsWith(extension, Qt::CaseInsensitive)) {
                name.remove(dir.absolutePath());
                if(name.startsWith('/') || name.startsWith('\\')) // remove '/' (linux/mac) or '\' (windows?)
                    name.remove(0,1);
                returning << name;
                break;
            }
        }
    }
    return returning;
}

bool
MediaHelper::isMedia(QString name)
{
    foreach (QString extension, supported) {
        if (name.endsWith(extension, Qt::CaseInsensitive))
            return true;
    }
    return false;
}
