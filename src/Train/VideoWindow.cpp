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
#include <QtGlobal>
#include "VideoWindow.h"
#include "Context.h"
#include "Athlete.h"
#include "RideItem.h"
#include "RideFile.h"
#include "MeterWidget.h"
#include "VideoLayoutParser.h"
#include "HelpWhatsThis.h"

class Lock
{
    QRecursiveMutex& mutex;
public:
    Lock(QRecursiveMutex& m) : mutex(m) { mutex.lock(); }
    ~Lock() { mutex.unlock(); }
};

VideoWindow::VideoWindow(Context *context)  :
    GcChartWindow(context), context(context), m_MediaChanged(false), layoutSelector(NULL)
{
    HelpWhatsThis *helpContents = new HelpWhatsThis(this);
    this->setWhatsThis(helpContents->getWhatsThisText(HelpWhatsThis::ChartTrain_VideoPlayer));

    QWidget *c = NULL;
    setProperty("color", QColor(Qt::black));

    QHBoxLayout *layout = new QHBoxLayout();
    setChartLayout(layout);

    videoSyncDistanceAdjustFactor = 1.;
    videoSyncTimeAdjustFactor = 1.;

    curPosition = 1;

    state = PlaybackState::None;

    init = true; // assume initialisation was ok ...

#ifdef GC_VIDEO_VLC
    //
    // USE VLC VIDEOPLAYER
    //

#ifdef Q_OS_MAC
    QString VLC_PLUGIN_PATH = QProcessEnvironment::systemEnvironment().value("VLC_PLUGIN_PATH", "");
    if (VLC_PLUGIN_PATH.isEmpty()) qputenv("VLC_PLUGIN_PATH", QString(QCoreApplication::applicationDirPath() + "/../Frameworks/plugins").toUtf8());
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

    /* Load the VLC engine */
    inst = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);

    /* Create a new item */
    if (inst) { // if vlc doesn't initialise don't even try!

        m = NULL;

        /* Create a media player playing environement */
        mp = libvlc_media_player_new (inst);

        container = new QWidget(this);
        layout->addWidget(container);

#if defined(WIN32)
        libvlc_media_player_set_hwnd (mp, (HWND)(container->winId()));
#elif defined(Q_OS_MAC)
        libvlc_media_player_set_nsobject (mp, (void*)(container->winId()));
#elif defined(Q_OS_LINUX)
        libvlc_media_player_set_xwindow (mp, container->winId());
#endif

#if defined(WIN32) || defined(Q_OS_LINUX)

        // Read the video layouts just to list the names for the layout selector
        readVideoLayout(-1);

        // Create the layout selector form
        c = new QWidget(this);
        HelpWhatsThis *helpConfig = new HelpWhatsThis(c);
        c->setWhatsThis(helpConfig->getWhatsThisText(HelpWhatsThis::ChartTrain_VideoPlayer));
        c->setContentsMargins(0,0,0,0);
        QVBoxLayout *cl = new QVBoxLayout(c);
        QFormLayout *controlsLayout = new QFormLayout();
        controlsLayout->setSpacing(0);
        controlsLayout->setContentsMargins(5,5,5,5);
        cl->addLayout(controlsLayout);
        QLabel *layoutLabel = new QLabel(tr("Meters layout"), this);
        layoutLabel->setAutoFillBackground(true);
        layoutSelector = new QComboBox(this);

        for(int i = 0; i < layoutNames.length(); i++) {
            layoutSelector->addItem(layoutNames[i], i);
        }
        controlsLayout->addRow(layoutLabel, layoutSelector);
        QPushButton *resetLayoutBtn = new QPushButton(tr("Reset Meters layout to default"), this);
        resetLayoutBtn->setAutoDefault(false);
        cl->addWidget(resetLayoutBtn);

        connect(context, SIGNAL(configChanged(qint32)), this, SLOT(layoutChanged()));
        connect(layoutSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(layoutChanged()));
        connect(resetLayoutBtn, SIGNAL(clicked()), this, SLOT(resetLayout()));

        // Instantiate a layout as initial default
        layoutChanged();
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

    setControls(c);

    if (init) {
        // get updates..
        connect(context, SIGNAL(telemetryUpdate(RealtimeData)), this, SLOT(telemetryUpdate(RealtimeData)));
        connect(context, SIGNAL(stop()), this, SLOT(stopPlayback()));
        connect(context, SIGNAL(start()), this, SLOT(startPlayback()));
        connect(context, SIGNAL(pause()), this, SLOT(pausePlayback()));
        connect(context, SIGNAL(seek(long)), this, SLOT(seekPlayback(long)));
        connect(context, SIGNAL(unpause()), this, SLOT(resumePlayback()));
        connect(context, SIGNAL(mediaSelected(QString)), this, SLOT(mediaSelected(QString)));

        // The video file may have been already selected
        mediaSelected(context->videoFilename);
        // We may add a video player while training is already running!
        if(context->isRunning) startPlayback();
    }
}

