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

VideoWindow::VideoWindow(Context *context)  :
    GcWindow(context), context(context), m_MediaChanged(false)
{
    setControls(NULL);
    setProperty("color", QColor(Qt::black));

    QHBoxLayout *layout = new QHBoxLayout();
    setLayout(layout);

    curPosition = 0;

    init = true; // assume initialisation was ok ...

#ifdef GC_VIDEO_VLC
    //
    // USE VLC VIDEOPLAYER
    //
#if QT_VERSION >= 0x050000
#warning "WARNING: Please ensure the VLC QT4 plugin (gui/libqt4_plugin) is NOT available as it will cause GC to crash."
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

        speedmeterwidget = new NeedleMeterWidget(container, 20.0, 15.0, 20.0, 77.0);
        speedmeterwidget->MainColor = QColor(255,0,0,180);
        speedmeterwidget->BackgroundColor = QColor(100,100,100,100);
        speedmeterwidget->RangeMax = 60.0;
        speedmeterwidget->Angle = 220.0;
        speedmeterwidget->SubRange = 6;
        m_metersWidget.append(speedmeterwidget);
        textspeedmeterwidget = new TextMeterWidget(speedmeterwidget, 70.0, 40.0, 50.0, 75.0);
        textspeedmeterwidget->MainColor = QColor(255,0,0,180);
        m_metersWidget.append(textspeedmeterwidget);

        powermeterwidget = new CircularIndicatorMeterWidget(container, 20.0, 15.0, 80.0, 77.0);
        powermeterwidget->Angle = 280.0;
        powermeterwidget->RangeMax = 350.0;
        m_metersWidget.append(powermeterwidget);
        textpowermeterwidget = new TextMeterWidget(powermeterwidget, 50.0, 40.0, 50.0, 50.0);
        textpowermeterwidget->MainColor = QColor(255,100,100,180);
        m_metersWidget.append(textpowermeterwidget);
        TextMeterWidget* unitpowermeterwidget = new TextMeterWidget(powermeterwidget, 50.0, 20.0, 50.0, 85.0);
        unitpowermeterwidget->MainColor = QColor(255,100,100,180);
        unitpowermeterwidget->Text = tr("Watts");
        m_metersWidget.append(unitpowermeterwidget);

        cadencemeterwidget = new CircularIndicatorMeterWidget(container, 20.0, 15.0, 50.0, 77.0);
        m_metersWidget.append(cadencemeterwidget);
        cadencemeterwidget->Angle = 280.0;
        cadencemeterwidget->RangeMax = 350.0;
        m_metersWidget.append(cadencemeterwidget);
        textcadencemeterwidget = new TextMeterWidget(cadencemeterwidget, 50.0, 40.0, 50.0, 50.0);
        textcadencemeterwidget->MainColor = QColor(255,100,100,180);
        m_metersWidget.append(textcadencemeterwidget);
        TextMeterWidget* unitcadencemeterwidget = new TextMeterWidget(cadencemeterwidget, 50.0, 20.0, 50.0, 85.0);
        unitcadencemeterwidget->MainColor = QColor(255,100,100,180);
        unitcadencemeterwidget->Text = tr("rpm");
        m_metersWidget.append(unitcadencemeterwidget);

        textHRMmeterwidget = new TextMeterWidget(container, 15.0, 5.0, 50.0, 90.0);
        textHRMmeterwidget->MainColor = QColor(255,0,0,180);
        m_metersWidget.append(textHRMmeterwidget);
        
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
    {
        p_meterWidget->AdjustSizePos();
    }
}

void VideoWindow::startPlayback()
{
#ifdef GC_VIDEO_VLC
    if (!m) return; // ignore if no media selected

    // stop playback & wipe player
    libvlc_media_player_stop (mp);

    /* set the media to playback */
    libvlc_media_player_set_media (mp, m);

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
    libvlc_media_player_pause (mp);
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
        libvlc_media_player_pause (mp);
#endif

#ifdef GC_VIDEO_QT5
    mp->play();
#endif
}