VideoWindow::~VideoWindow()
{
    if (!init) return; // we didn't initialise properly so all bets are off

    stopPlayback();

#ifdef GC_VIDEO_VLC
    // VLC

    /* No need to keep the media now */
    if (m) {
        libvlc_media_t* capture_m = this->m;
        vlcDispatch.AsyncCall([capture_m]() {libvlc_media_release(capture_m); });
    }

    /* nor the player */
    libvlc_media_player_t* capture_mp = this->mp;
    vlcDispatch.AsyncCall([capture_mp]() {libvlc_media_player_release(capture_mp); });

    // unload vlc
    libvlc_instance_t* capture_inst = this->inst;
    vlcDispatch.AsyncCall([capture_inst]() {libvlc_release(capture_inst); });

    vlcDispatch.Drain();
#endif

#ifdef GC_VIDEO_QT5
    // QT MEDIA
    delete mp;
    delete wd;
#endif
}

// Note: This method should only be called under state lock.
bool VideoWindow::hasActiveVideo() const
{
    if (state == PlaybackState::None)
        return false;

#ifdef GC_VIDEO_VLC
    libvlc_media_player_t* capture_mp = this->mp;
    switch (vlcDispatch.SyncCall<libvlc_state_t>([capture_mp]() {return libvlc_media_player_get_state(capture_mp);})) {
    case libvlc_Playing:
    case libvlc_Paused:
        return true;
    default:
        break; // needed for compiler warning
    }
#endif
#ifdef GC_VIDEO_QT5
    if (mp->state() != QMediaPlayer::StoppedState)
        return true;
#endif

    return false;
}

void VideoWindow::layoutChanged()
{
    readVideoLayout(videoLayout());
}

void VideoWindow::resetLayout()
{
    readVideoLayout(-1, true);
    layoutSelector->clear();
    for(int i = 0; i < layoutNames.length(); i++) {
        layoutSelector->addItem(layoutNames[i], i);
    }
}

void VideoWindow::readVideoLayout(int pos, bool useDefault)
{
    // Video Overlays Initialization: if video config file is not present
    // copy a default one to be used as a model by the user.
    // An empty video-layout.xml file disables video overlays
    QString filename = context->athlete->home->config().canonicalPath() + "/" + "video-layout.xml";
    QFile file(filename);
    if (useDefault) file.remove();
    if (!file.exists())
    {
        file.setFileName(":/xml/video-layout.xml");
        file.copy(filename);
        QFile::setPermissions(filename, QFileDevice::ReadUser|QFileDevice::WriteUser);
    }
    if (file.exists())
    {
        // clean previous layout
        foreach(MeterWidget* p_meterWidget, m_metersWidget)
        {
            m_metersWidget.removeAll(p_meterWidget);
            p_meterWidget->deleteLater();
        }
        layoutNames.clear();

        VideoLayoutParser handler(&m_metersWidget, &layoutNames, container);
        QXmlInputSource source(&file);
        QXmlSimpleReader reader;
        handler.layoutPositionSelected = pos;
        reader.setContentHandler(&handler);
        reader.parse(source);
    }
    else
    {
        qDebug() << qPrintable(QString("file" + filename + " (video layout XML file) not found"));
    }
}

void VideoWindow::showMeters()
{
    foreach(MeterWidget* p_meterWidget , m_metersWidget)
    {
        p_meterWidget->setWindowOpacity(1); // Show the widget
        p_meterWidget->AdjustSizePos();
        p_meterWidget->update();
        p_meterWidget->raise();
        p_meterWidget->show();
        p_meterWidget->startPlayback(context);
    }
    prevPosition = mapToGlobal(pos());
}

void VideoWindow::resizeEvent(QResizeEvent * )
{
    Lock lock(stateLock);
    if (!hasActiveVideo())
        return;

    foreach(MeterWidget* p_meterWidget , m_metersWidget)
        p_meterWidget->AdjustSizePos();
    prevPosition = mapToGlobal(pos());
}

VideoSyncFilePoint VideoWindow::VideoSyncPointAdjust(const VideoSyncFilePoint& vsfp) const
{
    VideoSyncFilePoint r(vsfp);
    r.secs *= videoSyncTimeAdjustFactor;
    r.km *= videoSyncDistanceAdjustFactor;
    return r;
}

void VideoWindow::startPlayback()
{
    Lock lock(stateLock);

#ifdef GC_VIDEO_VLC
    if (!m) return; // ignore if no media selected

    // stop playback & wipe player
    libvlc_media_player_t* capture_mp = this->mp;
    vlcDispatch.AsyncCall([capture_mp](){libvlc_media_player_stop(capture_mp); });

    /* set the media to playback */
    libvlc_media_t* capture_m = this->m;
    vlcDispatch.AsyncCall([capture_mp, capture_m](){libvlc_media_player_set_media(capture_mp, capture_m); });

    /* Reset playback rate */
    /* If video speed will be controlled by a sync file, set almost stationary
       until first telemetry update. Otherwise (re)set to normal rate */
    float rate = 1.0f;
    if (context->currentVideoSyncFile() && context->currentVideoSyncFile()->Points.count() > 1)
        rate = 0.1f;
    vlcDispatch.AsyncCall([capture_mp, rate](){libvlc_media_player_set_rate(capture_mp, rate); });

    /* play the media_player */
    vlcDispatch.AsyncCall([capture_mp] () { libvlc_media_player_play(capture_mp); });

    m_MediaChanged = false;
#endif

#ifdef GC_VIDEO_QT5
    // open the media object
    mp->play();
#endif

    // Compute VideoSync unit correction factors.
    videoSyncTimeAdjustFactor = 1.;
    videoSyncDistanceAdjustFactor = 1.;
    VideoSyncFile* currentVideoSyncFile = context->currentVideoSyncFile();
    if (currentVideoSyncFile) {

        // VideoSync Distance Correction.

        // Adjust distance factor wrt ratio of videosync distance duration to ergfile workout duraction
        // For example if videosync is for a route is 20 km long, and workout says it is 15km long, then
        // videosync's distances must all be adjusted by 15 / 20.

        // This commonly occurs when a gpx file is used for workout with an older
        // tts used for videosync. Example: CH_Umbrail, or pretty much any older
        // tacx video where tts doesn't contain location data.
        double videoSyncDistanceMeters = currentVideoSyncFile->Distance * 1000.;
        if (videoSyncDistanceMeters > 0.) {
            ErgFile* currentErgFile = context->currentErgFile();
            if (currentErgFile) {
                double ergFileDistanceMeters = currentErgFile->Duration;
                if (ergFileDistanceMeters > 0.) {
                    videoSyncDistanceAdjustFactor = ergFileDistanceMeters / videoSyncDistanceMeters;
                }
            }
        }

        // VideoSync Time Correction.

        // Videosync files will often declare a framerate that differs from its associated
        // video's frame rate. The videosyncs declarated time points must be translated
        // into video time. This is done by multiplying all videosync time by the
        // videoSyncFrameAdjustFactor, which is Videosync-FrameRate / Actual-Media-FrameRate.

        // The videoSyncFrameAdjustFactor is applied by loading each videosync point through the
        // VideoSyncTimeAdjust method and using its rewritten form.

        // This rewrite must wait until media load time since that is the earliest that we can
        // query for media's actual frame rate.

        // This issue occurs on almost all tacx virtual rides.
#ifdef GC_VIDEO_VLC
        double videoSyncFrameRate = currentVideoSyncFile->VideoFrameRate;
        if (videoSyncFrameRate > 0.) {
            libvlc_media_player_t* capture_mp = this->mp;
            double mediaFrameRate = vlcDispatch.SyncCall<double>([capture_mp] () { return libvlc_media_player_get_fps(capture_mp); });
            if (mediaFrameRate > 0.) {
                videoSyncTimeAdjustFactor = videoSyncFrameRate / mediaFrameRate;
            }
        }
#endif
#if defined(GC_VIDEO_QT5)
        // QT doesn't expose media frame rate so make due with duration.
        double videoSyncDuration = currentVideoSyncFile->Duration;
        if (videoSyncDuration > 0) {
            double mediaDuration = (double)mp->duration();
            if (mediaDuration > 0) {
                videoSyncTimeAdjustFactor = mediaDuration / videoSyncDuration;
            }
        }
#endif
    }

    state = PlaybackState::Playing;

    showMeters();
}