void VideoWindow::telemetryUpdate(RealtimeData rtd)
{

#ifdef GC_VIDEO_NONE
    Q_UNUSED(rtd)
#endif

#ifdef GC_VIDEO_VLC
    if (!m) return;

    // find the curPosition
    if (context->currentVideoSyncFile())
    {
        // when we selected a videosync file in traning mode (rlv...):

        QVector<VideoSyncFilePoint> VideoSyncFiledataPoints = context->currentVideoSyncFile()->Points;

        if (!VideoSyncFiledataPoints.count()) return;

        if(VideoSyncFiledataPoints.count() < curPosition)
        {
            curPosition = VideoSyncFiledataPoints.count();
        } // make sure the current position is less than the new distance
        else if (VideoSyncFiledataPoints[curPosition].km < rtd.getDistance())
        {
            for( ; curPosition < VideoSyncFiledataPoints.count(); curPosition++)
                if(VideoSyncFiledataPoints[curPosition].km >= rtd.getDistance())
                    break;
        }
        else
        {
            for( ; curPosition > 0; curPosition--)
                if(VideoSyncFiledataPoints[curPosition].km <= rtd.getDistance())
                    break;
        }

        // update the rfp
        rfp.km = VideoSyncFiledataPoints[curPosition].km;
        rfp.secs = VideoSyncFiledataPoints[curPosition].secs;
        rfp.kph = VideoSyncFiledataPoints[curPosition].kph;

        //TODO : weighted average to improve smoothness
    }
    else if (myRideItem->ride())
    {
        // otherwise we use the gpx from selected ride in analysis view:
        QVector<RideFilePoint*> dataPoints =  myRideItem->ride()->dataPoints();
        if (!dataPoints.count()) return;

        if(dataPoints.count() < curPosition)
        {
            curPosition = dataPoints.count();
        } // make sure the current position is less than the new distance
        else if (dataPoints[curPosition]->km < rtd.getDistance())
        {
            for( ; curPosition < dataPoints.count(); curPosition++)
                if(dataPoints[curPosition]->km >= rtd.getDistance())
                    break;
        }
        else
        {
            for( ; curPosition > 0; curPosition--)
                if(dataPoints[curPosition]->km <= rtd.getDistance())
                    break;
        }
        // update the rfp
        rfp = *dataPoints[curPosition];

        //TODO : weighted average to improve smoothness
    }
    // set video rate ( theoretical : video rate = training speed / ghost speed)
    float rate;
    float video_time_shift_ms;
    video_time_shift_ms = (rfp.secs*1000.0 - (double) libvlc_media_player_get_time(mp));
    if (rfp.kph == 0.0)
        rate = 1.0;
    else
        rate = rtd.getSpeed() / rfp.kph;

    // FIXME : remove debug info
/*
    qDebug() << "TimeDiff=" << qPrintable(QString("%1").arg(video_time_shift_ms / 1000.0, 5, 'f', 2, ' ')) \
             << "  TargetRate=" << qPrintable(QString("%1").arg(rate, 5, 'f', 2, ' ')) \
             << "  Corr=" << qPrintable(QString("%1").arg(1.0 + video_time_shift_ms / 10000.0, 5, 'f', 2, ' ')) \
             << "  CurrentRate=" << qPrintable(QString("%1").arg(currentVideoRate, 5, 'f', 2, ' '));
*/

    //if video is far (empiric) from ghost:
    if (fabs(video_time_shift_ms) > 15000)
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

#endif

#ifdef GC_VIDEO_QT5
//TODO
//    // seek to ms position in current file
//    mp->setPosition(ms);
#endif

    speedmeterwidget->Value = rtd.getSpeed();
    textspeedmeterwidget->Text = QString::number((int) rtd.getSpeed());
    textspeedmeterwidget->AltText = QString(".") +QString::number((int)(rtd.getSpeed() * 10.0) - (((int) rtd.getSpeed()) * 10)) + tr(" kph");

    powermeterwidget->Value =  rtd.getWatts();
    textpowermeterwidget->Text = QString::number((int)powermeterwidget->Value);

    cadencemeterwidget->Value = rtd.getCadence();
    textcadencemeterwidget->Text = QString::number((int)cadencemeterwidget->Value);

    textHRMmeterwidget->Text = QString::number((int)rtd.getHr()) + tr(" bpm");

    foreach(MeterWidget* p_meterWidget , m_metersWidget)
    {
        p_meterWidget->update();
    }
}

void VideoWindow::seekPlayback(long ms)
{

#ifdef GC_VIDEO_NONE
    Q_UNUSED(ms)
#endif

#ifdef GC_VIDEO_VLC
    if (!m) return;

    // seek to ms position in current file
    libvlc_media_player_set_time(mp, (libvlc_time_t) ms);
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

#ifdef Q_OS_LINUX
        QString fileURL = "file://" + filename.replace(" ","%20").replace("\\", "/");
#else
        // A Windows "c:\xyz\abc def.avi" filename should become file:///c:/xyz/abc%20def.avi
        QString fileURL = "file:///" + filename.replace(" ","%20").replace("\\", "/");
#endif
        //qDebug()<<"file url="<<fileURL;


        /* open media */
        m = libvlc_media_new_location(inst, filename.endsWith("/DVD") ? "dvd://" : fileURL.toLatin1());

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