void VideoWindow::stopPlayback()
{
    Lock lock(stateLock);

    // Widget communication... its very important that all widgets
    // are done and do not try to paint after 'the stop' has begun.
    // QT paints on a different thread from widget paint method, so
    // we must get acknowledgement from all widgets that someone has
    // attempted to paint after stop was seen. If we don't see that
    // we must assume qt is still painting a widget.

    // Careful notification process:

    // 1.) If we havent started playback then do nothing here.
    if (state == PlaybackState::None)
        return;

    // 2.) Let everyone know we're now officially not running,
    //     even before we actually call video stop.
    state = PlaybackState::None;

    // 3.) Tell all the widgets to stop, and tell them to hide.
    //     The hide means they will no longer see paint.

    // We do this stuff before calling stop, because stop is a
    // synchronous operation that expects to be able to tear
    // down decoder and its resources.
    foreach(MeterWidget * p_meterWidget, m_metersWidget) {
        p_meterWidget->stopPlayback();
        p_meterWidget->hide();
    }

#ifdef GC_VIDEO_VLC
    if (!m) return; // ignore if no media selected

    // stop playback & wipe player
    libvlc_media_player_t* capture_mp = this->mp;
    vlcDispatch.AsyncCall([capture_mp]{ libvlc_media_player_stop(capture_mp); });
#endif

#ifdef GC_VIDEO_QT5
    mp->stop();
#endif
}

void VideoWindow::pausePlayback()
{
    Lock lock(stateLock);
    if (!hasActiveVideo())
        return;

    state = PlaybackState::Paused;

#ifdef GC_VIDEO_VLC
    if (!m) return; // ignore if no media selected

    // stop playback & wipe player
    libvlc_media_player_t* capture_mp = this->mp;
    vlcDispatch.AsyncCall([capture_mp]{libvlc_media_player_set_pause(capture_mp, true); });
#endif

#ifdef GC_VIDEO_QT5
    mp->pause();
#endif
}

void VideoWindow::resumePlayback()
{
    Lock lock(stateLock);
    if (!hasActiveVideo())
        return;

#ifdef GC_VIDEO_VLC
    if (!m) return; // ignore if no media selected

    // stop playback & wipe player
    if(m_MediaChanged)
        startPlayback();
    else {
        libvlc_media_player_t* capture_mp = this->mp;
        vlcDispatch.AsyncCall([capture_mp]{libvlc_media_player_set_pause(capture_mp, false); });
    }
#endif

#ifdef GC_VIDEO_QT5
    mp->play();
#endif

    state = PlaybackState::Playing;
}

void VideoWindow::telemetryUpdate(RealtimeData rtd)
{
    Lock lock(stateLock);
    if (!hasActiveVideo())
        return;

    bool metric = GlobalContext::context()->useMetricUnits;

    foreach(MeterWidget* p_meterWidget , m_metersWidget)
    {
        QString myQstr1 = p_meterWidget->Source();
        std::string smyStr1 = myQstr1.toStdString();
        if (p_meterWidget->Source() == QString("None"))
        {
            p_meterWidget->AltText = p_meterWidget->AltTextSuffix;
        }
        else if (p_meterWidget->Source() == QString("Speed"))
        {
            p_meterWidget->Value = rtd.getSpeed() * (metric ? 1.0 : MILES_PER_KM);
            p_meterWidget->Text = QString::number((int)p_meterWidget->Value).rightJustified(p_meterWidget->textWidth);
            p_meterWidget->AltText = QString(".") +QString::number((int)(p_meterWidget->Value * 10.0) - (((int) p_meterWidget->Value) * 10)) + (metric ? tr(" kph") : tr(" mph")) + p_meterWidget->AltTextSuffix;
        }
        else if (p_meterWidget->Source() == QString("Elevation"))
        {
            // Do not show in ERG mode
            if (rtd.mode == ERG || rtd.mode == MRC)
            {
                p_meterWidget->setWindowOpacity(0); // Hide the widget
            }
            p_meterWidget->Value = rtd.getRouteDistance();
            ElevationMeterWidget* elevationMeterWidget = dynamic_cast<ElevationMeterWidget*>(p_meterWidget);
            if (!elevationMeterWidget)
                qDebug() << "Error: Elevation keyword used but widget is not elevation type";
            else
            {
                elevationMeterWidget->setContext(context);
                elevationMeterWidget->gradientValue = rtd.getSlope();
            }
        }
        else if (p_meterWidget->Source() == QString("LiveMap"))
        {
            LiveMapWidget* liveMapWidget = dynamic_cast<LiveMapWidget*>(p_meterWidget);
            if (!liveMapWidget)
                qDebug() << "Error: LiveMap keyword used but widget is not LiveMap type";
            else
            {
                double dLat = rtd.getLatitude();
                double dLon = rtd.getLongitude();
                double dAlt = rtd.getAltitude();

                // show/plot or hide depending on existance of valid location
                // data, only when there is a video to play
                geolocation geo(dLat, dLon, dAlt);
                if (geo.IsReasonableGeoLocation())
                {
                    liveMapWidget->plotNewLatLng(dLat, dLon);
                }
            }
        }
        else if (p_meterWidget->Source() == QString("Cadence"))
        {
            p_meterWidget->Value = rtd.getCadence();
            p_meterWidget->Text = QString::number((int)p_meterWidget->Value).rightJustified(p_meterWidget->textWidth);
            p_meterWidget->AltText = p_meterWidget->AltTextSuffix;
        }
        else if (p_meterWidget->Source() == QString("Watt"))
        {
            p_meterWidget->Value =  rtd.getWatts();
            p_meterWidget->Text = QString::number((int)p_meterWidget->Value).rightJustified(p_meterWidget->textWidth);
            p_meterWidget->AltText = p_meterWidget->AltTextSuffix;
        }
        else if (p_meterWidget->Source() == QString("Altitude"))
        {
            p_meterWidget->Value =  rtd.getAltitude() * (metric ? 1.0 : FEET_PER_METER);
            p_meterWidget->Text = QString::number((int)p_meterWidget->Value).rightJustified(p_meterWidget->textWidth);
            p_meterWidget->AltText = (metric ? tr(" m") : tr(" feet")) + p_meterWidget->AltTextSuffix;
        }
        else if (p_meterWidget->Source() == QString("HRM"))
        {
            p_meterWidget->Value =  rtd.getHr();
            p_meterWidget->Text = QString::number((int)p_meterWidget->Value).rightJustified(p_meterWidget->textWidth);
            p_meterWidget->AltText = p_meterWidget->AltTextSuffix;
        }
        else if (p_meterWidget->Source() == QString("Load"))
        {
            if (rtd.mode == ERG || rtd.mode == MRC) {
                p_meterWidget->Value = rtd.getLoad();
                p_meterWidget->Text = QString("%1").arg(round(p_meterWidget->Value)).rightJustified(p_meterWidget->textWidth);
                p_meterWidget->AltText = tr("w") +  p_meterWidget->AltTextSuffix;
            } else {
                p_meterWidget->Value = rtd.getSlope();
                p_meterWidget->Text = ((-1.0 < p_meterWidget->Value && p_meterWidget->Value < 0.0) ? QString("-") : QString("")) + QString::number((int)p_meterWidget->Value).rightJustified(p_meterWidget->textWidth);
                p_meterWidget->AltText = QString(".") + QString::number(abs((int)(p_meterWidget->Value * 10.0) - (((int) p_meterWidget->Value) * 10))) + tr("%") + p_meterWidget->AltTextSuffix;
            }
        }
        else if (p_meterWidget->Source() == QString("Distance"))
        {
            p_meterWidget->Value = rtd.getDistance() * (metric ? 1.0 : MILES_PER_KM);
            p_meterWidget->Text = QString::number((int) p_meterWidget->Value).rightJustified(p_meterWidget->textWidth);
            p_meterWidget->AltText = QString(".") +QString::number((int)(p_meterWidget->Value * 10.0) - (((int) p_meterWidget->Value) * 10)) + (metric ? tr(" km") : tr(" mi")) + p_meterWidget->AltTextSuffix;
        }
        else if (p_meterWidget->Source() == QString("Time"))
        {
            p_meterWidget->Value = round(rtd.value(RealtimeData::Time)/100.0)/10.0;
            p_meterWidget->Text = time_to_string(trunc(p_meterWidget->Value)).rightJustified(p_meterWidget->textWidth);
            p_meterWidget->AltText = QString(".") + QString::number((int)(p_meterWidget->Value * 10.0) - (((int) p_meterWidget->Value) * 10)) + p_meterWidget->AltTextSuffix;
        }
        else if (p_meterWidget->Source() == QString("LapTime"))
        {
            p_meterWidget->Value = round(rtd.value(RealtimeData::LapTime)/100.0)/10.0;
            p_meterWidget->Text = time_to_string(trunc(p_meterWidget->Value)).rightJustified(p_meterWidget->textWidth);
            p_meterWidget->AltText = QString(".") + QString::number((int)(p_meterWidget->Value * 10.0) - (((int) p_meterWidget->Value) * 10)) + p_meterWidget->AltTextSuffix;
        }
        else if (p_meterWidget->Source() == QString("LapTimeRemaining"))
        {
            p_meterWidget->Value = round(rtd.value(RealtimeData::LapTimeRemaining)/100.0)/10.0;
            p_meterWidget->Text = time_to_string(trunc(p_meterWidget->Value)).rightJustified(p_meterWidget->textWidth);
            p_meterWidget->AltText = QString(".") + QString::number((int)(p_meterWidget->Value * 10.0) - (((int) p_meterWidget->Value) * 10)) + p_meterWidget->AltTextSuffix;
        }
        else if (p_meterWidget->Source() == QString("ErgTimeRemaining"))
        {
            p_meterWidget->Value = round(rtd.value(RealtimeData::ErgTimeRemaining)/100.0)/10.0;
            p_meterWidget->Text = time_to_string(trunc(p_meterWidget->Value)).rightJustified(p_meterWidget->textWidth);
            p_meterWidget->AltText = QString(".") + QString::number((int)(p_meterWidget->Value * 10.0) - (((int) p_meterWidget->Value) * 10)) + p_meterWidget->AltTextSuffix;
        }
        else if (p_meterWidget->Source() == QString("TrainerStatus"))
        {
            p_meterWidget->AltText = p_meterWidget->AltTextSuffix;

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

    // The Meter Widgets need to follow the Video Window when it moves
    // (main window moves, scrolling...), we check the position at every update
    if(mapToGlobal(pos()) != prevPosition) resizeEvent(NULL);

    foreach(MeterWidget* p_meterWidget , m_metersWidget)
        p_meterWidget->update();

#ifdef GC_VIDEO_NONE
    Q_UNUSED(rtd)
#endif

#ifdef GC_VIDEO_VLC
    if (PlaybackState::Playing != state)
        return;

    // find the curPosition
    if (context->currentVideoSyncFile())
    {
        // when we selected a videosync file in training mode (rlv...):

        QVector<VideoSyncFilePoint> VideoSyncFiledataPoints = context->currentVideoSyncFile()->Points;
        if (VideoSyncFiledataPoints.count()<2) return;

        if (curPosition > VideoSyncFiledataPoints.count() - 1 || curPosition < 1)
            curPosition = 1; // minimum curPosition is 1 as we will use [curPosition-1]

        // Ensure distance is within bounds of the videosyncfile.
        double CurrentDistance = qBound(0.0, rtd.getRouteDistance(), context->currentVideoSyncFile()->Distance);

        // make sure the current position is less than the new distance
        while ((VideoSyncPointAdjust(VideoSyncFiledataPoints[curPosition]).km > CurrentDistance) && (curPosition > 1))
            curPosition--;
        while ((VideoSyncPointAdjust(VideoSyncFiledataPoints[curPosition]).km <= CurrentDistance) && (curPosition < VideoSyncFiledataPoints.count()-1))
            curPosition++;

        /* Create an RFP to represent where we are */
        VideoSyncFilePoint syncPrevious = VideoSyncPointAdjust(VideoSyncFiledataPoints[curPosition-1]);
        VideoSyncFilePoint syncNext     = VideoSyncPointAdjust(VideoSyncFiledataPoints[curPosition]);

        double syncKmDelta = syncNext.km - syncPrevious.km;
        double syncKphDelta = syncNext.kph - syncPrevious.kph;
        double syncTimeDelta = syncNext.secs - syncPrevious.secs;
        double distanceFactor, speedFactor, timeFactor, timeExtra;

        // Calculate how far we are between points in terms of distance
        if (syncKmDelta == 0) distanceFactor = 0.0;
        else distanceFactor = (CurrentDistance - syncPrevious.km) / syncKmDelta;

        // Now create the appropriate factors and interpolate the
        // video speed and time for the point we have reached.
        // If there has been no acceleration we can just use use the distance factor
        if (syncKphDelta == 0) {
            // Constant filming speed
            rfp.kph = syncPrevious.kph;
            rfp.secs = syncPrevious.secs + syncTimeDelta * distanceFactor;
        }
        else {
            // Calculate time difference because of change in speed
            timeExtra = syncTimeDelta - ((syncKmDelta / syncPrevious.kph) * 3600);
            if (syncKphDelta > 0) {
                // The filming speed increased
                speedFactor = qPow(distanceFactor, 0.66667);
                timeFactor = qPow(distanceFactor, 0.33333);
                rfp.kph = syncPrevious.kph + speedFactor * syncKphDelta;
            }
            else {
                // The filming speed decreased
                speedFactor = 1 - qPow(distanceFactor, 1.5);
                timeFactor = qPow(distanceFactor, 3.0);
                rfp.kph = syncNext.kph - speedFactor * syncKphDelta;
            }
            rfp.secs = syncPrevious.secs + (distanceFactor * (syncTimeDelta - timeExtra)) + (timeFactor * timeExtra);
        }
        rfp.km = CurrentDistance;

        // set video rate ( theoretical : video rate = training speed / ghost speed)
        float rate;
        float video_time_shift_ms;

        libvlc_media_player_t* capture_mp = this->mp;
        video_time_shift_ms = (rfp.secs * 1000.0 - vlcDispatch.SyncCall<double>([capture_mp]{ return libvlc_media_player_get_time(capture_mp); }));
        if (rfp.kph == 0.0)
            rate = 1.0;
        else
            rate = rtd.getSpeed() / rfp.kph;

        //if video is far (empiric) from ghost:
        if (fabs(video_time_shift_ms) > 5000)
        {
            double rfp_secs = rfp.secs;
            vlcDispatch.AsyncCall([capture_mp, rfp_secs]{ libvlc_media_player_set_time(capture_mp, (libvlc_time_t)(rfp_secs * 1000.0)); });
        }
        else {
            // otherwise add "small" empiric corrective parameter to get video back to ghost position:
            rate *= 1.0 + (video_time_shift_ms / 10000.0);
        }

        vlcDispatch.AsyncCall([capture_mp, rate]{ libvlc_media_player_set_pause(capture_mp, (rate < 0.01)); });

        // change video rate but only if there is a significant change
        if ((rate != 0.0) && (fabs((rate - currentVideoRate) / rate) > 0.05))
        {
            vlcDispatch.AsyncCall([capture_mp, rate]{ libvlc_media_player_set_rate(capture_mp, rate); });
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
    Lock lock(stateLock);
    if (!hasActiveVideo())
        return;

#ifdef GC_VIDEO_NONE
    Q_UNUSED(ms)
#endif

#ifdef GC_VIDEO_VLC
    if (!m) return;

    // when we selected a videosync file in training mode (rlv...)
    if (context->currentVideoSyncFile())
    {
        // Nothing to do here, videosync distance adjustments are handled elsewhere.
    }
    else
    {
        // seek to ms position in current file
        libvlc_media_player_t* capture_mp = this->mp;
        vlcDispatch.AsyncCall([capture_mp, ms]{ libvlc_media_player_set_time(capture_mp, (libvlc_time_t)ms); });
    }
#endif

#ifdef GC_VIDEO_QT5
    Q_UNUSED(ms)
//TODO
//    // seek to ms position in current file
//    mp->setPosition(ms);
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
    if (m) {
        libvlc_media_t* capture_m = this->m;
        vlcDispatch.AsyncCall([capture_m]{ libvlc_media_release(capture_m); });
    }
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
        std::string loc = filename.endsWith("/DVD") ? "dvd://" : fileURL.toLocal8Bit().constData();
        libvlc_instance_t* capture_inst = this->inst;
        m = vlcDispatch.SyncCall<libvlc_media_t*>([capture_inst, loc]{ return libvlc_media_new_location(capture_inst, loc.c_str()); });

        /* set the media to playback */
        if (m) {
            libvlc_media_player_t* capture_mp = this->mp;
            libvlc_media_t* capture_m = this->m;
            vlcDispatch.AsyncCall([capture_mp, capture_m]{ libvlc_media_player_set_media(capture_mp, capture_m); });
        }

        m_MediaChanged = true;
    }
#endif

#ifdef GC_VIDEO_QT5
    // QT MEDIA
    mc = QMediaContent(QUrl::fromLocalFile(filename));
    mp->setMedia(mc);
#endif
    if(context->isRunning) startPlayback();
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
